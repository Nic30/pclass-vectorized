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
	auto i = next_level[index];
	if (i == INVALID_INDEX)
		return nullptr;
	else
		return &by_index(i);
}

BTree::Node::~Node() {
	if (not is_leaf) {
		for (uint8_t i = 0; i < key_cnt + 1; i++) {
			delete &child(i);
		}
	}
	for (uint8_t i = 0; i < key_cnt; i++) {
		delete get_next_layer(i);
	}
}

void BTree::Node::set_child(unsigned index, Node * child) {
	child_index[index] = Node::_Mempool_t::getId(child);
}

void BTree::Node::set_next_layer(unsigned index, Node * next_layer_root) {
	next_level[index] = Node::_Mempool_t::getId(next_layer_root);
}

BTree::~BTree() {
	delete root;
}

}
