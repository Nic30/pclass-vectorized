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
	public:
		std::array<int, BTree::D> & dimensio_order;
		uint8_t level;

		InsertCookie(BTree & tree) :
				dimensio_order(tree.dimension_order), level(0) {
		}
		Range1d<typename BTree::value_t> get_actual_key(
				const rule_spec_t & rule) const {
			auto d = dimensio_order.at(level);
			return rule.first.at(d);
		}
		bool required_more_levels(const rule_spec_t & rule) const {
			if (level < dimensio_order.size()) {
				for (size_t i = level + 1; i < dimensio_order.size(); i++) {
					auto d = dimensio_order[i];
					auto & k = rule.first[d];
					if (not k.is_wildcard())
						return true;
				}
			}
			return false;
		}
	};

	inline static void insert(BTree & tree, const rule_spec_t & rule) {
		InsertCookie cookie(tree);
		insert(tree, rule, cookie);
	}

	/* A utility function to insert a new key in this node
	 * The assumption is, the node must be non-full when this
	 * function is called
	 * */
	static void insertNonFull(Node & node, const rule_spec_t & rule,
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
			node.set_key(i + 1, KeyInfo(k, rule.second, BTree::INVALID_INDEX));
			node.set_key_cnt(node.key_cnt + 1);

			// continue on next layer if required
			if (cookie.required_more_levels(rule)) {
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
				splitChild(node, i, *node.child(i));

				// After split, the middle key of C[i] goes up and
				// C[i] is splitted into two. See which of the two
				// is going to have the new key
				if (node.get_key(i) < k)
					i++;
			}
			assert(i >= 0);
			Node * c = node.child(i);
			insertNonFull(*c, rule, cookie);
		}
	}

	/*
	 * @note the rule must not collide with anything in the tree
	 * @param root root of tree where the rule should be inserted (can be nullptr)
	 * @param rule rule to insert
	 * @param cookie which stores the state of insertion
	 * @return new root of the tree
	 * */
	static Node * insert_to_root(Node * root, const rule_spec_t & rule,
			InsertCookie & cookie) {
		auto k = cookie.get_actual_key(rule);
		// If tree is empty
		if (root == nullptr) {
			// Allocate memory for root
			root = new Node;
			root->set_key(0, KeyInfo(k, rule.second, BTree::INVALID_INDEX)); // Insert key
			root->set_key_cnt(1);
			root->value[0] = rule.second;
			if (cookie.required_more_levels(rule)) {
				cookie.level++;
				auto nl = insert_to_root(root->get_next_layer(0), rule, cookie);
				root->set_next_layer(0, nl);
			}
		} else if (root->key_cnt == Node::MAX_DEGREE) {
			// If root is full, then tree grows in height
			// Allocate memory for new root
			Node *s = new Node;
			s->is_leaf = false;

			// Make old root as child of new root
			s->set_child(0, root);
			// Split the old root and move 1 key to the new root
			splitChild(*s, 0, *root);

			// New root has two children now.  Decide which of the
			// two children is going to have new key
			//s->insertNonFull(rule, cookie);
			int i = 0;
			if (s->get_key(0) < k)
				i++;
			Node * c = s->child(i);
			insertNonFull(*c, rule, cookie);
			// Change root
			root = s;
		} else {
			// If root is not full, call insertNonFull for root
			insertNonFull(*root, rule, cookie);
		}
		return root;
	}
	/*
	 * if insert fails there is last node stored in cookie
	 * for reinsert
	 * */
	static void insert(BTree & tree, const rule_spec_t & rule,
			InsertCookie & cookie) {
		// search if there is some prefix already in decision tree because we do not
		// want to duplicate keys which are already present
		std::vector<std::tuple<Node *, Node *, unsigned>> path;
		BTreeSearch<BTree>::search_path(tree, rule, path);
		cookie.level += path.size();
		if (cookie.level == BTree::D) {
			// rule of same specification is already contained in tree
			// it is required only to update the rule id in specified node
			auto b = path.back();
			std::get<1>(b)->value[std::get<2>(b)] = rule.second;
			// [TODO] on_rewrite()
		} else if (cookie.level > 0) {
			Node * r, *n;
			unsigned i;
			std::tie(r, n, i) = path.back();

			// insert to the next layer
			Node * ins_root = n->get_next_layer(i);
			ins_root = insert_to_root(ins_root, rule, cookie);
			// update ptr on next layer in previous layer
			n->set_next_layer(i, ins_root);
		} else {
			tree.root = insert_to_root(tree.root, rule, cookie);
		}
	}

	/* A utility function to split the child y of this node
	 * Note that y must be full when this function is called
	 * @param i index of key which should be transfered to this node
	 * @param y the child node
	 **/
	static void splitChild(Node & node, unsigned i, Node & y) {
		// y - left
		// this - parent
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
			auto ch = y.child(j);
			z->set_child(j + 1, ch);
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
