#pragma once
#include <string>

#include <pcv/common/range.h>
#include <pcv/partition_sort/b_tree_search.h>

namespace pcv {

template<typename BTree>
class BTreeInsert {
public:
	using value_t = typename BTree::value_t;
	using Node = typename BTree::Node;
	using rule_spec_t = typename BTree::rule_spec_t;
	using index_t = typename BTree::index_t;
	using KeyInfo = typename BTree::KeyInfo;

	/*
	 * Information about insert state
	 * */
	class InsertCookie {
		size_t total_levels_required_cnt(const rule_spec_t & rule) {
			// iterate from the end of the rule ordered by dimension_order and check
			// where is the last specified value for the field
			for (int i = int(dimensio_order.size()) - 1; i >= 0; i--) {
				auto d = dimensio_order[i];
				auto & k = rule.first[d];

				if (not k.is_wildcard())
					return i;
			}

			return 0;
		}
	public:
		// [TODO] grammar fix
		std::array<unsigned, BTree::D> & dimensio_order;
		uint8_t level;
		const uint8_t requires_levels;
		const rule_spec_t & rule;

		InsertCookie(BTree & tree, const rule_spec_t & rule) :
				dimensio_order(tree.dimension_order), level(0), requires_levels(
						total_levels_required_cnt(rule)), rule(rule) {
		}
		inline Range1d<typename BTree::value_t> get_actual_key(
				const rule_spec_t & rule) const {
			auto d = dimensio_order.at(level);
			return rule.first.at(d);
		}
		/*
		 * @note including this level
		 * */
		size_t levels_required_cnt() const {
			return requires_levels - level + 1;
		}
		bool required_more_levels() const {
			return level < requires_levels;
		}
	};

	inline static void insert(BTree & tree, const rule_spec_t & rule) {
		InsertCookie cookie(tree, rule);
		insert(tree, rule, cookie);
		tree.root->integrity_check(tree.dimension_order);
	}

	/* A utility function to insert a new key in this node
	 * The assumption is, the node must be non-full when this
	 * function is called
	 * */
	static void insert_non_full(Node & node, const rule_spec_t & rule,
			InsertCookie & cookie) {
		// Initialise index as index of rightmost element
		int i = int(node.key_cnt) - 1;
		// If this is a leaf node
		auto k = cookie.get_actual_key(rule);
		if (node.is_leaf) {
			// The following loop does two things
			// a) Finds the location of new key to be inserted
			// b) Moves all greater keys to one place ahead
			while (i >= 0 && node.get_key(i) > k) {
				node.move_key(i, node, i + 1);
				i--;
			}

			// Insert the new key at found location
			bool require_more_levels = cookie.required_more_levels();
			auto r_id = require_more_levels ? BTree::INVALID_RULE : rule.second;
			node.set_key(i + 1, KeyInfo(k, r_id, BTree::INVALID_INDEX));
			node.set_key_cnt(node.key_cnt + 1);

			// continue on next layer if required
			if (require_more_levels) {
				cookie.level++;
				auto nl = node.get_next_layer(i + 1);
				nl = insert_to_root(nl, rule, cookie);
				node.set_next_layer(i + 1, nl);
			}
		} else {
			// If this node is not leaf
			// Find the child which is going to have the new key
			while (i >= 0 && node.get_key(i) > k)
				i--;

			i++;
			// See if the found child is full
			if (node.child(i)->key_cnt == Node::MAX_DEGREE) {
				// If the child is full, then split it
				split_child(node, i, *node.child(i));

				// After split, the middle key of C[i] goes up and
				// C[i] is splitted into two. See which of the two
				// is going to have the new key
				if (node.get_key(i) < k)
					i++;
			}
			assert(i >= 0);
			Node * c = node.child(i);
			insert_non_full(*c, rule, cookie);
		}
	}
	static void insert_in_to_compressed(Node * root, const rule_spec_t & rule,
			InsertCookie & cookie) {
		assert(root->is_compressed);
		assert(root->is_leaf);
		assert(root->key_cnt > 1);
// search for the longest common prefix
		uint8_t keep_keys_cnt;
		for (keep_keys_cnt = 0; keep_keys_cnt < root->key_cnt;
				keep_keys_cnt++) {
			auto k = cookie.get_actual_key(rule);
			auto _k = root->get_key(keep_keys_cnt);
			if (_k.key != k) {
				break;
			}
			cookie.level++;
		}
		_insert_in_to_compressed(root, rule, cookie, keep_keys_cnt);
	}
	static Node * decompress_node(Node * root,
			uint8_t index_of_key_to_separate) {
		// if the key does not exists in the node new empty node is created and connected
		// as new layer behind the last item
		if (root->key_cnt <= index_of_key_to_separate) {
			assert(root->key_cnt == index_of_key_to_separate);
			auto nl = new Node;
			root->set_next_layer(index_of_key_to_separate - 1, nl);
			return nl;
		} else if (index_of_key_to_separate == 0) {
			if (root->key_cnt == 1) {
				// there is only this non matching item for same dimension, we will use this item
				// and the newly generated item from the rule to build a new b-tree there
				return root;
			}
			// the re is something behind the first non matching key, wee need to to extract it to new node
			auto nl = new Node;
			for (uint8_t i2 = 1; i2 < root->key_cnt; i2++) {
				nl->set_key(i2 - 1, root->get_key(i2));
				nl->set_dim(i2 - 1, root->get_dim(i2));
			}
			nl->set_key_cnt(root->key_cnt - 1);
			nl->is_compressed = nl->key_cnt > 1;

			root->set_next_layer(0, nl);
			root->set_key_cnt(1);
			root->is_compressed = false;
			return root;
		} else {
			// the first non matching key is surrounded by the other keys
			// this item has to be extracted to a separate node and the items behind has to be extracted
			// to other node

			// extract the segment behind the first non matching item
			Node * nnl = nullptr;
			auto keys_to_nnl_cnt = root->key_cnt - index_of_key_to_separate - 1;
			if (keys_to_nnl_cnt) {
				nnl = new Node;
				for (uint8_t i2 = 0; i2 < keys_to_nnl_cnt; i2++) {
					auto i = index_of_key_to_separate + i2 + 1;
					nnl->set_key(i2, root->get_key(i));
					nnl->set_dim(i2, root->get_dim(i));
				}
				nnl->set_key_cnt(root->key_cnt - index_of_key_to_separate - 1);
				nnl->is_compressed = nnl->key_cnt > 1;
			}
			// extract the non matching item
			auto nl = new Node;
			nl->set_key(0, root->get_key(index_of_key_to_separate));
			nl->set_key_cnt(1);

			// connect the layers together
			assert(
					root->get_next_layer(index_of_key_to_separate - 1)
							== nullptr);
			root->set_next_layer(index_of_key_to_separate - 1, nl);
			if (nnl) {
				assert(nl->get_next_layer(0) == nullptr);
				nl->set_next_layer(0, nnl);
			}
			root->set_key_cnt(index_of_key_to_separate);
			root->is_compressed = root->key_cnt > 1;

			return nl;
		}
	}
	/*
	 * @note the cookie.level is expected to be set on the level of the item after the last matching item
	 * @param keep_keys_cnt the number of the same keys in the node
	 * 	(same as the rule, that means that this keys should stay in this node as they are)
	 * */
	static void _insert_in_to_compressed(Node * root, const rule_spec_t & rule,
			InsertCookie & cookie, uint8_t keep_keys_cnt) {
		assert(root->is_compressed);
		assert(root->is_leaf);
		assert(keep_keys_cnt <= root->key_cnt);

		// decompress part of the tree if required
		// insert to last common prefix and optionally create new tree from this node

		if (keep_keys_cnt == root->key_cnt) {
			// the rule of exactly same value or same prefix is already stored
			// in tree
			if (cookie.required_more_levels()) {
				// continue insert on next level
				auto nl = root->get_next_layer(keep_keys_cnt - 1);
				nl = insert(nl, rule, cookie);
				root->set_next_layer(keep_keys_cnt - 1, nl);
			} else {
				root->value[keep_keys_cnt - 1] = rule.second;
			}
			return;
		} else {
			root = decompress_node(root, keep_keys_cnt);
			root->integrity_check(cookie.dimensio_order, nullptr, cookie.level);
			insert(root, rule, cookie);
		}
//// we need to put the first non matching key to a new node so it is possible to start new b-tree there
//if (keep_keys_cnt == 0) {
//	// if the first non matching key is the first one only other keys need to be stored to a new node
//
//} else {
//
//}
//// there are items before this we need to allocate new layer and put the key there
//auto nl = new Node;
//root->set_next_layer(keep_keys_cnt - 1, nl);
//nl->set_key(keep_keys_cnt, root->get_key(keep_keys_cnt));
//nl->set_key_cnt(1);
//
//root->set_key_cnt(keep_keys_cnt - 1);
//root->is_compressed = root->key_cnt > 1;
////cookie.level--;
//nl = insert(root, rule, cookie);

//{
//	// this layer can remain for keys in this dimension only
//	// put the rest in to a separate node
//	// the new node will contain this key and the key from the node and will be for this dimension
//	root->is_compressed = false;
//	auto nl = new Node;
//	root->set_next_layer(0, nl);
//	assert(root->key_cnt > 1);
//	// copy the data to new node
//	for (uint8_t i2 = 1; i2 < root->key_cnt; i2++) {
//		nl->set_key(i2 - 1, root->get_key(i2));
//		nl->set_dim(i2 - 1, root->get_dim(i2));
//	}
//	nl->set_key_cnt(root->key_cnt - 1);
//	nl->is_compressed = nl->key_cnt > 1;
//	root->set_key_cnt(1);
//	if (keep_keys_cnt == 1) {
//		// the same key is in node
//		// continue on other level
//		insert(nl, rule, cookie);
//	} else {
//		// proper key is not in root have to add it
//		//cookie.level--;
//		root = insert(root, rule, cookie);
//	}
//} else {
//	// there are items before this we need to allocate new layer and put the key there
//	auto nl = new Node;
//	root->set_next_layer(keep_keys_cnt - 1, nl);
//	nl->set_key(keep_keys_cnt, root->get_key(keep_keys_cnt));
//	nl->set_key_cnt(1);
//
//	// create another new layer for the remaining items if required if it is required
//	if (root->key_cnt > keep_keys_cnt) {
//		auto nnl = new Node;
//		for (uint8_t i2 = 0; i2 < (root->key_cnt - keep_keys_cnt);
//				i2++) {
//			nnl->set_key(i2, root->get_key(keep_keys_cnt + i2));
//			nnl->set_dim(i2, root->get_dim(keep_keys_cnt + i2));
//		}
//		nnl->set_key_cnt(root->key_cnt - 2);
//		nnl->is_compressed = nnl->key_cnt > 1;
//		nl->set_next_layer(0, nnl);
//	}
//	root->set_key_cnt(keep_keys_cnt - 1);
//	root->is_compressed = root->key_cnt > 1;
//	//cookie.level--;
//	nl = insert(root, rule, cookie);
//}

	}

	/*
	 * Insert to a compressed node
	 *
	 * @note just a copy of the
	 * */
	static void insert_compressed(Node * root, const rule_spec_t & rule,
			InsertCookie & cookie, size_t keys_to_insert) {
		auto end = std::min(Node::MAX_DEGREE, root->key_cnt + keys_to_insert);
		for (size_t i = root->key_cnt; i < end; i++) {
			bool is_last_key_in_rule = i == keys_to_insert - 1;
			auto k = cookie.get_actual_key(rule);
			auto r_id = is_last_key_in_rule ? rule.second : BTree::INVALID_RULE;
			root->set_key(i, KeyInfo(k, r_id, BTree::INVALID_INDEX)); // Insert key
			root->set_dim(i, cookie.dimensio_order.at(cookie.level));
			cookie.level++;
		}
		root->set_key_cnt(end);
		if (cookie.required_more_levels()) {
			cookie.level++;
			auto end_next_level = root->get_next_layer(end - 1);
			auto nl = insert_to_root(end_next_level, rule, cookie);
			root->set_next_layer(end - 1, nl);
		}
		root->is_compressed = end > 1;
	}

	/*
	 * Allocate new node and insert the rule in it with an optional path compression
	 * */
	static Node * insert_in_to_new_node(const rule_spec_t & rule,
			InsertCookie & cookie) {
		auto k = cookie.get_actual_key(rule);
		// Allocate memory for root
		auto root = new Node;
		auto nl_cnt =
				BTree::PATH_COMPRESSION ?
						cookie.levels_required_cnt() - 1 : 0;
		if (nl_cnt > 0) {
			// compress all the keys in to this node
			// +1 because we also include this level
			insert_compressed(root, rule, cookie, nl_cnt + 1);
		} else {
			// insert the keys from the rule to compressed node
			root->set_key(0, KeyInfo(k, rule.second, BTree::INVALID_INDEX)); // Insert key
			root->set_key_cnt(1);
			if (cookie.required_more_levels()) {
				cookie.level++;
				auto nl = insert_to_root(root->get_next_layer(0), rule, cookie);
				root->set_next_layer(0, nl);
			}
		}
		return root;
	}

	/*
	 * @note the rule must not collide with anything in the tree
	 * 	(that also means that the value has to be unique)
	 * @param root root of tree where the rule should be inserted (can be nullptr)
	 * @param rule rule to insert
	 * @param cookie which stores the state of insertion
	 * @return new root of the tree
	 * */
	static Node * insert_to_root(Node * root, const rule_spec_t & rule,
			InsertCookie & cookie) {
		// If tree is empty
		if (root == nullptr) {
			root = insert_in_to_new_node(rule, cookie);
			return root;
		} else if (root->is_compressed) {
			insert_in_to_compressed(root, rule, cookie);
			return root;
		} else if (root->key_cnt == Node::MAX_DEGREE) {
			auto k = cookie.get_actual_key(rule);
			// If root is full, then tree grows in height
			// Allocate memory for new root
			Node *s = new Node;
			s->is_leaf = false;

			// Make old root as child of new root
			s->set_child(0, root);
			// Split the old root and move 1 key to the new root
			split_child(*s, 0, *root);

			// New root has two children now.  Decide which of the
			// two children is going to have new key
			//s->insert_non_full(rule, cookie);
			int i = 0;
			if (s->get_key(0) < k)
				i++;
			Node * c = s->child(i);
			insert_non_full(*c, rule, cookie);
			// Change root
			root = s;
		} else {
			// If root is not full, call insert_non_full for root
			insert_non_full(*root, rule, cookie);
		}
		return root;
	}

	/*
	 * if insert fails there is last node stored in cookie
	 * for reinsert
	 *
	 * */
	static void insert(BTree & tree, const rule_spec_t & rule,
			InsertCookie & cookie) {
		tree.root = insert(tree.root, rule, cookie);
	}
	/*
	 * Check if the inserting value is unique and perform the insert or the update
	 */
	static Node * insert(Node * root, const rule_spec_t & rule,
			InsertCookie & cookie) {
// search if there is some prefix already in decision tree because we do not
// want to duplicate keys which are already present
		std::vector<std::tuple<Node *, Node *, unsigned>> path;
		BTreeSearch<BTree>::search_path(root, cookie.dimensio_order, rule, path,
				cookie.level);
		cookie.level += path.size();
		if (path.size()) {
			auto nlr = cookie.levels_required_cnt();
			if (nlr == 0) {
				// rule prefix is already contained in tree
				// it is required only to update the rule id in specified node
				auto b = path.back();
				std::get<1>(b)->value[std::get<2>(b)] = rule.second;
				// [TODO] on_rewrite()
			} else {
				Node * r, *n;
				unsigned i;
				std::tie(r, n, i) = path.back();
				if (n->is_compressed) {
					_insert_in_to_compressed(n, rule, cookie, i + 1);
				} else {
					// else add next level
					// insert to the next layer
					Node * ins_root = n->get_next_layer(i);
					ins_root = insert_to_root(ins_root, rule, cookie);
					// update ptr on next layer in previous layer
					n->set_next_layer(i, ins_root);
				}
			}
		} else {
			root = insert_to_root(root, rule, cookie);
		}
		return root;
	}

	/* A utility function to split the child y of this node
	 * Note that y must be full when this function is called
	 * @param i index of key which should be transfered to this node
	 * @param y the child node
	 **/
	static void split_child(Node & node, unsigned i, Node & y) {
		// y - left
		// node - parent
		// z - right

		// Create a new node which is going to store (t-1) keys
		// of y
		Node *z = new Node;
		z->is_leaf = y.is_leaf;
		z->set_key_cnt(Node::MIN_DEGREE);

		// Copy the last (MIN_DEGREE-1) keys of y to z
		for (unsigned j = 0; j < Node::MIN_DEGREE; j++) {
			y.move_key(Node::MIN_DEGREE + j + 1, *z, j);
		}

		if (not y.is_leaf) {
			for (unsigned j = 0; j < Node::MIN_DEGREE + 1; j++) {
				// Copy the last t children of y to z
				auto ch = y.child(Node::MIN_DEGREE + j + 1);
				z->set_child(j, ch);
			}
		}

		// Subtract number of keys moved from y to z
		y.set_key_cnt(Node::MIN_DEGREE);

		assert(node.key_cnt < Node::MAX_DEGREE);
		// Since this node is going to have a new child,
		// create space of new child
		for (int j = node.key_cnt; j >= int(i + 1); j--) {
			node.move_child(j, node, j + 1);
		}

		// Link the new child to this node
		node.set_child(i + 1, z);

		// A key of y will move to this node. Find location of
		// new key and move all greater keys one space ahead
		for (int j = int(node.key_cnt) - 1; j >= int(i); j--)
			node.move_key(j, node, j + 1);

		// Copy the middle key of y to this node
		y.move_key(node.MIN_DEGREE, node, i);

		// Increment count of keys in this node
		node.set_key_cnt(node.key_cnt + 1);
	}

}
;

}
