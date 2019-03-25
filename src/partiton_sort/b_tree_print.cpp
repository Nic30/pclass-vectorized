#include <pcv/partiton_sort/b_tree.h>
#include <sstream>

using namespace std;

namespace pcv {

void BTree::print_to_stream(ostream & str, const Node & n) const {
	auto id = Node::_Mempool_t::getId(&n);
	/*
	 * <id>
	 * <key>[MAX_DEGREE]
	 * <child>[MAX_DEGREE]
	 **/

	str << "node" << id << " [label=\"{ <label> " << id << " ";
	if (n.parent) {
		str << "(parent:" << Node::_Mempool_t::getId(n.parent) << ")";
	}
	// print keys
	str << "|{";
	for (size_t i = 0; i < Node::MAX_DEGREE; i++) {
		if (i < n.key_cnt) {
			auto k = n.get_key<uint32_t>(i);
			auto f = str.flags();
			// << hex

			// range on first row rule id on second
			str << "{ <range" << i << ">" << k.key.low << "-" << k.key.high << " | ";
			if (k.value != INVALID_RULE)
				str << k.value;
			str << "}";
			str.flags(f);
		}
		bool last = i == Node::MAX_DEGREE - 1;
		if (not last)
			str << "|";
	}
	str << "}" << endl;

	// print children
	str << "|{";
	for (size_t i = 0; i < Node::MAX_DEGREE + 1; i++) {
		str << "<ch" << i << "> ch" << i;
		bool last = i == Node::MAX_DEGREE;
		if (not last)
			str << " | ";
	}
	str << "}" << endl;

	str << "}\"];";
	if (not n.is_leaf) {
		// print child nodes
		for (uint8_t i = 0; i < n.key_cnt + 1; i++) {
			auto ch = n.child(i);
			print_to_stream(str, *ch);
		}
		// print connections to them
		for (uint8_t i = 0; i < n.key_cnt + 1; i++) {
			str << "    node" << id << ":ch" << int(i) << " -> " << "node"
					<< n.child_index[i] << ":label;" << endl;
		}
	}
	for (size_t i = 0; i < n.key_cnt; i++) {
		auto nl = n.get_next_layer(i);
		if (nl) {
			print_to_stream(str, *nl);
			str << "    node" << id << ":range" << int(i) << " -> " << "node"
					<< n.next_level[i] << ":label;" << endl;
		}
	}

	str << endl;

}

// serialize graph to string in dot format
ostream & operator<<(ostream & str, const BTree & t) {
	str << "digraph layered_btree {" << endl;
	str << "    " << "node [shape=record];" << endl;
	if (t.root)
		t.print_to_stream(str, *t.root);
	str << "}";
	return str;
}

BTree::operator string() const {
	stringstream ss;
	ss << *this;
	return ss.str();
}

}
