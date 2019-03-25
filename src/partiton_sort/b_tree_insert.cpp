#include <pcv/partiton_sort/b_tree.h>

using namespace std;

namespace pcv {

BTree::Node::InsertCookie::InsertCookie(BTree & tree) :
		dimensio_order(tree.dimension_order), level(0) {
}

Range1d<BTree::value_t> BTree::Node::InsertCookie::get_actual_key(
		const rule_spec_t & rule) const {
	auto d = dimensio_order.at(level);
	return rule.first.at(d);
}

bool BTree::Node::InsertCookie::required_more_levels(
		const rule_spec_t & rule) const {
	if (level < dimensio_order.size()) {
		for (size_t i = level; i < dimensio_order.size(); i++) {
			if (not rule.first[dimensio_order[i]].is_wildcard())
				return true;
		}
	}
	return false;
}

// A utility function to insert a new key in this node
// The assumption is, the node must be non-full when this
// function is called
void BTree::Node::insertNonFull(const rule_spec_t & rule,
		InsertCookie & cookie) {
	// Initialize index as index of rightmost element
	int i = int(key_cnt) - 1;
	// If this is a leaf node
	auto k = cookie.get_actual_key(rule);
	if (is_leaf) {
		// The following loop does two things
		// a) Finds the location of new key to be inserted
		// b) Moves all greater keys to one place ahead
		while (i >= 0 && get_key<uint32_t>(i) > k) {
			move_key<uint32_t>(*this, i, *this, i + 1);
			i--;
		}

		// Insert the new key at found location
		set_key<uint32_t>(i + 1, KeyInfo(k, rule.second, INVALID_INDEX));
		set_key_cnt(key_cnt + 1);

		// continue on next layer if requiered
		if (cookie.required_more_levels(rule)) {
			cookie.level++;
			auto nl = get_next_layer(i + 1);
			nl = Node::insert_to_root(nl, rule, cookie);
			set_next_layer(i + 1, nl);
		}
	} else {
		// If this node is not leaf
		// Find the child which is going to have the new key
		while (i >= 0 && get_key<uint32_t>(i) > k)
			i--;

		i++;
		// See if the found child is full
		if (child(i)->key_cnt == MAX_DEGREE) {
			// If the child is full, then split it
			splitChild(i, *child(i));

			// After split, the middle key of C[i] goes up and
			// C[i] is splitted into two. See which of the two
			// is going to have the new key
			if (get_key<uint32_t>(i) < k)
				i++;
		}
		assert(i >= 0);
		child(i)->insertNonFull(rule, cookie);
	}
}

BTree::Node * BTree::Node::insert_to_root(Node * root, const rule_spec_t & rule,
		Node::InsertCookie & cookie) {
	auto k = cookie.get_actual_key(rule);
	// If tree is empty
	if (root == nullptr) {
		// Allocate memory for root
		root = new Node;
		root->set_key<uint32_t>(0, KeyInfo(k, rule.second, INVALID_INDEX)); // Insert key
		root->set_key_cnt(1);
		root->value[0] = rule.second;
		cookie.level++;
		if (cookie.required_more_levels(rule)) {
			auto nl = insert_to_root(root->get_next_layer(0), rule, cookie);
			root->set_next_layer(0, nl);
		}
	} else if (root->key_cnt == Node::MAX_DEGREE) {
		// If root is full, then tree grows in height
		// Allocate memory for new root
		Node *s = new Node;
		s->is_leaf = false;

		// Make old root as child of new root
		s->set_child(0, root);
		// Split the old root and move 1 key to the new root
		s->splitChild(0, *root);

		// New root has two children now.  Decide which of the
		// two children is going to have new key
		//s->insertNonFull(rule, cookie);
		int i = 0;
		if (s->get_key<uint32_t>(0) < k)
			i++;
		s->child(i)->insertNonFull(rule, cookie);
		// Change root
		root = s;
	} else {
		// If root is not full, call insertNonFull for root
		root->insertNonFull(rule, cookie);
	}
	return root;
}

void BTree::insert(const rule_spec_t & rule, Node::InsertCookie & cookie) {
	// search if there is some prefix already in decision tree because we do not
	// want to duplicate keys which are already present
	std::vector<std::pair<Node *, unsigned>> path;
	search_path(rule, path);
	if (path.size() > 0) {
		cookie.level += path.size();
		auto ins_root = root;
		auto b = path.back();
		ins_root = b.first->get_next_layer(b.second);
		ins_root = Node::insert_to_root(ins_root, rule, cookie);
		b.first->set_next_layer(b.second, ins_root);
	} else {
		root = Node::insert_to_root(root, rule, cookie);
	}
}

void BTree::Node::splitChild(unsigned i, Node & y) {
	// y - left
	// this - parent
	// z - right

	// Create a new node which is going to store (t-1) keys
	// of y
	Node *z = new Node;
	z->is_leaf = y.is_leaf;
	z->set_key_cnt(MIN_DEGREE);

	// Copy the last (MIN_DEGREE-1) keys of y to z
	for (unsigned j = 0; j < MIN_DEGREE; j++) {
		move_key<uint32_t>(y, MIN_DEGREE + j + 1, *z, j);
	}

	if (not y.is_leaf) {
		for (unsigned j = 0; j < MIN_DEGREE + 1; j++) {
			// Copy the last t children of y to z
			auto ch = y.child(MIN_DEGREE + j + 1);
			z->set_child(j, ch);
		}
	}

	// Subtract number of keys moved from y to z
	y.set_key_cnt(MIN_DEGREE);

	assert(key_cnt < MAX_DEGREE);
	// Since this node is going to have a new child,
	// create space of new child
	for (int j = key_cnt; j >= int(i + 1); j--) {
		auto ch = y.child(j);
		z->set_child(j + 1, ch);
	}

	// Link the new child to this node
	set_child(i + 1, z);

	// A key of y will move to this node. Find location of
	// new key and move all greater keys one space ahead
	for (int j = key_cnt - 1; j >= int(i); j--)
		move_key<uint32_t>(*this, j, *this, j + 1);

	// Copy the middle key of y to this node
	move_key<uint32_t>(y, MIN_DEGREE, *this, i);

	// Increment count of keys in this node
	set_key_cnt(key_cnt + 1);
}

}
