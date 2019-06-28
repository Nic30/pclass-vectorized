#pragma once
#include <array>
#include <vector>
#include <assert.h>
#include <byteswap.h>
#include <immintrin.h>

#include <pcv/common/range.h>
#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_key_iterator.h>

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

namespace search_fn {
/*
 * 8*32b a > b
 *
 * @param a the signed integer vector
 * @param b the unsigned integer vector
 * */
inline static __m256i __attribute__((__always_inline__)) _mm256_cmpgt_epu32(
		__m256i const a, __m256i const b) {
	constexpr uint32_t offset = 0x1 << 31;
	__m256i const fix_val = _mm256_set1_epi32(offset);
	return _mm256_cmpgt_epi32(_mm256_add_epi32(a, fix_val), b); // PCMPGTD
}
/*
 * 16*16b a > b
 *
 * @param a the signed integer vector
 * @param b the unsigned integer vector
 * */
inline static __m256i __attribute__((__always_inline__)) _mm256_cmpgt_epu16(
		__m256i const a, __m256i const b) {
	constexpr uint16_t offset = 0x1u << 15;
	__m256i const fix_val = _mm256_set1_epi16(offset);
	return _mm256_cmpgt_epi16(_mm256_add_epi16(a, fix_val), b); // PCMPGTD
}

template<typename T>
SearchResult search_seq(const __m256i * keys, const uint8_t key_cnt,
		const Range1d<T> & val) {
	SearchResult r;
	for (r.val_index = 0; r.val_index < key_cnt; r.val_index++) {
		auto low = reinterpret_cast<const T*>(&keys[0])[r.val_index];
		auto high = reinterpret_cast<const T*>(&keys[1])[r.val_index];
		auto cur = Range1d<T>(low, high);
		if (val < cur.low) {
			break;
		} else if (cur == val) {
			r.in_range = true;
			break;
		}
	}
	return r;
}

template<typename T>
SearchResult search_seq(const __m256i * keys, const uint8_t key_cnt, T val) {
	SearchResult r;
	for (r.val_index = 0; r.val_index < key_cnt; r.val_index++) {
		auto low = reinterpret_cast<const T*>(&keys[0])[r.val_index];
		if (val < low) {
			break;
		} else {
			auto high = reinterpret_cast<const T*>(&keys[1])[r.val_index];
			auto cur = Range1d<T>(low, high);
			if (cur.in_range(val)) {
				r.in_range = true;
				break;
			}
		}
	}
	return r;
}

template<typename T>
SearchResult search_avx2(const __m256i * keys, T val);


}

template<typename _Key_t, size_t _D, size_t _T, bool _PATH_COMPRESSION>
class BTreeSearch {
	using BTree = _BTree<_Key_t, _D, _T, _PATH_COMPRESSION>;
public:
	using Node = typename BTree::Node;
	using key_t = typename BTree::key_t;
	using rule_spec_t = typename BTree::rule_spec_t;
	using KeyInfo = typename BTree::KeyInfo;
	using key_vec_t = typename BTree::key_vec_t;
	using level_t = typename BTree::level_t;
	using rule_value_t = typename BTree::rule_value_t;

	// the position of the data in input packet
	struct in_packet_position_t {
		uint16_t offset;
		uint8_t size; // currently only 1 or 2
		uint8_t is_big_endian;

		in_packet_position_t() :
				offset(0), size(0), is_big_endian(0) {
		}
		in_packet_position_t(uint16_t _offset, uint8_t _size,
				uint8_t _is_big_endian) :
				offset(_offset), size(_size), is_big_endian(_is_big_endian) {
		}
	};
	using KeyIterator = BTreeKeyIterator<Node, KeyInfo>;

	const BTree & t;
	using packet_spec_t = std::array<in_packet_position_t, _D>;
	const packet_spec_t & in_packet_position;
	packet_spec_t default_in_packet_position;

	/*
	 * Constructor with specified in packet positions
	 * */
	BTreeSearch(const BTree & _t, const packet_spec_t & _in_packet_pos) :
			t(_t), in_packet_position(_in_packet_pos) {
	}

	/*
	 * Constructor with a dummy in_packet_position for just simple array of values
	 * */
	BTreeSearch(const BTree & _t) :
			BTreeSearch(_t, default_in_packet_position) {
		for (uint16_t i = 0; i < _D; i++) {
			default_in_packet_position[i] = {
				static_cast<uint16_t>(i*2),
				2,
				static_cast<uint8_t>(false)};
		}
	}

	template<typename T>
	static SearchResult search_compressed_seq(const __m256i * keys,
			const level_t * _dims __attribute__((unused)),
			const uint8_t key_cnt __attribute__((unused)),
			const Range1d<T> & val __attribute__((unused))) {
		SearchResult r;
		r.val_index = 0;
		// search only the first position as this function is called only for the roots
		// if there is math the rest of node is checked on different place
		auto low = reinterpret_cast<const T*>(&keys[0])[0];
		auto high = reinterpret_cast<const T*>(&keys[1])[0];
		auto k = Range1d<T>(low, high);
		r.in_range = k == val;

		return r;
	}

	template<typename T>
	static SearchResult search_compressed_seq(const __m256i * keys,
			const level_t * _dims __attribute__((unused)),
			const uint8_t key_cnt __attribute__((unused)), const T val) {
		SearchResult r;
		r.val_index = 0;
		// search only the first position as this function is called only for the roots
		// if there is math the rest of node is checked on different place

		auto low = reinterpret_cast<const T*>(&keys[0])[0];
		auto high = reinterpret_cast<const T*>(&keys[1])[0];
		auto k = Range1d<T>(low, high);
		r.in_range = k.in_range(val);
		return r;
	}
	key_t get_val(const uint8_t * val_vec, level_t dim) const {
#ifndef NDEBUG
		assert(dim < in_packet_position.size());
#endif
		auto p = in_packet_position[dim];
		const key_t * k = reinterpret_cast<const key_t *>(val_vec + p.offset);
		if (p.size == 1) {
			return *k;
		} else if (p.is_big_endian) {
			assert(p.size == 2);
			return __swab16p(((const key_t *)k));
		} else {
			assert(p.size == 2);
			return *k;
		}
	}

	/*
	 * Search the value or range in single level of the tree
	 *
	 * @return pair<node where key resides, index of the key in node>
	 * @note if the node is compressed there may be some
	 * */
	template<typename Key_t>
	static std::pair<Node*, unsigned> search_possition_1d(Node * n,
			const Key_t val) {
		while (n) {
			//auto s = search_fn::search_avx2(n->keys, n->key_mask, val);
			SearchResult s;
			if (n->is_compressed) {
				s = search_compressed_seq(n->keys, &n->dim_index[0],
						n->key_cnt, val);
			} else {
				s = search_fn::search_seq(n->keys, n->key_cnt, val);
			}
			if (s.in_range) {
				return {n, s.val_index};
			} else if (n->is_leaf or n->is_compressed) {
				return {nullptr, 0};
			} else {
				n = n->child(s.val_index);
			}
		}
		return {nullptr, 0};
	}

	//void preprocess_avx2(union b_node* const node) {
	//	__m256i const perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
	//	__m256i * const middle = (__m256i *) &node->i32[4];
	//
	//	__m256i x = _mm256_loadu_si256(middle);
	//	x = _mm256_permutevar8x32_epi32(x, perm_mask);
	//	_mm256_storeu_si256(middle, x);
	//}

	// A function to get predecessor of keys[idx] (only in the local subtree)
	static typename BTree::KeyInfo getPred(Node & node, unsigned idx) {
		// Keep moving to the right most node until we reach a leaf
		auto cur = node.child(idx);
		while (!cur->is_leaf) {
			cur = cur->child(cur->key_cnt);
		}
		// Return the last key of the leaf
		return cur->get_key(cur->key_cnt - 1);
	}
	// A function to get successor of keys[idx] (only in the local subtree)
	static typename BTree::KeyInfo getSucc(Node & node, unsigned idx) {
		// Keep moving the left most node starting from child(idx+1) until we reach a leaf
		auto * cur = node.child(idx + 1);
		while (not cur->is_leaf) {
			cur = cur->child(0);
		}
		// Return the first key of the leaf
		return cur->get_key(0);
	}
	static KeyIterator iter_keys(BTree & tree) {
		return KeyIterator(tree.root);
	}
	static KeyIterator iter_keys(Node * start, unsigned start_i) {
		return KeyIterator(start, start_i);
	}
	// Find index of the key in this node
	static unsigned findKey(Node & node, const Range1d<key_t> k) {
		unsigned i = 0;
		while (i < node.key_cnt && node.get_key(i) < k) {
			++i;
		}
		return i;
	}
	// @param n node to search in
	// @param _val value vector to search
	// @param res id or best matching rule
	// @param i dimension index (the index of the field index in the dimension_order array)
	Node * search_rest_of_path_in_compressed_node(Node * n, const uint8_t * val_vec, rule_value_t & res,
			unsigned & i) const {
		// first item was already checked in search_possition_1d
		// find length of the sequence of the matching ranges in the items stored in node
		auto v0 = n->value[0];
		if (v0.is_valid()) {
			res = v0;
		}
		Node * next_n = nullptr;
		unsigned add_to_i = 0;
		for (unsigned i2 = 1; i2 < n->key_cnt; i2++) {
			auto k = n->get_key(i2);
			auto d = n->get_dim(i2);
			typename key_vec_t::value_type val = get_val(val_vec, d);
			if (not k.key.in_range(val)) {
				break;
			}
			auto v = n->value[i2];
			if (v.is_valid()) {
				res = v;
			}
			auto _next_n = n->get_next_layer(i2);
			if (_next_n) {
				// do not overwrite if there are some non matching items in this node
				next_n = _next_n;
				add_to_i = i2 + 1;
			}
		}
		i += add_to_i;
		return next_n;
	}
	/*
	 * Search in all levels of the tree
	 *
	 * */
	rule_value_t search(const key_vec_t & _val) const {
		const uint8_t * val_vec = (const uint8_t*) &_val[0];
		return search(val_vec);
	}

	rule_value_t search(const uint8_t * val_vec) const {
		rule_value_t res;
		Node * n = t.root;
		unsigned i = 0;
		while (n) {
			auto d = t.dimension_order[i];
			auto val = get_val(val_vec, d);
			auto r = search_possition_1d(n, val);
			n = r.first;
			if (n) {
				if (n->is_compressed) {
					n = search_rest_of_path_in_compressed_node(n, val_vec, res, i);
					continue;
				}
				auto v = n->value[r.second];
				if (v.is_valid()) {
					// some matching rule found on path from the root in this node
					res = v;
				}
				// search in next layer if there is some
				n = n->get_next_layer(r.second);
				i++;
			}
		}
		return res;
	}
	/*
	 * Search the tuples <node, index in node> for each part of the rule
	 * @note the items are in order specified by dimension_order array
	 *
	 * @param path vector of tuples <root of the tree, node where item is stored, the index on which the item is stored>
	 * 		the compressed node may be stored in the path multiple times if multiple keys are matching on the path
	 * @param start the index of the item in rule vector where the search should start
	 **/
	static void search_path(Node * root,
			const std::array<unsigned, BTree::D> & dimension_order,
			const rule_spec_t & rule,
			std::vector<std::tuple<Node *, Node *, unsigned>> & path,
			size_t start = 0) {
		{
			Node * n = root;
			unsigned level = start;
			while (n) {
				auto d = dimension_order[level];
#ifndef NDEBUG
				{
					auto n_d = n->get_dim(0);
					assert(n_d == d);
				}
#endif
				auto val = rule.first[d];
				auto r = search_possition_1d(n, val);
				if (r.first == nullptr) {
					break;
				}

#ifndef NDEBUG
				{
					auto n_d = r.first->get_dim(r.second);
					assert(n_d == d);
					auto nl = r.first->get_next_layer(r.second);
					auto _d = dimension_order[level + 1];
					if (nl != nullptr) {
						auto nl_dim = nl->get_dim(0);
						assert(nl_dim == _d);
					}
				}
#endif
				// some matching rule found on path from the root in this node
				// search in next layer if there is some
				path.push_back(
						std::tuple<Node *, Node *, unsigned>(n, r.first,
								r.second));

				if (r.first->is_compressed and r.second < n->key_cnt) {
					// it is required to find the rest of the path in this compressed node
					//assert(d == n->get_dim(0));
					assert(n->key_cnt > 1);
					assert(n == r.first);

					bool match = true;
					unsigned i2;
					for (i2 = 1; i2 < n->key_cnt; i2++) {
						// (the first item was already checked)
						auto _d = dimension_order[level + i2];
#ifndef NDEBUG
						auto actual_d = n->get_dim(i2);
						assert(_d == actual_d);
#endif
						auto k = rule.first[_d];
						if (n->get_key(i2).key != k) {
							match = false;
							break;
						}
						path.push_back(
								std::tuple<Node *, Node *, unsigned>(n, n, i2));
					}
					if (not match) {
						break;
					}

					level += n->key_cnt;
					n = n->get_next_layer(n->key_cnt - 1);
				} else {
					level++;
					n = r.first->get_next_layer(r.second);
				}
			}
		}
#ifndef NDEBUG
		{
			// integrity check of the path continuity
			for (size_t i = 0; i < path.size(); i++) {
				Node *r, *n;
				unsigned i2;
				std::tie(r, n, i2) = path[i];

				if (i == 0) {
					assert(r == root);
				} else {
					// asssert that the path is continuous
					Node *_r, *_n;
					unsigned _i2;
					std::tie(_r, _n, _i2) = path[i - 1];
					if (_n->is_compressed and _n == n) {
						assert(i2 == _i2 + 1);
					} else {
						assert(r == _n->get_next_layer(_i2));
					}
				}

				assert(r->parent == nullptr && "root is really root");
				if (n->is_compressed) {
					assert(r == n);
				} else {
					assert(
							r->get_dim(0) == n->get_dim(0)
							&& "root and node are from same tree");
				}
				auto n_d = n->get_dim(i2);
				auto l = start + i;
				auto expected_d = dimension_order[l];
				assert(n_d == expected_d);
			}
		}
#endif
	}

};

}
