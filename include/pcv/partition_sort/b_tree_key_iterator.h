#pragma once
#include <tuple>

namespace pcv {

template<typename Node, typename KeyInfo>
class BTreeKeyIterator {
public:
	class State {
	public:
		Node * actual;
		unsigned index;

		bool operator!=(const State & other) const {
			return actual != other.actual or index != other.index;
		}
		bool operator==(const State & other) const {
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

	State _end;
	State _begin;

	BTreeKeyIterator(Node * root) :
			BTreeKeyIterator(left_most(root), 0) {
	}
	BTreeKeyIterator(Node * start_node, unsigned start_index) {
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

	constexpr State & end() {
		return _end;
	}

	constexpr State & begin() {
		return _begin;
	}
	static unsigned index_of_child(Node *p, Node * ch) {
		size_t i = 0;
		for (; p != nullptr and i <= p->key_cnt; i++) {
			if (p->child(i) == ch)
				break;
		}
		return i;
	}
	static Node* get_most_left(Node * n) {
		while (not n->is_leaf) {
			n = n->child(0);
		}
		return n;
	}

	static std::pair<Node*, unsigned> get_most_left_after(Node * p, Node * ch) {
		while (p != nullptr) {
			// move up to a parent node
			size_t my_index = index_of_child(p, ch);
			if (my_index + 1 < p->key_cnt) {
				return {get_most_left(p->child(my_index + 1)), 0};
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
			return {get_most_left(node.child(idx + 1)), 0};
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

};

}
