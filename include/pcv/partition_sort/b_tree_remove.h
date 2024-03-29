#pragma once
#include <string>
#include <tuple>
#include <pcv/partition_sort/b_tree_search.h>

namespace pcv {

template<typename BTree>
class BTreeRemove: _BTreeNodeNavigator<BTree> {
	using BTreeSearch_t = BTreeSearch<BTree>;
public:
	using Node = typename BTree::Node;
	using rule_spec_t = typename BTree::rule_spec_t;
	using key_t = typename BTree::key_t;
	using rule_value_t = typename BTree::rule_value_t;

	BTreeSearch_t searcher;

	BTreeRemove(BTree &_t) :
			_BTreeNodeNavigator<BTree>(_t), searcher(_t) {
	}

	void remove(const rule_spec_t &k) {
		std::vector<std::tuple<Node*, Node*, unsigned>> path;
		path.reserve(64);
		searcher.search_path(this->tree.root, this->tree.dimension_order, k,
				path);
		//search in boom-up manner
		for (int i = int(path.size()) - 1; i >= 0; i--) {
			auto d = this->tree.dimension_order[i];
			auto key = k.first[d];
			// <node, index of item>
			Node *root;
			Node *node;
			unsigned index;
			Node *new_root;
			std::tie(root, node, index) = path[i];
			auto nl = this->get_next_layer(*node, index);
			if (nl) {
				// delete only rule specification
				node->value[index] = rule_value_t();
				// there is something down in the tree and there is not a record about
				// this rule up in tree
				break;
			} else {
				// delete whole item in node
				new_root = remove_1d(key, root);
			}
			if (root == this->tree.root) {
				this->tree.root = new_root;
				// skip check of the parent as there is not any
				break;
			}

			// replace next payer pointer in up level
			Node *prev_root;
			Node *prev_node;
			unsigned prev_index;
			std::tie(prev_root, prev_node, prev_index) = path.at(i - 1);
			this->set_next_layer(*prev_node, prev_index, nl);
		}
	}
	Node* remove_1d(const Range1d<key_t> &k, Node *current_root) {
		assert(current_root && "The tree is empty");

		// Call the remove function for root
		remove(*current_root, k);

		// If the root node has 0 keys, make its first child as the new root
		// if it has a child, otherwise set root as NULL
		if (current_root->key_cnt == 0) {
			auto tmp = current_root;
			if (current_root->is_leaf) {
				current_root = nullptr;
			} else {
				current_root = this->child(*current_root, 0);
				current_root->parent = nullptr;
			}
			// Free the old root
			tmp->clean_children();
			this->tree.node_allocator.release(tmp);
		}
		return current_root;
	}

	/* A wrapper function to remove the key k in subtree rooted with
	 * this node.
	 * */
	void remove(Node &node, Range1d<key_t> k) {
		unsigned idx = BTreeSearch_t::findKey(node, k);

		// The key to be removed is present in this node
		if (idx < node.key_cnt && node.get_key(idx).key == k) {
			// If the node is a leaf node - removeFromLeaf is called
			// Otherwise, removeFromNonLeaf function is called
			if (node.is_leaf) {
				removeFromLeaf(node, idx);
			} else {
				removeFromNonLeaf(node, idx);
			}
		} else {
			// If this node is a leaf node, then the key is not present in tree
			if (node.is_leaf) {
				throw std::runtime_error(
						std::string("The key ") + std::string(k)
								+ " does not exist in the tree");
			}

			// The key to be removed is present in the sub-tree rooted with this node
			// The flag indicates whether the key is present in the sub-tree rooted
			// with the last child of this node
			bool flag = idx == node.key_cnt;
			// If the child where the key is supposed to exist has less that t keys,
			// we fill that child
			if (this->child(node, idx)->key_cnt < Node::MIN_DEGREE + 1) {
				fill(node, idx);
			}
			// If the last child has been merged, it must have merged with the previous
			// child and so we recurse on the (idx-1)th child. Else, we recurse on the
			// (idx)th child which now has at least t keys
			Node *c;
			if (flag && idx > node.key_cnt) {
				c = this->child(node, idx - 1);
			} else {
				c = this->child(node, idx);
			}
			remove(*c, k);
		}
	}
	/* A function to remove the key present in idx-th position in
	 * this node which is a leaf
	 * */
	void removeFromLeaf(Node &node, unsigned idx) {
		assert(
				this->get_next_layer(node, idx) == nullptr
						and "The remove has to be performed in bottom-up order");
		// Move all the keys after the idx-th pos one place backward
		for (unsigned i = idx + 1; i < node.key_cnt; ++i)
			node.move_key(i, node, i - 1);

		// Reduce the count of keys
		node.set_key_cnt(node.key_cnt - 1);
	}

	/**
	 * A function to remove the key present in idx-th position in
	 * this node which is a non-leaf node
	 *
	 * @param idx index of the key to remove
	 */
	void removeFromNonLeaf(Node &node, unsigned idx) {
		auto k = node.get_key(idx);

		if (this->child(node, idx)->key_cnt >= Node::MIN_DEGREE + 1) {
			// If the child that precedes k (child(idx)) has atleast t keys,
			// find the predecessor 'pred' of k in the subtree rooted at
			// child(idx). Replace k by pred. Recursively delete pred
			// in child(idx)
			auto pred = searcher.getPred(node, idx);
			node.set_key(idx, pred);
			auto c = this->child(node, idx);
			remove(*c, pred.key);

		} else if (this->child(node, idx + 1)->key_cnt
				>= Node::MIN_DEGREE + 1) {
			// If the child child(idx) has less that t keys, examine child(idx+1).
			// If child(idx+1) has at least t keys, find the successor 'succ' of k in
			// the subtree rooted at child(idx+1)
			// Replace k by succ
			// Recursively delete succ in child(idx+1)
			auto succ = searcher.getSucc(node, idx);
			node.set_key(idx, succ);
			auto c = this->child(node, idx + 1);
			remove(*c, succ.key);

		} else {

			// If both child(idx) and child(idx+1) has less that t keys,merge k and all of child(idx+1)
			// into child(idx)
			// Now child(idx) contains 2t-1 keys
			// Free child(idx+1) and recursively delete k from child(idx)
			merge(node, idx);
			auto c = this->child(node, idx);
			remove(*c, k.key);
		}
	}
	/* A function to borrow a key from child(idx-1) and insert it
	 * into child(idx)
	 * */
	void borrowFromPrev(Node &node, unsigned idx) {
		Node *ch = this->child(node, idx);
		Node *sib = this->child(node, idx - 1);

		// The last key from child(idx-1) goes up to the parent and key[idx-1]
		// from parent is inserted as the first key in child(idx). Thus, the loses
		// sibling one key and child gains one key

		// Moving all key in child(idx) one step ahead
		// If child(idx) is not a leaf, move all its child pointers one step ahead
		for (int i = ch->key_cnt - 1; i >= 0; --i)
			ch->move_key(i, *ch, i + 1);

		if (!ch->is_leaf) {
			for (int i = ch->key_cnt; i >= 0; --i)
				ch->child_index[i + 1] = ch->child_index[i];
		}

		// Setting child's first key equal to keys[idx-1] from the current node
		node.move_key(idx - 1, *ch, 0);

		// Moving sibling's last child as child(idx)'s first child
		if (!ch->is_leaf) {
			auto _ch = this->child(*sib, sib->key_cnt);
			this->set_child(*ch, 0, _ch);
			//ch->child_index[0] = sib->child_index[sib->key_cnt];
		}
		// Moving the key from the sibling to the parent
		// This reduces the number of keys in the sibling
		sib->move_key(sib->key_cnt - 1, node, idx - 1);

		ch->set_key_cnt(ch->key_cnt + 1);
		sib->set_key_cnt(sib->key_cnt - 1);
	}
	// A function to borrow a key from the child(idx+1) and place
	// it in child(idx)
	void borrowFromNext(Node &node, unsigned idx) {
		Node *ch = this->child(node, idx);
		Node *sib = this->child(node, idx + 1);

		// keys[idx] is inserted as the last key in child(idx)
		node.move_key(idx, *ch, ch->key_cnt);

		// Sibling's first child is inserted as the last child
		// into child(idx)
		if (!(ch->is_leaf)) {
			auto _ch = this->child(*sib, 0);
			this->set_child(*ch, ch->key_cnt + 1, _ch);
			//ch->child_index[ch->key_cnt + 1] = sib->child_index[0];
		}
		//The first key from sib is inserted into keys[idx]
		sib->move_key(0, node, idx);

		// Moving all keys in sib one step behind
		for (int i = 1; i < sib->key_cnt; ++i)
			sib->move_key(i, *sib, i - 1);

		// Moving the child pointers one step behind
		if (not sib->is_leaf) {
			for (int i = 1; i <= sib->key_cnt; ++i)
				sib->child_index[i - 1] = sib->child_index[i];
		}

		// Increasing and decreasing the key count of child(idx) and child(idx+1)
		// respectively
		ch->set_key_cnt(ch->key_cnt + 1);
		sib->set_key_cnt(sib->key_cnt - 1);
	}
	// A function to merge child(idx) with child(idx+1)
	// child(idx+1) is freed after merging
	void merge(Node &node, unsigned idx) {
		auto ch = this->child(node, idx);
		auto sib = this->child(node, idx + 1);
		assert(idx <= node.key_cnt);

		node.move_key(idx, *ch, Node::MIN_DEGREE);

		// Copying the keys from child(idx+1) to child(idx) at the end
		// Copying the child pointers from child(idx+1) to child(idx)
		this->transfer_items(*sib, 0, *ch, Node::MIN_DEGREE + 1, sib->key_cnt);

		// Moving all keys after idx in the current node one step before -
		// to fill the gap created by moving keys[idx] to child(idx)
		// Moving the child pointers after (idx+1) in the current node one
		// step before
		node.shift_items_on_right_to_left(idx);

		// Updating the key count of child and the current node
		ch->set_key_cnt(ch->key_cnt + sib->key_cnt + 1);
		node.set_key_cnt(node.key_cnt - 1);

		sib->clean_children();
		this->tree.node_allocator.release(sib);
	}
	// A function to fill child child(idx) which has less than MIN_DEGREE-1 keys
	void fill(Node &node, unsigned idx) {
		if (idx != 0
				&& this->child(node, idx - 1)->key_cnt
						>= Node::MIN_DEGREE + 1) {
			// If the previous child(idx-1) has more than MIN_DEGREE-1 keys, borrow a key
			// from that child
			borrowFromPrev(node, idx);

		} else if (idx != node.key_cnt
				&& this->child(node, idx + 1)->key_cnt
						>= Node::MIN_DEGREE + 1) {
			// If the next child(idx+1) has more than t-1 keys, borrow a key
			// from that child
			borrowFromNext(node, idx);

		} else {
			// Merge child(idx) with its sibling
			// If child(idx) is the last child, merge it with with its previous sibling
			// Otherwise merge it with its next sibling
			if (idx != node.key_cnt) {
				merge(node, idx);
			} else {
				assert(idx > 0);
				merge(node, idx - 1);
			}
		}
	}
};

}
