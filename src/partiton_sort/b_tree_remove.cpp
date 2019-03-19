#include <pcv/partiton_sort/b_tree.h>

namespace pcv {

template<typename KEY_t>
void move_key(BTree::Node & src, uint8_t src_i, BTree::Node & dst,
		uint8_t dst_i) {
	dst.set_key<KEY_t>(dst_i, src.get_key<KEY_t>(src_i));
}

void BTree::Node::borrowFromPrev(int idx) {
	Node & ch = child(idx);
	Node & sibling = child(idx - 1);

	// The last key from child(idx-1) goes up to the parent and key[idx-1]
	// from parent is inserted as the first key in child(idx). Thus, the loses
	// sibling one key and child gains one key

	// Moving all key in child(idx) one step ahead
	for (int i = ch.key_cnt - 1; i >= 0; --i)
		move_key<uint32_t>(ch, i, ch, i + 1);

	// If child(idx) is not a leaf, move all its child pointers one step ahead
	if (!ch.is_leaf) {
		for (int i = ch.key_cnt; i >= 0; --i)
			ch.child_index[i + 1] = ch.child_index[i];
	}

	// Setting child's first key equal to keys[idx-1] from the current node
	move_key<uint32_t>(*this, idx - 1, ch, 0);

	// Moving sibling's last child as child(idx)'s first child
	if (!ch.is_leaf) {
		ch.child_index[0] = sibling.child_index[sibling.key_cnt];
	}
	// Moving the key from the sibling to the parent
	// This reduces the number of keys in the sibling
	move_key<uint32_t>(sibling, sibling.key_cnt - 1, *this, idx - 1);

	ch.set_key_cnt(ch.key_cnt + 1);
	sibling.set_key_cnt(sibling.key_cnt - 1);

	return;
}

void BTree::Node::borrowFromNext(int idx) {

	auto & ch = child(idx);
	auto & sibling = child(idx + 1);

	// keys[idx] is inserted as the last key in child(idx)
	move_key<uint32_t>(*this, idx, ch, ch.key_cnt);

	// Sibling's first child is inserted as the last child
	// into child(idx)
	if (!(ch.is_leaf))
		ch.child_index[ch.key_cnt + 1] = sibling.child_index[0];

	//The first key from sibling is inserted into keys[idx]
	move_key<uint32_t>(sibling, 0, *this, idx);

	// Moving all keys in sibling one step behind
	for (int i = 1; i < sibling.key_cnt; ++i)
		move_key<uint32_t>(sibling, i, sibling, i - 1);

	// Moving the child pointers one step behind
	if (!sibling.is_leaf) {
		for (int i = 1; i <= sibling.key_cnt; ++i)
			sibling.child_index[i - 1] = sibling.child_index[i];
	}

	// Increasing and decreasing the key count of child(idx) and child(idx+1)
	// respectively
	ch.set_key_cnt(ch.key_cnt + 1);
	sibling.set_key_cnt(sibling.key_cnt - 1);
}

void BTree::Node::merge(int idx) {
	auto& ch = child(idx);
	auto& sibling = child(idx + 1);

	// Pulling a key from the current node and inserting it into (t-1)th
	// position of child(idx)
	move_key<uint32_t>(*this, idx, ch, MIN_DEGREE - 1);

	// Copying the keys from child(idx+1) to child(idx) at the end
	for (unsigned i = 0; i < sibling.key_cnt; ++i)
		move_key<uint32_t>(sibling, i, ch, i + MIN_DEGREE);

	// Copying the child pointers from child(idx+1) to child(idx)
	if (!ch.is_leaf) {
		for (unsigned i = 0; i <= sibling.key_cnt; ++i)
			ch.child_index[i + MIN_DEGREE] = sibling.child_index[i];
	}

	// Moving all keys after idx in the current node one step before -
	// to fill the gap created by moving keys[idx] to child(idx)
	for (unsigned i = idx + 1; i < key_cnt; ++i)
		move_key<uint32_t>(*this, i, *this, i - 1);

	// Moving the child pointers after (idx+1) in the current node one
	// step before
	for (unsigned i = idx + 2; i <= key_cnt; ++i)
		child_index[i - 1] = child_index[i];

	// Updating the key count of child and the current node
	ch.set_key_cnt(sibling.key_cnt + 1);
	set_key_cnt(key_cnt - 1);

	delete &sibling;
}

// A function to fill child child(idx) which has less than t-1 keys
void BTree::Node::fill(int idx) {
	// If the previous child(idx-1) has more than MIN_DEGREE-1 keys, borrow a key
	// from that child
	if (idx != 0 && child(idx - 1).key_cnt >= MIN_DEGREE) {
		borrowFromPrev(idx);

		// If the next child(idx+1) has more than t-1 keys, borrow a key
		// from that child
	} else if (idx != key_cnt && child(idx + 1).key_cnt >= MIN_DEGREE) {
		borrowFromNext(idx);

		// Merge child(idx) with its sibling
		// If child(idx) is the last child, merge it with with its previous sibling
		// Otherwise merge it with its next sibling
	} else {
		if (idx != key_cnt)
			merge(idx);
		else
			merge(idx - 1);
	}
	return;
}

void BTree::Node::remove(Range1d<uint32_t> k, int lvl, const rule_spec_t & _k) {
	int idx = findKey(k);

	// The key to be removed is present in this node
	if (idx < key_cnt && get_key<uint32_t>(idx).key == k) {
		// If the node is a leaf node - removeFromLeaf is called
		// Otherwise, removeFromNonLeaf function is called
		if (is_leaf)
			removeFromLeaf(idx);
		else
			removeFromNonLeaf(idx, lvl, _k);
	} else {

		// If this node is a leaf node, then the key is not present in tree
		if (is_leaf) {
			throw std::runtime_error("The key is does not exist in the tree");
		}

		// The key to be removed is present in the sub-tree rooted with this node
		// The flag indicates whether the key is present in the sub-tree rooted
		// with the last child of this node
		bool flag = ((idx == key_cnt) ? true : false);

		// If the child where the key is supposed to exist has less that t keys,
		// we fill that child
		if (child(idx).key_cnt < MAX_DEGREE)
			fill(idx);

		// If the last child has been merged, it must have merged with the previous
		// child and so we recurse on the (idx-1)th child. Else, we recurse on the
		// (idx)th child which now has atleast t keys
		if (flag && idx > key_cnt)
			child(idx - 1).remove(k, lvl, _k);
		else
			child(idx).remove(k, lvl, _k);
	}
	return;
}

void BTree::Node::removeFromLeaf(unsigned idx) {
	// Move all the keys after the idx-th pos one place backward
	for (unsigned i = idx + 1; i < key_cnt; ++i)
		move_key<uint32_t>(*this, i, *this, i - 1);

	// Reduce the count of keys
	set_key_cnt(key_cnt - 1);
}

// A function to get predecessor of keys[idx]
BTree::KeyInfo BTree::Node::getPred(unsigned idx) {
	// Keep moving to the right most node until we reach a leaf
	auto cur = &child(idx);
	while (!cur->is_leaf)
		cur = &cur->child(cur->key_cnt);

	// Return the last key of the leaf
	return cur->get_key<uint32_t>(cur->key_cnt - 1);
}

BTree::KeyInfo BTree::Node::getSucc(unsigned idx) {
	// Keep moving the left most node starting from child(idx+1) until we reach a leaf
	auto * cur = &child(idx + 1);
	while (not cur->is_leaf)
		cur = &cur->child(0);

	// Return the first key of the leaf
	return cur->get_key<uint32_t>(0);
}

void BTree::Node::removeFromNonLeaf(unsigned idx, unsigned lvl,
		const rule_spec_t & _k) {
	auto k = get_key<uint32_t>(idx);

	if (child(idx).key_cnt >= MIN_DEGREE) {
		// If the child that precedes k (child(idx)) has atleast t keys,
		// find the predecessor 'pred' of k in the subtree rooted at
		// child(idx). Replace k by pred. Recursively delete pred
		// in child(idx)
		auto pred = getPred(idx);
		set_key<uint32_t>(idx, pred);
		child(idx).remove(pred.key, lvl, _k);

	} else if (child(idx + 1).key_cnt >= MIN_DEGREE) {
		// If the child child(idx) has less that t keys, examine child(idx+1).
		// If child(idx+1) has at least t keys, find the successor 'succ' of k in
		// the subtree rooted at child(idx+1)
		// Replace k by succ
		// Recursively delete succ in child(idx+1)
		auto succ = getSucc(idx);
		set_key<uint32_t>(idx, succ);
		child(idx + 1).remove(succ.key, lvl, _k);

	} else {

		// If both child(idx) and child(idx+1) has less that t keys,merge k and all of child(idx+1)
		// into child(idx)
		// Now child(idx) contains 2t-1 keys
		// Free child(idx+1) and recursively delete k from child(idx)
		merge(idx);
		child(idx).remove(k.key, lvl, _k);
	}
}

void BTree::remove(const rule_spec_t & k) {
	if (!root) {
		throw std::runtime_error("The tree is empty");
	}

	// Call the remove function for root
	root->remove(k.first[0], 0, k);

	// If the root node has 0 keys, make its first child as the new root
	// if it has a child, otherwise set root as NULL
	if (root->key_cnt == 0) {
		auto tmp = root;
		if (root->is_leaf)
			root = nullptr;
		else
			root = &root->child(0);

		// Free the old root
		delete tmp;
	}
}

}
