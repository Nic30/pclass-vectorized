#include <pcv/partiton_sort/b_tree.h>

using namespace std;

namespace pcv {

/*
 * 8*32b a > b
 *
 * @param a the signed integer vector
 * @param b the unsigned integer vector
 * */
inline __m256i                             __attribute__((__always_inline__))                             _mm256_cmpgt_epu32(
		__m256i                             const a, __m256i                             const b) {
	constexpr uint32_t offset = 0x1 << 31;
	__m256i                             const fix_val = _mm256_set1_epi32(offset);
	return _mm256_cmpgt_epi32(_mm256_add_epi32(a, fix_val), b); // PCMPGTD
}

BTree::rule_id_t BTree::search(const value_t & val) {
	vector<value_t> v = { val, };
	return search(v);
}

//void preprocess_avx2(union b_node* const node) {
//	__m256i   const perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
//	__m256i * const middle = (__m256i *) &node->i32[4];
//
//	__m256i x = _mm256_loadu_si256(middle);
//	x = _mm256_permutevar8x32_epi32(x, perm_mask);
//	_mm256_storeu_si256(middle, x);
//}

BTree::SearchResult search_seq(const BTree::Node & node,
		const Range1d<BTree::value_t> val) {
	BTree::SearchResult r;
	for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
		BTree::KeyInfo cur = node.get_key<uint32_t>(r.val_index);
		if (val < cur.key.low) {
			break;
		} else if (cur.key == val) {
			r.in_range = true;
			break;
		}
	}
	return r;
}

BTree::SearchResult search_seq(const BTree::Node & node, BTree::value_t val) {
	BTree::SearchResult r;
	for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
		BTree::KeyInfo cur = node.get_key<uint32_t>(r.val_index);
		if (val < cur.key.low) {
			break;
		} else if (cur.in_range(val)) {
			r.in_range = true;
			break;
		}
	}
	return r;
}

BTree::SearchResult search_avx2(const BTree::Node & node, BTree::value_t val) {
	__m256i value = _mm256_set1_epi32(val);
	// compare the two halves of the cache line.
	// load 256b to avx
	__m256i cmp1 = _mm256_load_si256(&node.keys[0]);
	__m256i cmp2 = _mm256_load_si256(&node.keys[1]);

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

	__m256i                             const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
	__m256i cmp = _mm256_packs_epi32(cmp1, cmp2); // PACKSSDW
	cmp = _mm256_permutevar8x32_epi32(cmp, perm_mask); // PERMD

	// finally create a move mask and count trailing
	// zeroes to get an index to the next node.
	uint32_t mask = _mm256_movemask_epi8(cmp); // PMOVMSKB
	auto next_index = _tzcnt_u32(mask) / 2; // TZCNT

	BTree::SearchResult r;
	r.val_index = next_index;
	r.in_range = false; // [TODO]
	assert(r.val_index < BTree::Node::MAX_DEGREE + 1);
	return r;
}

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

BTree::rule_id_t BTree::search(const std::vector<value_t> & _val) {
	rule_id_t res = INVALID_RULE;
	Node * n = root;
	unsigned i = 0;
	while (n) {
		auto d = dimension_order[i];
		auto val = _val[d];
		auto r = search_possition_1d(n, val);
		n = r.first;
		if (n) {
			auto v = n->value[r.second];
			if (v != INVALID_RULE) {
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

void BTree::search_path(const rule_spec_t & rule,
		std::vector<std::pair<Node *, unsigned>> & path) {
	Node * n = root;
	unsigned i = 0;
	while (n) {
		auto d = dimension_order[i];
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

BTree::Node::KeyIterator BTree::Node::iter_keys() {
	return KeyIterator(this);
}

unsigned index_of_child(BTree::Node *p, BTree::Node * ch) {
	size_t i = 0;
	for (; p != nullptr and i <= p->key_cnt; i++) {
		if (p->child(i) == ch)
			break;
	}
	return i;
}
pair<BTree::Node*, unsigned> get_most_left(BTree::Node * n) {
	while (not n->is_leaf) {
		n = n->child(0);
	}
	return {n, 0};
}
pair<BTree::Node*, unsigned> get_most_left_after(BTree::Node * p,
		BTree::Node * ch) {
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
std::pair<BTree::Node*, unsigned> BTree::Node::getSucc_global(unsigned idx) {
	if (is_leaf) {
		if (int(idx) < int(key_cnt) - 1) {
			// move forward in this node
			return {this, idx + 1};
		} else {
			// just move to parent key
			return {parent, index_of_child(parent, this)};
		}
	} else if (idx < key_cnt) {
		// move on lowest in right sibling
		return get_most_left(child(idx + 1));
	} else {
		return get_most_left_after(parent, this);
	}
}

}
