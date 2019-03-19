#include <pcv/partiton_sort/b_tree.h>

using namespace std;

namespace pcv {

// A utility function to insert a new key in this node
// The assumption is, the node must be non-full when this
// function is called
void BTree::Node::insertNonFull(const rule_spec_t & rule) {
	// Initialize index as index of rightmost element
	int i = int(key_cnt) - 1;
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
		set_key<uint32_t>(i + 1, KeyInfo(k, rule.second, INVALID_INDEX));
		set_key_cnt(key_cnt + 1);
	} else {
		// If this node is not leaf

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

// https://www.geeksforgeeks.org/b-tree-set-1-insert-2/
void BTree::insert(const rule_spec_t & rule) {

	// If tree is empty
	if (root == nullptr) {
		// Allocate memory for root
		root = new Node();
		root->set_key<uint32_t>(0, KeyInfo(rule.first[0], rule.second, INVALID_INDEX)); // Insert key
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
			s->insertNonFull(rule);
			//int i = 0;
			//if (s->get_key<uint32_t>(0) < rule.first[0])
			//	i++;
			//s->child(i).insertNonFull(rule);
			// Change root
			root = s;
		} else
			// If root is not full, call insertNonFull for root
			root->insertNonFull(rule);
	}
}

void BTree::Node::splitChild(int i, Node & y) {
	// y - left
	// this - parent
	// z - right
	assert(y.key_cnt == MAX_DEGREE);
	// Create a new node which is going to store (t-1) keys
	// of y
	Node *z = new Node;
	z->is_leaf = y.is_leaf;
	z->set_key_cnt(MIN_DEGREE - 1);

	// Copy the last (MIN_DEGREE-1) keys of y to z
	for (int j = 0; j < int(MIN_DEGREE) - 1; j++) {
		z->set_key<uint32_t>(j, y.get_key<uint32_t>(MIN_DEGREE + j + 1));
	}

	if (not y.is_leaf) {
		for (unsigned j = 0; j < MIN_DEGREE; j++) {
			// Copy the last t children of y to z
			z->child_index[j] = y.child_index[MIN_DEGREE + j + 1];
		}
	}

	// Subtract number of keys moved from y to z
	y.set_key_cnt(MIN_DEGREE);

	assert(key_cnt < MAX_DEGREE);
	// Since this node is going to have a new child,
	// create space of new child
	for (int j = key_cnt; j >= i + 1; j--)
		child_index[j + 1] = child_index[j];

	// Link the new child to this node
	set_child(i + 1, z);

	// A key of y will move to this node. Find location of
	// new key and move all greater keys one space ahead
	for (int j = key_cnt - 1; j >= i + 1; j--)
		set_key<uint32_t>(j + 1, get_key<uint32_t>(j));

	// Copy the middle key of y to this node
	set_key<uint32_t>(i, y.get_key<uint32_t>(MIN_DEGREE));

	// Increment count of keys in this node
	set_key_cnt(key_cnt + 1);
	//assert(key_cnt >= MIN_DEGREE);
}

}
