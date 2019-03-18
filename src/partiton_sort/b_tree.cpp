#include <pcv/partiton_sort/b_tree.h>

using namespace std;

void print_avx2_hex256(__m256i ymm) {
	array<uint64_t, sizeof(__m256i) / sizeof(u_int32_t)> buffer;
	_mm256_storeu_si256((__m256i*)&buffer[0], ymm);
	for (auto i: buffer) {
		std::cout << i << " ";
	}
}

const BTree::index_t BTree::INVALID_INDEX = std::numeric_limits<index_t>::max();
const BTree::index_t BTree::INVALID_RULE = INVALID_INDEX;

void BTree::Node::set_key_cnt(size_t key_cnt) {
	this->key_cnt = key_cnt;
	key_mask = (1 << key_cnt) - 1;
}

BTree::Node & BTree::Node::by_index(const index_t index) {
	return *reinterpret_cast<Node*>(BTree::Node::_Mempool_t::getById(index));
}

BTree::Node & BTree::Node::child(const index_t index) {
	return by_index(child_index[index]);
}

BTree::Node * BTree::Node::get_next_layer(unsigned index) {
	auto i = child_index[index];
	if (i == INVALID_INDEX)
		return nullptr;
	else
		return &by_index(i);
}

// A utility function to insert a new key in this node
// The assumption is, the node must be non-full when this
// function is called
void BTree::Node::insertNonFull(const rule_spec_t & rule) {
	// Initialize index as index of rightmost element
	int i = int(key_cnt) - 1;
	assert(i >= 0);
	// If this is a leaf node
	auto k = rule.first[0];
	if (is_leaf) {
		// The following loop does two things
		// a) Finds the location of new key to be inserted
		// b) Moves all greater keys to one place ahead
		while (i >= 0 && get_key<uint32_t>(i) > k) {
			set_key<uint32_t>(i + 1, get_key<uint32_t>(i));
			i--;
		}

		// Insert the new key at found location
		set_key<uint32_t>(i + 1,
				KeyInfo<uint32_t>(k, rule.second, INVALID_INDEX));
		set_key_cnt(key_cnt + 1);
	} else // If this node is not leaf
	{
		// Find the child which is going to have the new key
		while (i >= 0 && get_key<uint32_t>(i) > k)
			i--;

		// See if the found child is full
		if (child(i + 1).key_cnt == MAX_DEGREE) {
			// If the child is full, then split it
			splitChild(i + 1, child(i + 1));

			// After split, the middle key of C[i] goes up and
			// C[i] is splitted into two. See which of the two
			// is going to have the new key
			if (get_key<uint32_t>(i + 1) < k)
				i++;
		}
		child(i + 1).insertNonFull(rule);
	}
}

void BTree::Node::set_child(unsigned index, Node * child) {
	child_index[index] = Node::_Mempool_t::getId(child);
}
void BTree::Node::splitChild(int i, Node & y) {
	// Create a new node which is going to store (t-1) keys
	// of y
	Node *z = new Node;
	z->is_leaf = y.is_leaf;
	z->set_key_cnt(MIN_DEGREE - 1);

	// Copy the last (t-1) keys of y to z
	for (int j = 0; j < int(MIN_DEGREE) - 1; j++)
		set_key<uint32_t>(j, y.get_key<uint32_t>(MIN_DEGREE + j));

	// Copy the last t children of y to z
	if (not y.is_leaf) {
		for (int j = 0; j < int(MIN_DEGREE); j++)
			z->child_index[j] = y.child_index[MIN_DEGREE + j];
	}

	// Reduce the number of keys in y
	y.set_key_cnt(MIN_DEGREE - 1);

	// Since this node is going to have a new child,
	// create space of new child
	for (int j = key_cnt; j >= i + 1; j--)
		child_index[j + 1] = child_index[j];

	// Link the new child to this node
	set_child(i + 1, z);

	// A key of y will move to this node. Find location of
	// new key and move all greater keys one space ahead
	for (int j = key_cnt - 1; j >= i; j--)
		set_key<uint32_t>(j + 1, get_key<uint32_t>(j));

	// Copy the middle key of y to this node
	set_key<uint32_t>(i, get_key<uint32_t>(MIN_DEGREE - 1));

	// Increment count of keys in this node
	set_key_cnt(key_cnt + 1);
}

// https://www.geeksforgeeks.org/b-tree-set-1-insert-2/
void BTree::insert(const rule_spec_t & rule) {
	// If tree is empty
	if (root == nullptr) {
		// Allocate memory for root
		root = new Node();
		root->set_key(0,
				Node::KeyInfo<uint32_t>(rule.first[0], rule.second,
						INVALID_INDEX)); // Insert key
		root->set_key_cnt(1);
		root->value[0] = rule.second;
	} else {
		// If root is full, then tree grows in height
		if (root->key_cnt == Node::MAX_DEGREE) {
			// Allocate memory for new root
			Node *s = new Node;
			s->is_leaf = false;

			// Make old root as child of new root
			s->set_child(0, root);

			// Split the old root and move 1 key to the new root
			s->splitChild(0, *root);

			// New root has two children now.  Decide which of the
			// two children is going to have new key
			int i = 0;
			if (s->get_key<uint32_t>(i) < rule.first[0])
				i++;
			s->child(i).insertNonFull(rule);

			// Change root
			root = s;
		} else
			// If root is not full, call insertNonFull for root
			root->insertNonFull(rule);
	}
}

/*
 * 8*32b a > b
 *
 * @param a the signed integer vector
 * @param b the unsigned integer vector
 * */
inline __m256i     __attribute__((__always_inline__))
    _mm256_cmpgt_epu32(
		__m256i   const a, __m256i     const b) {
	constexpr uint32_t offset = 0x1 << 31;
	__m256i   const fix_val = _mm256_set1_epi32(offset);
	return _mm256_cmpgt_epi32(_mm256_add_epi32(a, fix_val), b); // PCMPGTD
}

BTree::rule_id_t BTree::search(const value_t & val) {
	rule_id_t res = INVALID_RULE;
	Node * n = root;
	while (n) {
		//auto s = search_avx2(*n, val);
		auto s = search_seq(*n, val);
		if (s.in_range) {
			auto v = n->value[s.val_index];
			if (v != INVALID_RULE) {
				// some matching rule found on path from the root in this node
				res = v;
				// search in next layer if there is some
				n = n->get_next_layer(s.val_index);
			}
		} else if (n->is_leaf) {
			// did not found any suitable item in this node and there is nothing to search further
			break;
		} else {
			n = &n->child(s.val_index);
		}
	}

	return res;
}

//void preprocess_avx2(union b_node* const node) {
//	__m256i   const perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
//	__m256i * const middle = (__m256i *) &node->i32[4];
//
//	__m256i x = _mm256_loadu_si256(middle);
//	x = _mm256_permutevar8x32_epi32(x, perm_mask);
//	_mm256_storeu_si256(middle, x);
//}

BTree::SearchResult BTree::search_seq(const Node & node, value_t val) {
BTree::SearchResult r;
// [TODO] bin search
for (r.val_index = 0; r.val_index < node.key_cnt; r.val_index++) {
	Node::KeyInfo<uint32_t> cur = node.get_key<uint32_t>(r.val_index);
	if (val > cur.key.high) {
		continue;
	} else if (cur.in_range(val)) {
		r.in_range = true;
		break;
	}
}
assert(r.val_index < Node::MAX_DEGREE + 1);
return r;
}

BTree::SearchResult BTree::search_avx2(const Node & node, value_t val) {
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

__m256i   const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
__m256i cmp = _mm256_packs_epi32(cmp1, cmp2); // PACKSSDW
cmp = _mm256_permutevar8x32_epi32(cmp, perm_mask); // PERMD

// finally create a move mask and count trailing
// zeroes to get an index to the next node.
uint32_t mask = _mm256_movemask_epi8(cmp); // PMOVMSKB
auto next_index = _tzcnt_u32(mask) / 2; // TZCNT

BTree::SearchResult r;
r.val_index = next_index;
r.in_range = false; // [TODO]
assert(r.val_index < Node::MAX_DEGREE + 1);
return r;
}
