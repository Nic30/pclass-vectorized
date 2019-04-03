#pragma once
#include <string>

namespace pcv {

template<typename BTree>
class BTreePrinter {
public:
	using Node = typename BTree::Node;
	using value_t = typename BTree::value_t;

	static void print_to_stream(std::ostream & str, const Node & n) {
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
		if (n.is_leaf) {
			str << " leaf";
		}
		if (n.is_compressed) {
			str << " compressed ";
		}
		// print keys
		str << "|{";
		for (size_t i = 0; i < Node::MAX_DEGREE; i++) {
			if (i < n.key_cnt) {
				auto k = n.get_key(i);
				auto f = str.flags();
				// << hex

				// range on first row rule id on second
				str << "{ <range" << i << ">" << k.key.low << "-" << k.key.high;
				if (n.is_compressed) {
					str << " D" << int(n.get_dim(i));
				}
				str << " | ";
				if (k.value != BTree::INVALID_RULE)
					str << k.value;
				str << "}";
				str.flags(f);
			}
			bool last = i == Node::MAX_DEGREE - 1;
			if (not last)
				str << "|";
		}
		str << "}" << std::endl;

		// print children
		str << "|{";
		for (size_t i = 0; i < Node::MAX_DEGREE + 1; i++) {
			str << "<ch" << i << "> ch" << i;
			bool last = i == Node::MAX_DEGREE;
			if (not last)
				str << " | ";
		}
		str << "}" << std::endl;

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
						<< n.child_index[i] << ":label;" << std::endl;
			}
		}
		for (size_t i = 0; i < n.key_cnt; i++) {
			auto nl = n.get_next_layer(i);
			if (nl) {
				print_to_stream(str, *nl);
				str << "    node" << id << ":range" << int(i) << " -> "
						<< "node" << n.next_level[i] << ":label;" << std::endl;
			}
		}

		str << std::endl;

	}

	// serialize graph to string in dot format
	static std::ostream & print_top(std::ostream & str, const BTree & t) {
		str << "digraph layered_btree {" << std::endl;
		str << "    " << "node [shape=record];" << std::endl;
		if (t.root)
			print_to_stream(str, *t.root);
		str << "}";
		return str;
	}

};

}
