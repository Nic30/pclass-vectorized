#pragma once
#include <pcv/partiton_sort/b_tree.h>

namespace pcv {

/*
 * If the searched value is in range it means that the search in B-tree is finished
 * and the val_index is index of item in this node where the search ended
 * Otherwise the val_index is index of the children node which should be searched
 **/
class SearchResult {
public:
	unsigned val_index;
	bool in_range;
	SearchResult() :
			val_index(-1), in_range(false) {
	}
};

SearchResult search_seq(const BTree::Node & node, BTree::value_t val);
SearchResult search_seq(const BTree::Node & node,
		const Range1d<BTree::value_t> val);
/*
 * Search the value or range in single level of the tree
 *
 * @return pair<node where key resides, index of the key in node>
 * */
template<typename value_t>
std::pair<BTree::Node*, unsigned> search_possition_1d(BTree::Node * n,
		const value_t val) {
	while (n) {
		//auto s = search_avx2(*n, val);
		auto s = search_seq(*n, val);
		if (s.in_range) {
			return {n, s.val_index};
		} else if (n->is_leaf) {
			return {nullptr, 0};
		} else {
			n = n->child(s.val_index);
		}
	}
	return {nullptr, 0};
}

template<typename BTree>
class BTreeSearch {
public:
	using Node = typename BTree::Node;
	using value_t = typename BTree::value_t;
	using KeyInfo = typename BTree::KeyInfo;

	class KeyIterator {
	public:
		class State {
		public:
			Node * actual;
			unsigned index;

			bool operator!=(const State & other) const {
				return actual != other.actual or index != other.index;
			}
			void operator++() {
				std::tie(actual, index) = getSucc_global(*actual, index);
			}
			void operator--() {
				std::tie(actual, index) = getPred_global(*actual, index);
			}
			KeyInfo operator*() {
				return actual->template get_key<value_t>(index);
			}

		};

		State _end;
		State _begin;

		KeyIterator(Node * start_node, unsigned start_index) {
			_begin.actual = start_node;
			_begin.index = start_index;

			_end.actual = nullptr;
			_end.index = 0;
		}

		static Node * left_most(Node * n) {
			while (n->child(0)) {
				n = n->child(0);
			}
			return n;
		}

		KeyIterator(Node * root) :
				KeyIterator(left_most(root), 0) {
		}

		constexpr State & end() {
			return _end;
		}

		constexpr State & begin() {
			return _begin;
		}
	};

	/*
	 * Search in all levels of the tree
	 *
	 * [TODO] use array
	 * */
	static typename BTree::rule_id_t search(BTree & t,
			const std::vector<typename BTree::value_t> & _val) {
		typename BTree::rule_id_t res = BTree::INVALID_RULE;
		Node * n = t.root;
		unsigned i = 0;
		while (n) {
			auto d = t.dimension_order[i];
			auto val = _val[d];
			auto r = search_possition_1d(n, val);
			n = r.first;
			if (n) {
				auto v = n->value[r.second];
				if (v != BTree::INVALID_RULE) {
					// some matching rule found on path from the root in this node
					res = v;
					// search in next layer if there is some
					n = n->get_next_layer(r.second);
					i++;
				}
			}
		}
		return res;
	}
	/*
	 * Search the tuples <node, index in node> for each part of the rule
	 * @note the items are in order specified by dimension_order array
	 **/
	static void search_path(BTree & t, const typename BTree::rule_spec_t & rule,
			std::vector<std::pair<Node *, unsigned>> & path) {
		Node * n = t.root;
		unsigned i = 0;
		while (n) {
			auto d = t.dimension_order[i];
			auto val = rule.first[d];
			auto r = search_possition_1d(n, val);
			if (r.first == nullptr)
				break;
			// some matching rule found on path from the root in this node
			// search in next layer if there is some
			path.push_back( { r.first, r.second });
			n = r.first->get_next_layer(r.second);
			i++;
		}
	}

	static unsigned index_of_child(Node *p, Node * ch) {
		size_t i = 0;
		for (; p != nullptr and i <= p->key_cnt; i++) {
			if (p->child(i) == ch)
				break;
		}
		return i;
	}

	static std::pair<Node*, unsigned> get_most_left(Node * n) {
		while (not n->is_leaf) {
			n = n->child(0);
		}
		return {n, 0};
	}

	static std::pair<Node*, unsigned> get_most_left_after(Node * p, Node * ch) {
		while (p != nullptr) {
			// move up to a parent node
			size_t my_index = index_of_child(p, ch);
			if (my_index + 1 < p->key_cnt) {
				return get_most_left(p->child(my_index + 1));
			} else {
				ch = p;
				p = p->parent;
			}
		}
		return {nullptr, 0};
	}

	// A function to get position of successor of keys[idx]
	// @return next node and next index of key in it
	// 		if there is no key after this one returns {nullptr, 0};
	static std::pair<Node*, unsigned> getSucc_global(Node & node,
			unsigned idx) {
		if (node.is_leaf) {
			if (int(idx) < int(node.key_cnt) - 1) {
				// move forward in this node
				return {&node, idx + 1};
			} else {
				Node * p = node.parent;
				Node * n = &node;
				while (p) {
					auto i = index_of_child(p, n);
					if (i == p->key_cnt) {
						// this is on end of parent and successor is somewhere up
						n = p;
						p = p->parent;
					} else {
						// just move to parent key
						return {p, i};
					}
				}
				return {nullptr, 0};
			}
		} else if (node.child(idx + 1)) {
			// move on lowest in right sibling
			return get_most_left(node.child(idx + 1));
		} else {
			return get_most_left_after(node.parent, &node);
		}
	}
	static std::pair<Node*, unsigned> getPred_global(Node & node,
			unsigned idx) {
		if (node.is_leaf) {
			if (idx > 0) {
				// move backward in this node
				return {&node, idx - 1};
			} else {
				// move to parent and check if there is key on left
				// if there is not this is the 1st child and the value can be on the parent of the parent
				Node * p = node.parent, *n = &node;
				while (p) {
					size_t i = index_of_child(p, n);
					if (i > 0) {
						// key in parent before the keys of this node
						return {p, i - 1};
					}
					// go one level up
					n = p;
					p = p->parent;
				}

				assert(p == nullptr);
				// we are on top on first item there is nowhere to go
				return {nullptr, 0};
			}
		} else {
			// in internal node
			// there has to be previous node because otherwise this would be a leaf node
			// move to children on the bootom left

			// the indexing of children is shifted against the indexing of the keys by -1
			auto ch = node.child(idx);
			while (not ch->is_leaf) {
				ch = ch->child(ch->key_cnt);
			}
			return {ch, ch->key_cnt - 1};
		}
	}

	// A function to get predecessor of keys[idx] (only in the local subtree)
	static typename BTree::KeyInfo getPred(Node & node, unsigned idx) {
		// Keep moving to the right most node until we reach a leaf
		auto cur = node.child(idx);
		while (!cur->is_leaf)
			cur = cur->child(cur->key_cnt);

		// Return the last key of the leaf
		return cur->template get_key<typename BTree::value_t>(cur->key_cnt - 1);
	}
	// A function to get successor of keys[idx] (only in the local subtree)
	static typename BTree::KeyInfo getSucc(Node & node, unsigned idx) {
		// Keep moving the left most node starting from child(idx+1) until we reach a leaf
		auto * cur = node.child(idx + 1);
		while (not cur->is_leaf)
			cur = cur->child(0);

		// Return the first key of the leaf
		return cur->template get_key<typename BTree::value_t>(0);
	}
	static KeyIterator iter_keys(BTree & tree) {
		return KeyIterator(tree.root);
	}
	static KeyIterator iter_keys(Node * start, unsigned start_i) {
		return KeyIterator(start, start_i);
	}
	// Find index of the key in this node
	static unsigned findKey(Node & node, const Range1d<value_t> k) {
		unsigned i = 0;
		while (i < node.key_cnt && node.template get_key<value_t>(i) < k)
			++i;
		return i;
	}
};

}
