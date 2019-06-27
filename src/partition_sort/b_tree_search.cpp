#include <pcv/partition_sort/b_tree_search.h>

namespace pcv {
namespace search_fn {

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
}
