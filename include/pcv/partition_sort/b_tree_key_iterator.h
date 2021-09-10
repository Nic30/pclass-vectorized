#pragma once

#include <tuple>
#include <pcv/partition_sort/b_tree_node_navigator.h>

namespace pcv {

template<typename BTree, typename KeyInfo>
class BTreeKeyIterator {
public:
	using Node = typename BTree::Node;
	class State: public _BTreeNodeNavigator<BTree> {
	public:
		Node *actual;
		unsigned index;

		State(BTree &_tree, Node *_actual, unsigned _index) :
				_BTreeNodeNavigator<BTree>(_tree), actual(_actual), index(
						_index) {
		}
		Node* left_most(Node *n) {
			while (this->child(*n, 0)) {
				n = this->child(*n, 0);
			}
			return n;
		}
		unsigned index_of_child(Node *p, Node *ch) {
			size_t i = 0;
			for (; p != nullptr and i <= p->key_cnt; i++) {
				if (this->child(*p, i) == ch)
					break;
			}
			return i;
		}
		Node* get_most_left(Node *n) {
			while (not n->is_leaf) {
				n = this->child(*n, 0);
			}
			return n;
		}

		std::pair<Node*, unsigned> get_most_left_after(Node *p, Node *ch) {
			while (p != nullptr) {
				// move up to a parent node
				size_t my_index = index_of_child(p, ch);
				if (my_index + 1 < p->key_cnt) {
					return {get_most_left(this->child(*p, my_index + 1)), 0};
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
		std::pair<Node*, unsigned> getSucc_global(Node &node, unsigned idx) {
			if (node.is_leaf) {
				if (int(idx) < int(node.key_cnt) - 1) {
					// move forward in this node
					return {&node, idx + 1};
				} else {
					Node *p = node.parent;
					Node *n = &node;
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
			} else if (this->child(node, idx + 1)) {
				// move on lowest in right sibling
				return {this->get_most_left(this->child(node, idx + 1)), 0};
			} else {
				return this->get_most_left_after(node.parent, &node);
			}
		}
		std::pair<Node*, unsigned> getPred_global(Node &node, unsigned idx) {
			if (node.is_leaf) {
				if (idx > 0) {
					// move backward in this node
					return {&node, idx - 1};
				} else {
					// move to parent and check if there is key on left
					// if there is not this is the 1st child and the value can be on the parent of the parent
					Node *p = node.parent, *n = &node;
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
				auto ch = this->child(node, idx);
				while (not ch->is_leaf) {
					ch = this->child(*ch, ch->key_cnt);
				}
				return {ch, ch->key_cnt - 1};
			}
		}
		bool operator!=(const State &other) const {
			return actual != other.actual or index != other.index;
		}
		bool operator==(const State &other) const {
			return actual == other.actual and index == other.index;
		}
		void operator++() {
			std::tie(actual, index) = getSucc_global(*actual, index);
		}
		void operator--() {
			std::tie(actual, index) = getPred_global(*actual, index);
		}
		KeyInfo operator*() {
			return actual->get_key(index);
		}
	};
	State _begin;
	State _end;

	BTreeKeyIterator(const BTreeKeyIterator &src) :
			BTreeKeyIterator(src._begin.tree, src._begin.actual, 0) {
	}
	BTreeKeyIterator(const BTreeKeyIterator &&src) :
			BTreeKeyIterator(src._begin.tree, src._begin.actual, 0) {
	}
	BTreeKeyIterator(BTree &t, Node *root) :
			BTreeKeyIterator(t, State(t, nullptr, 0).left_most(root), 0) {
	}
	BTreeKeyIterator(BTree &t, Node *start_node, unsigned start_index) :
			_begin(t, start_node, start_index), _end(t, nullptr, 0) {
	}

	void operator=(const BTreeKeyIterator &other) {
		assert(&this->_begin.tree == &other._begin.tree);
		//this->tree = other.tree;
		_begin.actual = other._begin.actual;
		_begin.index = other._begin.index;

		_end.actual = other._end.actual;
		_end.index = other._end.index;
	}

	constexpr State& end() {
		return _end;
	}

	constexpr State& begin() {
		return _begin;
	}

};

}
