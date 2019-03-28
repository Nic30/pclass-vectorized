#pragma once
#include <string>
#include <tuple>
#include <pcv/partition_sort/b_tree_search.h>

namespace pcv {

template<typename BTree>
class BTreeRemove {
public:
	using Node = typename BTree::Node;
	using rule_spec_t = typename BTree::rule_spec_t;
	using value_t = typename BTree::value_t;

	static void remove(BTree & tree, const rule_spec_t & k) {
		std::vector<std::tuple<Node *, Node *, unsigned>> path;
		path.reserve(64);
		BTreeSearch<BTree>::search_path(tree, k, path);
		//search in boom-up manner
		for (int i = int(path.size()) - 1; i >= 0; i--) {
			auto d = tree.dimension_order[i];
			auto key = k.first[d];
			// <node, index of item>
			Node * root;
			Node * node;
			unsigned index;
			Node * new_root;
			std::tie(root, node, index) = path[i];
			auto nl = node->get_next_layer(index);
			if (nl) {
				// delete only rule specification
				node->value[index] = BTree::INVALID_INDEX;
			} else {
				// delete whole item in node
				new_root = remove_1d(key, root);
			}
			if (nl) {
				// there is something down in the tree and there is not a record about
				// this rule up in tree
				break;
			}
			if (root == tree.root) {
				tree.root = new_root;
				// skip check of the parent as there is not any
				break;
			}

			// replace next payer pointer in up level
			Node * prev_root;
			Node * prev_node;
			unsigned prev_index;
			std::tie(prev_root, prev_node, prev_index) = path.at(i - 1);
			prev_node->set_next_layer(prev_index, nl);
		}
	}
	static Node * remove_1d(const Range1d<value_t> & k, Node * current_root) {
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
				current_root = current_root->child(0);
			}
			// Free the old root
			tmp->clean_children();
			delete tmp;
		}
		if (current_root)
			current_root->integrity_check();
		return current_root;
	}

	/* A wrapper function to remove the key k in subtree rooted with
	 * this node.
	 * */
	static void remove(Node & node, Range1d<value_t> k) {
		unsigned idx = BTreeSearch<BTree>::findKey(node, k);

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
			node.integrity_check();
			// If the child where the key is supposed to exist has less that t keys,
			// we fill that child
			if (node.child(idx)->key_cnt < Node::MIN_DEGREE + 1) {
				fill(node, idx);
			}
			// If the last child has been merged, it must have merged with the previous
			// child and so we recurse on the (idx-1)th child. Else, we recurse on the
			// (idx)th child which now has at least t keys
			Node * c;
			if (flag && idx > node.key_cnt) {
				c = node.child(idx - 1);
			} else {
				c = node.child(idx);
			}
			remove(*c, k);
		}
	}
	/* A function to remove the key present in idx-th position in
	 * this node which is a leaf
	 * */
	static void removeFromLeaf(Node & node, unsigned idx) {
		assert(
				node.get_next_layer(idx) == nullptr
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
	static void removeFromNonLeaf(Node & node, unsigned idx) {
		auto k = node.get_key(idx);

		if (node.child(idx)->key_cnt >= Node::MIN_DEGREE + 1) {
			// If the child that precedes k (child(idx)) has atleast t keys,
			// find the predecessor 'pred' of k in the subtree rooted at
			// child(idx). Replace k by pred. Recursively delete pred
			// in child(idx)
			auto pred = BTreeSearch<BTree>::getPred(node, idx);
			node.set_key(idx, pred);
			auto c = node.child(idx);
			remove(*c, pred.key);

		} else if (node.child(idx + 1)->key_cnt >= Node::MIN_DEGREE + 1) {
			// If the child child(idx) has less that t keys, examine child(idx+1).
			// If child(idx+1) has at least t keys, find the successor 'succ' of k in
			// the subtree rooted at child(idx+1)
			// Replace k by succ
			// Recursively delete succ in child(idx+1)
			auto succ = BTreeSearch<BTree>::getSucc(node, idx);
			node.set_key(idx, succ);
			auto c = node.child(idx + 1);
			remove(*c, succ.key);

		} else {

			// If both child(idx) and child(idx+1) has less that t keys,merge k and all of child(idx+1)
			// into child(idx)
			// Now child(idx) contains 2t-1 keys
			// Free child(idx+1) and recursively delete k from child(idx)
			merge(node, idx);
			auto c = node.child(idx);
			remove(*c, k.key);
		}
	}
	/* A function to borrow a key from child(idx-1) and insert it
	 * into child(idx)
	 * */
	static void borrowFromPrev(Node & node, unsigned idx) {
		Node * ch = node.child(idx);
		Node * sib = node.child(idx - 1);

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
			auto _ch = sib->child(sib->key_cnt);
			ch->set_child(0, _ch);
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
	static void borrowFromNext(Node & node, unsigned idx) {
		Node* ch = node.child(idx);
		Node* sib = node.child(idx + 1);

		// keys[idx] is inserted as the last key in child(idx)
		node.move_key(idx, *ch, ch->key_cnt);

		// Sibling's first child is inserted as the last child
		// into child(idx)
		if (!(ch->is_leaf)) {
			auto _ch = sib->child(0);
			ch->set_child(ch->key_cnt + 1, _ch);
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
	static void merge(Node & node, unsigned idx) {
		auto ch = node.child(idx);
		auto sib = node.child(idx + 1);
		assert(idx <= node.key_cnt);

		node.move_key(idx, *ch, Node::MIN_DEGREE);

		// Copying the keys from child(idx+1) to child(idx) at the end
		// Copying the child pointers from child(idx+1) to child(idx)
		sib->transfer_items(0, *ch, Node::MIN_DEGREE + 1, sib->key_cnt);

		// Moving all keys after idx in the current node one step before -
		// to fill the gap created by moving keys[idx] to child(idx)
		// Moving the child pointers after (idx+1) in the current node one
		// step before
		node.shift_items_on_right_to_left(idx);

		// Updating the key count of child and the current node
		ch->set_key_cnt(ch->key_cnt + sib->key_cnt + 1);
		node.set_key_cnt(node.key_cnt - 1);

		sib->clean_children();
		delete sib;

		//integrity_check(*this);
		ch->integrity_check();
	}
	// A function to fill child child(idx) which has less than MIN_DEGREE-1 keys
	static void fill(Node & node, unsigned idx) {
		if (idx != 0 && node.child(idx - 1)->key_cnt >= Node::MIN_DEGREE + 1) {
			// If the previous child(idx-1) has more than MIN_DEGREE-1 keys, borrow a key
			// from that child
			borrowFromPrev(node, idx);

		} else if (idx != node.key_cnt
				&& node.child(idx + 1)->key_cnt >= Node::MIN_DEGREE + 1) {
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
