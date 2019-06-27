#pragma once
#include <string>
#include <vector>
#include <tuple>

#include <pcv/common/range.h>
#include <pcv/partition_sort/b_tree_search.h>

namespace pcv {

template<typename Key_t, size_t _D, size_t _T, bool _PATH_COMPRESSION>
class BTreeInsert {
	using BTree = _BTree<Key_t, _D, _T, _PATH_COMPRESSION>;
	using BTreeSearch_t = BTreeSearch<Key_t, _D, _T, _PATH_COMPRESSION>;
public:
	using key_t = typename BTree::key_t;
	using Node = typename BTree::Node;
	using rule_spec_t = typename BTree::rule_spec_t;
	using index_t = typename BTree::index_t;
	using level_t = typename BTree::level_t;
	using KeyInfo = typename BTree::KeyInfo;

	/*
	 * Information about insert state
	 * */
	class InsertCookie {
		size_t total_levels_required_cnt(const rule_spec_t & rule_) const {
			// iterate from the end of the rule ordered by dimension_order and check
			// where is the last specified value for the field
			for (int i = int(dimension_order.size()) - 1; i >= 0; i--) {
				auto d = dimension_order[i];
				auto & k = rule_.first[d];

				if (not k.is_wildcard())
					return i + 1;
			}

			// for full wildcard we still need to store information about rule somewhere
			return 1;
		}
	public:
		std::array<level_t, BTree::D> & dimension_order;
		level_t level;
		const level_t requires_levels;
		const rule_spec_t & rule;

		InsertCookie(BTree & tree, const rule_spec_t & rule_) :
				dimension_order(tree.dimension_order), level(0), requires_levels(
						total_levels_required_cnt(rule_)), rule(rule_) {
			assert(requires_levels <= BTree::D);
		}
		inline Range1d<typename BTree::key_t> get_actual_key() const {
			auto d = dimension_order.at(level);
			return rule.first.at(d);
		}
		size_t additional_level_required_cnt() const {
			assert(level < requires_levels);
			return requires_levels - level - 1;
		}
		bool required_more_levels() const {
			return level + 1 < requires_levels;
		}
	};

	inline static void insert(BTree & tree, const rule_spec_t & rule) {
		InsertCookie cookie(tree, rule);
		tree.root = insert(tree.root, cookie);
		tree.root->integrity_check(cookie.dimension_order);
	}

	/*
	 * Check if the inserting value is unique and perform the insert or the update
	 */
	static Node * insert(Node * root, InsertCookie & cookie) {
		// search if there is some prefix already in decision tree because we do not
		// want to duplicate keys which are already present
		std::vector<std::tuple<Node *, Node *, level_t>> path;
		BTreeSearch_t::search_path(root, cookie.dimension_order, cookie.rule,
				path, cookie.level);

		if (path.size()) {
			cookie.level += path.size() - 1;
			// there was chain of values as they are in the rule, we need to continue on end of this chain
			if (cookie.required_more_levels()) {
				cookie.level++;
				// it is required to insert next layer to tree
				Node *r, *n;
				unsigned i;
				std::tie(r, n, i) = path.back();
				if (n->is_compressed) {
					// insert to last node in path if it is possible
					// +1 because index to count conversion
					_insert_in_to_compressed(n, cookie, i + 1);
				} else {
					// insert to the next layer
					Node * ins_root = n->get_next_layer(i);
					ins_root = insert_to_root(ins_root, cookie);
					// update ptr on next layer in previous layer
					n->set_next_layer(i, ins_root);
				}
			} else {
				// rule prefix is already contained in tree
				// it is required only to update the rule id in specified node
				auto b = path.back();
				std::get<1>(b)->value[std::get<2>(b)] = cookie.rule.second;
			}
		} else {
			// the value is not in this tree and we need to insert it
			root = insert_to_root(root, cookie);
		}
		return root;
	}

	/* A utility function to insert a new key in this node
	 * The assumption is, the node must be non-full when this
	 * function is called
	 * */
	static void insert_non_full(Node & node, InsertCookie & cookie) {
		// Initialise index as index of rightmost element
		int i = int(node.key_cnt) - 1;
		// If this is a leaf node
		auto k = cookie.get_actual_key();
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
			auto r_id =
					require_more_levels ?
							BTree::INVALID_RULE : cookie.rule.second.rule_id;
			auto p = require_more_levels ? 0 :  cookie.rule.second.priority;
			KeyInfo new_k(k, {p, r_id}, BTree::INVALID_INDEX);
			node.set_key(i + 1, new_k);
#ifndef NDEBUG
			node.set_dim(i + 1, cookie.dimension_order.at(cookie.level));
#endif
			node.set_key_cnt(node.key_cnt + 1);

			// continue on next layer if required
			if (require_more_levels) {
				cookie.level++;
				auto nl = node.get_next_layer(i + 1);
				nl = insert_to_root(nl, cookie);
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
			insert_non_full(*c, cookie);
		}
	}
	static void insert_in_to_compressed(Node * root, InsertCookie & cookie) {
		assert(root->is_compressed);
		assert(root->is_leaf);
		assert(root->key_cnt > 1);
		// search for the longest common prefix
		// keep_keys_cnt = index of first non matching item
		uint8_t keep_keys_cnt;
		for (keep_keys_cnt = 0; keep_keys_cnt < root->key_cnt;
				keep_keys_cnt++) {
#ifndef NDEBUG
			auto actual_d = root->get_dim(keep_keys_cnt);
			auto d = cookie.dimension_order.at(cookie.level);
			assert(actual_d == d);
#endif
			auto k = cookie.get_actual_key();
			auto _k = root->get_key(keep_keys_cnt);
			if (_k.key != k) {
				break;
			}
			cookie.level++;
		}
		_insert_in_to_compressed(root, cookie, keep_keys_cnt);
	}
	static Node * decompress_node(Node * node,
			uint8_t index_of_key_to_separate) {
		// if the key does not exists in the node new empty node is created and connected
		// as new layer behind the last item
		if (node->key_cnt <= index_of_key_to_separate) {
			assert(node->key_cnt == index_of_key_to_separate);
			auto nl = new Node;
			node->set_next_layer(index_of_key_to_separate - 1, nl);
			return nl;
		} else if (index_of_key_to_separate == 0) {
			if (node->key_cnt == 1) {
				// there is only this non matching item for same dimension, we will use this item
				// and the newly generated item from the rule to build a new b-tree there
				return node;
			}
			// the re is something behind the first non matching key, wee need to to extract it to new node
			auto nl = new Node;
			for (uint8_t i2 = 1; i2 < node->key_cnt; i2++) {
				nl->set_key(i2 - 1, node->get_key(i2));
				nl->set_dim(i2 - 1, node->get_dim(i2));
			}
			nl->set_key_cnt(node->key_cnt - 1);
			nl->is_compressed = nl->key_cnt > 1;

			node->set_next_layer(0, nl);
			node->set_key_cnt(1);
			node->is_compressed = false;
			return node;
		} else {
			// the first non matching key is surrounded by the other keys
			// this item has to be extracted to a separate node and the items behind has to be extracted
			// to other node

			// extract the segment behind the first non matching item
			Node * nnl = nullptr;
			auto keys_to_nnl_cnt = node->key_cnt - index_of_key_to_separate - 1;
			if (keys_to_nnl_cnt) {
				nnl = new Node;
				for (uint8_t i2 = 0; i2 < keys_to_nnl_cnt; i2++) {
					auto i = index_of_key_to_separate + i2 + 1;
					nnl->set_key(i2, node->get_key(i));
					nnl->set_dim(i2, node->get_dim(i));
				}
				nnl->set_key_cnt(node->key_cnt - index_of_key_to_separate - 1);
				nnl->is_compressed = nnl->key_cnt > 1;
			}
			// extract the non matching item
			auto nl = new Node;
			nl->set_key(0, node->get_key(index_of_key_to_separate));
#ifndef NDEBUG
			nl->set_dim(0, node->get_dim(index_of_key_to_separate));
#endif
			nl->set_key_cnt(1);

			// connect the layers together
			assert(
					node->get_next_layer(index_of_key_to_separate - 1)
							== nullptr);
			node->set_next_layer(index_of_key_to_separate - 1, nl);
			if (nnl) {
				assert(nl->get_next_layer(0) == nullptr);
				nl->set_next_layer(0, nnl);
			}
			node->set_key_cnt(index_of_key_to_separate);
			node->is_compressed = node->key_cnt > 1;

			return nl;
		}
	}
	/*
	 * @note the cookie.level is expected to be set on the level of the item after the last matching item
	 * @param keep_keys_cnt the number of the same keys in the node
	 * 	(same as the rule, that means that this keys should stay in this node as they are)
	 * */
	static void _insert_in_to_compressed(Node * root, InsertCookie & cookie,
			uint8_t keep_keys_cnt) {
		assert(root->is_compressed);
		assert(root->is_leaf);
		assert(keep_keys_cnt <= root->key_cnt);

		// decompress part of the tree if required
		// insert to last common prefix and optionally create new tree from this node

		if (keep_keys_cnt == root->key_cnt) {
			// the rule of exactly same value or same prefix is already stored
			// in tree
			// can not use required_more_levels() because level is already incremented
			bool required_more_levels = cookie.level < cookie.requires_levels;
			if (required_more_levels) {
				// continue insert on next level
				auto nl = root->get_next_layer(keep_keys_cnt - 1);
				nl = insert(nl, cookie);
				root->set_next_layer(keep_keys_cnt - 1, nl);
			} else {
				root->value[keep_keys_cnt - 1] = cookie.rule.second;
			}
			return;
		} else {
			auto n = decompress_node(root, keep_keys_cnt);
			n->integrity_check(cookie.dimension_order, nullptr, cookie.level);
			insert(n, cookie);
		}
	}

	/*
	 * Insert to a compressed node
	 *
	 * @note just a copy of the
	 * */
	static void insert_compressed(Node * root, InsertCookie & cookie,
			size_t keys_to_insert) {
		assert(keys_to_insert <= BTree::D);
		auto end = std::min(Node::MAX_DEGREE, root->key_cnt + keys_to_insert);
		for (size_t i = root->key_cnt; i < end; i++) {
			bool is_last_key_in_rule = i == keys_to_insert - 1;
			auto k = cookie.get_actual_key();
			auto r_id =
					is_last_key_in_rule ?
							cookie.rule.second.rule_id : BTree::INVALID_RULE;
			auto p = is_last_key_in_rule ? cookie.rule.second.priority : 0;
			KeyInfo new_k(k, { p, r_id }, BTree::INVALID_INDEX);
			root->set_key(i, new_k); // Insert key
			root->set_dim(i, cookie.dimension_order.at(cookie.level));
			bool last_it = i + 1 == end;
			if (!last_it)
				cookie.level++;
		}
		root->set_key_cnt(end);
		if (cookie.required_more_levels()) {
			cookie.level++;
			auto end_next_level = root->get_next_layer(end - 1);
			auto nl = insert_to_root(end_next_level, cookie);
			root->set_next_layer(end - 1, nl);
		}
		root->is_compressed = end > 1;
	}

	/*
	 * Allocate new node and insert the rule in it with an optional path compression
	 * */
	static Node * insert_in_to_new_node(InsertCookie & cookie) {
		auto k = cookie.get_actual_key();
		// Allocate memory for root
		auto root = new Node;

		auto nl_cnt =
				BTree::PATH_COMPRESSION and cookie.required_more_levels() ?
						cookie.additional_level_required_cnt() : 0;
		if (nl_cnt > 0) {
			// compress all the keys in to this node
			// +1 because we also include this level
			insert_compressed(root, cookie, nl_cnt + 1);
		} else {
			// insert the keys from the rule to compressed node
			root->set_key(0,
					KeyInfo(k, cookie.rule.second, BTree::INVALID_INDEX)); // Insert key
#ifndef NDEBUG
			root->set_dim(0, cookie.dimension_order[cookie.level]);
#endif
			root->set_key_cnt(1);
			if (cookie.required_more_levels()) {
				cookie.level++;
				auto nl = insert_to_root(root->get_next_layer(0), cookie);
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
	static Node * insert_to_root(Node * root, InsertCookie & cookie) {
		// If tree is empty
		if (root == nullptr) {
			root = insert_in_to_new_node(cookie);
			return root;
		} else if (root->is_compressed) {
			insert_in_to_compressed(root, cookie);
			return root;
		} else if (root->key_cnt == Node::MAX_DEGREE) {
			auto k = cookie.get_actual_key();
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
			insert_non_full(*c, cookie);
			// Change root
			root = s;
		} else {
			// If root is not full, call insert_non_full for root
			insert_non_full(*root, cookie);
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

};

}
