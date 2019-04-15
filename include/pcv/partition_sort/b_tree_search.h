#pragma once
#include <array>

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
SearchResult search_compressed_seq(const __m256i * keys, const __m64 * _dims,
		const uint8_t key_cnt, const Range1d<T> & val) {
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
SearchResult search_compressed_seq(const __m256i * keys, const __m64 * _dims,
		const uint8_t key_cnt, const T val) {
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

template<>
SearchResult search_avx2<uint32_t>(const __m256i * keys, uint32_t val) {
	__m256i value = _mm256_set1_epi32(val);
	// compare the two halves of the cache line.
	// load 256b to avx
	__m256i cmp1 = _mm256_load_si256(&keys[0]);
	__m256i cmp2 = _mm256_load_si256(&keys[1]);

	// 8* > u32
	cmp1 = _mm256_cmpgt_epu32(cmp1, value);
	cmp2 = _mm256_cmpgt_epu32(cmp2, value);

	// merge the comparisons back together.
	//
	// a permute is required to get the pack results back into order
	// because AVX-256 introduced that unfortunate two-lane interleave.
	//
	// alternately, you could pre-process your data to remove the need
	// for the permute.

	__m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
	__m256i cmp = _mm256_packs_epi32(cmp1, cmp2); // PACKSSDW
	cmp = _mm256_permutevar8x32_epi32(cmp, perm_mask); // PERMD

	// finally create a move mask and count trailing
	// zeroes to get an index to the next node.
	uint32_t mask = _mm256_movemask_epi8(cmp); // PMOVMSKB
	auto next_index = _tzcnt_u32(mask) / 2; // TZCNT

	SearchResult r;
	r.val_index = next_index;
	r.in_range = false; // [TODO]
	return r;
}

}

template<typename BTree>
class BTreeSearch {
public:
	using Node = typename BTree::Node;
	using value_t = typename BTree::value_t;
	using rule_spec_t = typename BTree::rule_spec_t;
	using KeyInfo = typename BTree::KeyInfo;
	using rule_id_t = typename BTree::rule_id_t;
	using val_vec_t = typename BTree::val_vec_t;

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
				return actual->get_key(index);
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
				s = search_fn::search_compressed_seq(n->keys, n->dim_index,
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
	//	__m256i   const perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
	//	__m256i * const middle = (__m256i *) &node->i32[4];
	//
	//	__m256i x = _mm256_loadu_si256(middle);
	//	x = _mm256_permutevar8x32_epi32(x, perm_mask);
	//	_mm256_storeu_si256(middle, x);
	//}

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
		return cur->get_key(cur->key_cnt - 1);
	}
	// A function to get successor of keys[idx] (only in the local subtree)
	static typename BTree::KeyInfo getSucc(Node & node, unsigned idx) {
		// Keep moving the left most node starting from child(idx+1) until we reach a leaf
		auto * cur = node.child(idx + 1);
		while (not cur->is_leaf)
			cur = cur->child(0);

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
	static unsigned findKey(Node & node, const Range1d<value_t> k) {
		unsigned i = 0;
		while (i < node.key_cnt && node.get_key(i) < k)
			++i;
		return i;
	}

	/*
	 * Search in all levels of the tree
	 *
	 * [TODO] use array
	 * */
	static rule_id_t search(BTree & t, const val_vec_t & _val) {
		rule_id_t res = BTree::INVALID_RULE;
		Node * n = t.root;
		unsigned i = 0;
		while (n) {
			auto d = t.dimension_order[i];
			auto val = _val[d];
			auto r = search_possition_1d(n, val);
			n = r.first;
			if (n) {
				if (n->is_compressed) {
					// first item was already checked in search_possition_1d
					// find length of the sequence of the matching ranges in the items stored in node
					unsigned i2;
					Node * next_n = nullptr;
					for (i2 = 1; i2 < n->key_cnt; i2++) {
						auto k = n->get_key(i2);
						d = n->get_dim(i2);
						val = _val[d];
						if (not k.key.in_range(val)) {
							break;
						}
						auto v = n->value[i2];
						if (v != BTree::INVALID_RULE) {
							res = v;
						}
						auto _next_n = n->get_next_layer(i2);
						if (_next_n)
							next_n = _next_n;
					}
					n = next_n;
					continue;
				}
				auto v = n->value[r.second];
				if (v != BTree::INVALID_RULE) {
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
			const std::array<int, BTree::D> & dimension_order,
			const rule_spec_t & rule,
			std::vector<std::tuple<Node *, Node *, unsigned>> & path,
			size_t start = 0) {
		Node * n = root;
		unsigned level = start;
		while (n) {
			auto d = dimension_order[level];
			auto val = rule.first[d];
			auto r = search_possition_1d(n, val);
			if (r.first == nullptr)
				break;
			// some matching rule found on path from the root in this node
			// search in next layer if there is some
			path.push_back(
					std::tuple<Node *, Node *, unsigned>(n, r.first, r.second));
			if (r.first->is_compressed and r.second < n->key_cnt) {
				// it is required to find the rest of the path in this compressed node
				assert(d == n->get_dim(0));
				assert(n->key_cnt > 1);

				n = r.first;
				bool match = true;
				unsigned i2;
				for (i2 = 1; i2 < n->key_cnt; i2++) {
					// (the first item was already checked)
					auto d = dimension_order[level + i2];
					assert(d == n->get_dim(i2));
					auto k = rule.first[d];
					if (n->get_key(i2).key != k) {
						match = false;
						break;
					}
					path.push_back(
							std::tuple<Node *, Node *, unsigned>(n, n, i2));
				}
				if (not match)
					break;
				r.second = i2;
			}
			n = r.first->get_next_layer(r.second);
			level++;
		}
	}

};

}
