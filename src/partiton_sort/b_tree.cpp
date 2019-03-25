#include <pcv/partiton_sort/b_tree.h>

using namespace std;

namespace pcv {

void print_avx2_hex256(__m256i ymm) {
	array<uint64_t, sizeof(__m256i) / sizeof(u_int32_t)> buffer;
	_mm256_storeu_si256((__m256i*)&buffer[0], ymm);
	for (auto i: buffer) {
		std::cout << i << " ";
	}
}

const BTree::index_t BTree::INVALID_INDEX = std::numeric_limits<index_t>::max();
const BTree::index_t BTree::INVALID_RULE = INVALID_INDEX;

BTree::BTree() :
		root(nullptr) {
	for (size_t i = 0; i < dimension_order.size(); i++)
		dimension_order[i] = i;
}

BTree::Node::Node() {
	assert(((uintptr_t ) this) % 64 == 0);
	keys[0] = keys[1] = _mm256_set1_epi32(std::numeric_limits<uint32_t>::max());
	dim_index = _m_from_int64(std::numeric_limits<uint64_t>::max());
	std::fill(value.begin(), value.end(), INVALID_INDEX);
	clean_children();
	set_key_cnt(0);
	is_leaf = true;
	parent = nullptr;
}

void BTree::Node::clean_children() {
	std::fill(next_level.begin(), next_level.end(), INVALID_INDEX);
	std::fill(child_index.begin(), child_index.end(), INVALID_INDEX);
}

void BTree::Node::set_key_cnt(size_t key_cnt) {
	assert(key_cnt <= MAX_DEGREE);
	this->key_cnt = key_cnt;
	key_mask = (1 << key_cnt) - 1;
}

BTree::Node * BTree::Node::child(const index_t index) {
	if (child_index[index] == INVALID_INDEX)
		return nullptr;
	else
		return &by_index(child_index[index]);
}

const BTree::Node * BTree::Node::child(const index_t index) const {
	if (child_index[index] == INVALID_INDEX)
		return nullptr;
	else
		return &by_index(child_index[index]);
}

BTree::Node * BTree::Node::get_next_layer(unsigned index) {
	auto i = next_level[index];
	if (i == INVALID_INDEX)
		return nullptr;
	else
		return &by_index(i);
}

const BTree::Node * BTree::Node::get_next_layer(unsigned index) const {
	auto i = next_level[index];
	if (i == INVALID_INDEX)
		return nullptr;
	else
		return &by_index(i);
}

unsigned BTree::Node::findKey(const Range1d<value_t> k) {
	unsigned i = 0;
	while (i < key_cnt && get_key<uint32_t>(i) < k)
		++i;
	return i;
}

BTree::Node::~Node() {
	if (not is_leaf) {
		for (uint8_t i = 0; i < key_cnt + 1; i++) {
			delete child(i);
		}
	}
	for (uint8_t i = 0; i < key_cnt; i++) {
		delete get_next_layer(i);
	}
}

void BTree::Node::set_child(unsigned index, Node * child) {
	if (child == nullptr)
		child_index[index] = INVALID_INDEX;
	else {
		child_index[index] = Node::_Mempool_t::getId(child);
		child->parent = this;
	}
}

void BTree::Node::set_next_layer(unsigned index, Node * next_layer_root) {
	if (next_layer_root == nullptr)
		next_level[index] = INVALID_INDEX;
	else
		next_level[index] = Node::_Mempool_t::getId(next_layer_root);
}

size_t BTree::size() const {
	if (root)
		return root->size();
	else
		return 0;
}

size_t BTree::Node::size() const {
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

BTree::~BTree() {
	delete root;
	root = nullptr;
}

}
