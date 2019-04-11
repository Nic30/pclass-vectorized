#pragma once
#include <iostream>
#include <immintrin.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <stdio.h>
#include <array>
#include <limits>
#include <set>
#include <tuple>
#include <sstream>

#include <pcv/common/range.h>
//#include <pcv/partiton_sort/mempool_mockup.h>
#include <pcv/partition_sort/b_tree_printer.h>
#include <pcv/partition_sort/key_info.h>
#include <pcv/partition_sort/mempool.h>

namespace pcv {

/*
 * The B-Tree with multidimensional key
 *
 * This is B-tree divide to several layers. Each layer performs the lookup in single dimension only.
 *
 * This tree uses path compression between the layers. This means if there is a path trough the tree which contains
 * only the nodes with a single items. All items on this paths are merged in to the single node with is_compressed flag set.
 * In order to reduce lookup speed an use SIMD efficiently. Note that the compressed node does not behaves like
 * b-tree node but just as a list of the ranges.
 *
 * @note that this is only implementation of the data structure. You can find the algorithms in b_tree_*.h files.
 *
 * @tparam Key_t the type of key which will be used by the nodes in the tree
 * @tparam _D maximal number of levels of the tree (number of fields in packet/dimensions)
 * @tparam _T parameter which specifies the number of items per node
 * @tparam _PATH_COMPRESSION is true the path compression is enabled
 * 		and some of the nodes may be compressed as described
 * */
template<typename Key_t, size_t _D, size_t _T = 4, bool _PATH_COMPRESSION = true>
class alignas(64) _BTree {
public:
	using rule_id_t = uint16_t;
	static constexpr size_t D = _D;
	using val_range_t = Range1d<Key_t>;
	using rule_spec_t = std::pair<std::array<val_range_t, D>, rule_id_t>;
	using value_t = Key_t;
	using index_t = uint16_t;
	using KeyInfo = _KeyInfo<value_t, index_t>;
	using val_vec_t = std::array<value_t, D>;

	static constexpr index_t INVALID_INDEX =
			std::numeric_limits<index_t>::max();
	static constexpr rule_id_t INVALID_RULE = INVALID_INDEX;
	static constexpr bool PATH_COMPRESSION = _PATH_COMPRESSION;

	/*
	 * The node of B-tree
	 * the array of the keys, items, and next level pointers together with additional info
	 * */
	class alignas(64) Node: public ObjectWithStaticMempool<Node, 65536, false> {
	public:
		// T is the parameter which describes the size of the B-tree node
		static constexpr size_t T = _T;
		// limit of number of items in the node
		static constexpr size_t MIN_DEGREE = T - 1;
		static constexpr size_t MAX_DEGREE = 2 * T - 1;
		// keys for the items in the node
		__m256i keys[2];
		static_assert(sizeof(Key_t)* MAX_DEGREE <= sizeof(__m256i));
		// 16*1B dimension index, only used for compressed nodes, the dimension order is same as in tree
		__m64 dim_index[2];
		// the value of rule
		std::array<index_t, MAX_DEGREE> value;
		// the pointers to the root of trees in next level of the tree
		std::array<index_t, MAX_DEGREE> next_level;
		// 9*2B child index
		std::array<index_t, MAX_DEGREE + 1> child_index;
		// if bit in mask is set the key is present
		uint32_t key_mask;
		// key_mask/key_cnt have same meaning
		uint8_t key_cnt;

		// if the node is leaf this means that it does not have any child on this level of this b-tree
		// however it still can have pointers to the trees on next level
		bool is_leaf;
		// if node is compressed it means that it contains also the keys from the lower levels of tree
		// this means that each item in this node uses key for different dimension (field)
		// and the search/insert/remove algorithm is different
		bool is_compressed;
		Node * parent;

		// the copy constructor is disabled in order to ensure there are not any unintended copies of this object
		Node(Node const&) = delete;
		Node& operator=(Node const&) = delete;

		Node() {
			assert(((uintptr_t ) this) % 64 == 0);
			// the initialisations which are not required
			keys[0] = keys[1] = _mm256_set1_epi32(
					std::numeric_limits<uint32_t>::max());
			dim_index[1] = dim_index[0] = _m_from_int64(
					std::numeric_limits<uint64_t>::max());
			std::fill(value.begin(), value.end(), INVALID_INDEX);
			clean_children();

			// the only required initialisations
			set_key_cnt(0);
			is_leaf = true;
			is_compressed = false;
			parent = nullptr;
		}
		inline void clean_children() {
			std::fill(next_level.begin(), next_level.end(), INVALID_INDEX);
			std::fill(child_index.begin(), child_index.end(), INVALID_INDEX);
		}

		inline uint8_t get_dim(uint8_t index) const {
			return reinterpret_cast<const uint8_t*>(&dim_index[0])[index];
		}
		inline void set_dim(uint8_t index, uint8_t val) {
			reinterpret_cast<uint8_t*>(&dim_index[0])[index] = val;
		}
		inline KeyInfo get_key(uint8_t index) const {
			assert(index < MAX_DEGREE);
			auto low = reinterpret_cast<const value_t*>(&keys[0])[index];
			auto high = reinterpret_cast<const value_t*>(&keys[1])[index];
			return {
				Range1d<value_t>(low, high),
				value[index],
				next_level[index]
			};
		}

		inline void set_key(uint8_t index, const KeyInfo & key_info) {
			assert(index < MAX_DEGREE);
			reinterpret_cast<value_t*>(&keys[0])[index] = key_info.key.low;
			reinterpret_cast<value_t*>(&keys[1])[index] = key_info.key.high;

			this->value[index] = key_info.value;
			this->next_level[index] = key_info.next_level;
		}

		/*
		 * Set pointer to child node
		 * */
		inline void set_child(unsigned index, Node * child) {
			if (child == nullptr)
				child_index[index] = INVALID_INDEX;
			else {
				child_index[index] = Node::_Mempool_t::getId(child);
				child->parent = this;
			}
		}

		/*
		 * Set pointer to next layer
		 * */
		inline void set_next_layer(unsigned index, Node * next_layer_root) {
			if (next_layer_root == nullptr)
				next_level[index] = INVALID_INDEX;
			else
				next_level[index] = Node::_Mempool_t::getId(next_layer_root);
		}

		/*
		 * Set key_cnt and also update key_mask
		 * */
		inline void set_key_cnt(size_t key_cnt) {
			assert(key_cnt <= MAX_DEGREE);
			this->key_cnt = key_cnt;
			key_mask = (1 << key_cnt) - 1;
		}

		// get node from mempool from its index
		static inline Node & by_index(const index_t index) {
			return *reinterpret_cast<Node*>(Node::_Mempool_t::getById(index));
		}

		// get child node on specified index
		inline Node * child(const index_t index) {
			return const_cast<Node*>(const_cast<const Node *>(this)->child(
					index));
		}
		inline const Node * child(const index_t index) const {
			if (child_index[index] == INVALID_INDEX)
				return nullptr;
			else
				return &by_index(child_index[index]);
		}

		// get root node of the next layer starting from this node on specified index
		inline Node * get_next_layer(unsigned index) {
			return const_cast<Node*>(const_cast<const Node *>(this)->get_next_layer(
					index));
		}
		/*
		 * @return the pointer on root of the next layer in the tree
		 * */
		inline const Node * get_next_layer(unsigned index) const {
			auto i = next_level[index];
			if (i == INVALID_INDEX)
				return nullptr;
			else
				return &by_index(i);
		}

		/*
		 * @return number of the keys in this node and all subtrees
		 * */
		size_t size() const {
			size_t s = key_cnt;
			for (size_t i = 0; i < key_cnt + 1u; i++) {
				auto ch = child(i);
				if (ch)
					s += ch->size();
			}
			for (size_t i = 0; i < key_cnt; i++) {
				auto nl = get_next_layer(i);
				if (nl)
					s += nl->size();
			}
			return s;
		}

		/*
		 * Move key, value and next level pointer between two places
		 * @note this node is source
		 * */
		inline void move_key(uint8_t src_i, Node & dst, uint8_t dst_i) {
			dst.set_key(dst_i, this->get_key(src_i));
		}

		/*
		 * Copy keys, child pointers etc between the nodes
		 *
		 * @note this node is source
		 * */
		inline void transfer_items(unsigned src_start, Node & dst,
				unsigned dst_start, unsigned key_cnt) {
			// Copying the keys from
			auto end = src_start + key_cnt;
			for (unsigned i = src_start; i < end; ++i)
				move_key(i, dst, i + dst_start);

			// Copying the child pointers
			if (not dst.is_leaf) {
				for (unsigned i = src_start; i <= end; ++i) {
					auto ch = child(i);
					dst.set_child(i + dst_start, ch);
					//dst.child_index[i + dst_start] = src.child_index[i];
				}
			}
		}

		/*
		 * Shift block of keys to position -1
		 * */
		inline void shift_items_on_right_to_left(uint8_t start) {
			for (unsigned i = start + 1; i < key_cnt; ++i)
				move_key(i, *this, i - 1);

			for (unsigned i = start + 2; i <= key_cnt; ++i) {
				child_index[i - 1] = child_index[i];
			}
		}

		/*
		 * Check if the pointers in node are valid (recursively)
		 * */
		inline void integrity_check(const std::array<int, D> & dimesion_order,
				std::set<Node*> * seen = nullptr, size_t level = 0) {
			std::set<Node*> _seen;
			if (seen == nullptr)
				seen = &_seen;
			assert(seen->find(this) == seen->end());
			assert(key_cnt > 0);
			if (is_compressed) {
				assert(is_leaf);
				assert(key_cnt > 1);
				for (size_t i = 0; i < key_cnt; i++) {
					auto d = dimesion_order[level + i];
					auto actual_d = get_dim(i);
					assert(d == actual_d);
				}
			}
			for (size_t i = 0; i <= key_cnt; i++) {
				auto ch = child(i);
				if (ch) {
					assert(ch->parent == this);
					ch->integrity_check(dimesion_order, seen, level);
				}
			}
			for (size_t i = 0; i < key_cnt; i++) {
				get_key(i);
				auto nl = get_next_layer(i);
				if (nl) {
					assert(nl->parent == nullptr);
					nl->integrity_check(dimesion_order, seen, level + 1);
				}
			}
		}

		/*
		 * @attention will delete the sub-tree recursively
		 * */
		~Node() {
			if (not is_leaf) {
				for (uint8_t i = 0; i < key_cnt + 1; i++) {
					delete child(i);
				}
			}
			for (uint8_t i = 0; i < key_cnt; i++) {
				delete get_next_layer(i);
			}
		}
	}__attribute__((aligned(64)));

	Node * root;
	std::array<int, D> dimension_order;

	// the copy constructor is disabled in order to ensure there are not any unintended copies of this object
	_BTree(_BTree const&) = delete;
	_BTree& operator=(_BTree const&) = delete;

	_BTree() :
			root(nullptr) {
		for (size_t i = 0; i < dimension_order.size(); i++)
			dimension_order[i] = i;
	}

	// get number of keys stored on all levels in tree (!= number of stored rules)
	size_t size() const {
		if (root)
			return root->size();
		else
			return 0;
	}

	// serialize graph to string in dot format
	friend std::ostream & operator<<(std::ostream & str, const _BTree & t) {
		return BTreePrinter<_BTree>::print_top(str, t);
	}
	operator std::string() const {
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}

	~_BTree() {
		delete root;
		root = nullptr;
	}

}__attribute__((aligned(64)));

}
