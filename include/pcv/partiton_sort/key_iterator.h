#pragma once
#include <tuple>

namespace pcv {

template<typename Node, typename KeyInfo, typename key_value_t>
class _KeyIterator {
public:
	class State {
	public:
		Node * actual;
		unsigned index;

		bool operator!=(const State & other) const {
			return actual != other.actual or index != other.index;
		}
		void operator++() {
			std::tie(actual, index) = actual->getSucc_global(index);
		}
		void operator--() {
			std::tie(actual, index) = actual->getPred_global(index);
		}
		KeyInfo operator*() {
			return actual->template get_key<key_value_t>(index);
		}

	};

	State _end;
	State _begin;

	_KeyIterator(Node * start_node, unsigned start_index) {
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

	_KeyIterator(Node * root) :
			_KeyIterator(left_most(root), 0) {
	}

	constexpr State & end() {
		return _end;
	}

	constexpr State & begin() {
		return _begin;
	}
};

}
