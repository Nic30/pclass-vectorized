#pragma once

#include <array>
#include <set>
#include <assert.h>


namespace pcv {
/*
 * The node of B-tree
 * the array of the keys, items, and next level pointers together with additional info
 *
 * @tparam cfg the class derived from pcv::_BTreeCfg which has parameters
 * 			   for specification of maximum sizes and other alg. configs
 * */
template<typename cfg, typename level_t, typename rule_value_t,
		typename index_t, typename KeyInfo, typename key_range_t>
class alignas(64) _BTreeNode {
public:
	// T is the parameter which describes the size of the B-tree node
	static constexpr size_t T = cfg::T;
	// limit of number of items in the node
	static constexpr size_t MIN_DEGREE = T - 1;
	static constexpr size_t MAX_DEGREE = 2 * T - 1;
	static constexpr index_t INVALID_INDEX = std::numeric_limits < index_t
			> ::max();

	// keys for the items in the node
	__m256i keys[2];
	static_assert(sizeof(typename cfg::Key_t)* MAX_DEGREE <= sizeof(__m256i));
	// 16*2B dimension index, only used for compressed nodes, the dimension order is same as in tree
	std::array<level_t, 16> dim_index;
	// the value of rule
	std::array<rule_value_t, MAX_DEGREE> value;
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
	_BTreeNode *parent;

	// the copy constructor is disabled in order to ensure there are not any unintended copies of this object
	_BTreeNode(_BTreeNode const&) = delete;
	_BTreeNode& operator=(_BTreeNode const&) = delete;

	_BTreeNode() {
		assert(((uintptr_t ) this) % 64 == 0);
		// the initializations which are not required
		keys[0] = keys[1] = _mm256_set1_epi32(
				std::numeric_limits<uint32_t>::max());
		std::fill(next_level.begin(), next_level.end(), 0);
		clean_children();

		// the only required initializations
		set_key_cnt(0);
		is_leaf = true;
		is_compressed = false;
		parent = nullptr;
	}

	inline void clean_children() {
		std::fill(next_level.begin(), next_level.end(), INVALID_INDEX);
		std::fill(child_index.begin(), child_index.end(), INVALID_INDEX);
	}

	inline level_t get_dim(uint8_t index) const {
		return reinterpret_cast<const level_t*>(&dim_index[0])[index];
	}
	inline void set_dim(uint8_t index, level_t val) {
		reinterpret_cast<level_t*>(&dim_index[0])[index] = val;
	}
	inline KeyInfo get_key(uint8_t index) const {
		assert(index < MAX_DEGREE);
		auto k = reinterpret_cast<const key_range_t*>(&keys[0])[index];
		return {
			k,
			value[index],
			next_level[index]
		};
	}

	inline void set_key(uint8_t index, const KeyInfo &key_info) {
		assert(index < MAX_DEGREE);
		reinterpret_cast<key_range_t*>(&keys[0])[index] = key_info.key;
		this->value[index] = key_info.value;
		this->next_level[index] = key_info.next_level;
	}

	/*
	 * Set key_cnt and also update key_mask
	 * */
	inline void set_key_cnt(size_t key_cnt_) {
		assert(key_cnt_ <= MAX_DEGREE);
		this->key_cnt = key_cnt_;
		key_mask = (1 << key_cnt_) - 1;
	}

	/*
	 * Move key, value and next level pointer between two places
	 * @note this node is source
	 * */
	inline void move_key(uint8_t src_i, _BTreeNode &dst, uint8_t dst_i) {
		dst.set_key(dst_i, this->get_key(src_i));
#ifndef NDEBUG
		dst.set_dim(dst_i, this->get_dim(src_i));
#endif
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

}__attribute__((aligned(64)));

template<typename cfg, typename level_t, typename rule_value_t,
		typename index_t, typename KeyInfo, typename key_range_t>
constexpr size_t _BTreeNode<cfg, level_t, rule_value_t, index_t, KeyInfo,
		key_range_t>::T;

template<typename cfg, typename level_t, typename rule_value_t,
		typename index_t, typename KeyInfo, typename key_range_t>
constexpr size_t _BTreeNode<cfg, level_t, rule_value_t, index_t, KeyInfo,
		key_range_t>::MIN_DEGREE;

template<typename cfg, typename level_t, typename rule_value_t,
		typename index_t, typename KeyInfo, typename key_range_t>
constexpr size_t _BTreeNode<cfg, level_t, rule_value_t, index_t, KeyInfo,
		key_range_t>::MAX_DEGREE;

template<typename cfg, typename level_t, typename rule_value_t,
		typename index_t, typename KeyInfo, typename key_range_t>
constexpr index_t _BTreeNode<cfg, level_t, rule_value_t, index_t, KeyInfo,
		key_range_t>::INVALID_INDEX;

}
