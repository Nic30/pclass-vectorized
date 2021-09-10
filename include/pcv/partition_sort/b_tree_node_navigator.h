#pragma once
#include <array>
#include <set>
#include <assert.h>

namespace pcv {

template<typename BTree>
class _BTreeNodeNavigator {
public:
	BTree &tree;
	using index_t = typename BTree::index_t;
	using level_t = typename BTree::level_t;
	using Node = typename BTree::Node;
	static constexpr size_t MIN_DEGREE = Node::MIN_DEGREE;
	static constexpr size_t MAX_DEGREE = Node::MAX_DEGREE;
	static constexpr index_t INVALID_INDEX = Node::INVALID_INDEX;
	static constexpr size_t D = BTree::D;

	_BTreeNodeNavigator(BTree &_t) :
			tree(_t) {
	}
	/*
	 * Set pointer to child node
	 * */
	inline void set_child(Node &n, unsigned index, Node *child) {
		if (child == nullptr)
			n.child_index[index] = INVALID_INDEX;
		else {
			n.child_index[index] = tree.node_allocator.getId(child);
			child->parent = &n;
		}
	}

	/*
	 * Set pointer to next layer
	 * */
	inline void set_next_layer(Node &n, unsigned index, Node *next_layer_root) {
		if (next_layer_root == nullptr)
			n.next_level[index] = INVALID_INDEX;
		else
			n.next_level[index] = tree.node_allocator.getId(next_layer_root);
	}

	// get node from mempool from its index
	inline Node& by_index(const index_t index) {
		return *reinterpret_cast<Node*>(tree.node_allocator.getById(index));
	}

	inline const Node* child_const(const Node &n, index_t index) const {
		if (n.child_index[index] == INVALID_INDEX)
			return nullptr;
		else
			return &const_cast<_BTreeNodeNavigator*>(this)->by_index(
					n.child_index[index]);
	}

	// get child node on specified index
	inline Node* child(Node &n, const index_t index) {
		return const_cast<Node*>(const_cast<_BTreeNodeNavigator<BTree>*>(this)->child_const(
				n, index));
	}

	/*
	 * @return the pointer on root of the next layer in the tree
	 * */
	inline const Node* get_next_layer_const(const Node &n,
			unsigned index) const {
		auto i = n.next_level[index];
		if (i == INVALID_INDEX)
			return nullptr;
		else
			return &const_cast<_BTreeNodeNavigator*>(this)->by_index(i);
	}
	// get root node of the next layer starting from this node on specified index

	inline Node* get_next_layer(Node &n, unsigned index) {
		return const_cast<Node*>(get_next_layer_const(n, index));
	}

	inline void move_child(Node &n, uint8_t src_i, Node &dst, uint8_t dst_i) {
		set_child(dst, dst_i, this->child(n, src_i));
	}

	/*
	 * @return number of the keys in this node and all subtrees
	 * */
	size_t size(const Node &n) const {
		size_t s = n.key_cnt;
		for (size_t i = 0; i < n.key_cnt + 1u; i++) {
			auto ch = child_const(n, i);
			if (ch)
				s += size(*ch);
		}
		for (size_t i = 0; i < n.key_cnt; i++) {
			auto nl = this->get_next_layer_const(n, i);
			if (nl)
				s += size(*nl);
		}
		return s;
	}

	/*
	 * Copy keys, child pointers etc between the nodes
	 *
	 * @note this node is source
	 * */
	inline void transfer_items(Node &src, unsigned src_start, Node &dst,
			unsigned dst_start, unsigned key_cnt_) {
		// Copying the keys from
		auto end = src_start + key_cnt_;
		for (unsigned i = src_start; i < end; ++i)
			src.move_key(i, dst, i + dst_start);

		// Copying the child pointers
		if (not dst.is_leaf) {
			for (unsigned i = src_start; i <= end; ++i) {
				auto ch = child(src, i);
				set_child(dst, i + dst_start, ch);
				//dst.child_index[i + dst_start] = src.child_index[i];
			}
		}
	}

	/*
	 * Check if the pointers in node are valid (recursively)
	 * */
#ifndef NDEBUG

	void integrity_check(Node &n, const std::array<level_t, D> &dimension_order,
			std::set<Node*> *seen = nullptr, size_t level = 0) {
		std::set<Node*> _seen;
		if (seen == nullptr)
			seen = &_seen;
		assert(seen->find(&n) == seen->end());
		assert(n.key_cnt > 0);
		assert(n.key_cnt <= MAX_DEGREE);
		if (n.is_compressed) {
			assert(n.is_leaf);
			assert(n.key_cnt > 1);
			for (size_t i = 0; i < n.key_cnt; i++) {
				auto d = dimension_order.at(level + i);
				auto actual_d = n.get_dim(i);
				assert(d == actual_d);
			}
		}
		if (!n.is_leaf) {
			assert(!n.is_compressed);
			for (size_t i = 0; i <= n.key_cnt; i++) {
				auto ch = child(n, i);
				assert(ch);
				assert(ch->parent == &n);
				integrity_check(*ch, dimension_order, seen, level);
			}
		}
		for (size_t i = 0; i < n.key_cnt; i++) {
			n.get_key(i);
			auto nl = get_next_layer(n, i);
			if (nl) {
				assert(nl->parent == nullptr);
				size_t l = level + 1;
				if (n.is_compressed)
					l += i;
				integrity_check(*nl, dimension_order, seen, l);
			}
		}
	}
#endif

};

}
