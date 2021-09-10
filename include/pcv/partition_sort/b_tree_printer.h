#pragma once

#include <string>
#include <array>
#include <functional>
#include <string>
#include <pcv/partition_sort/b_tree.h>
#include <pcv/partition_sort/b_tree_node_navigator.h>

namespace pcv {

template<typename BTree, typename formaters_t, typename names_t>
class BTreePrinter: public _BTreeNodeNavigator<BTree> {
	using _BTreeNodeNavigator<BTree>::_BTreeNodeNavigator;
public:
	using Node = typename BTree::Node;
	using key_t = typename BTree::key_t;

	const formaters_t &formaters;
	const names_t &names;

	BTreePrinter(BTree &_t, const formaters_t &_formaters,
			const names_t &_names) :
			_BTreeNodeNavigator<BTree>(_t), formaters(_formaters), names(_names) {
	}

	void print_to_stream(std::ostream &str, const Node &n) const {
		auto id = this->tree.node_allocator.getId(&n);
		/*
		 * <id>
		 * <key>[MAX_DEGREE]
		 * <child>[MAX_DEGREE]
		 **/

		str << "node" << id << " [label=\"{ <label> " << id << " ";
		if (n.parent) {
			str << "(parent:" << this->tree.node_allocator.getId(n.parent) << ")";
		}
		if (n.is_leaf) {
			str << " leaf";
		}
		if (n.is_compressed) {
			str << " compressed ";
		} else {
#ifndef NDEBUG
			str << " D=" << names.at(n.get_dim(0)) << "(" << int(n.get_dim(0))
					<< ")";
#endif
		}

		// print keys
		str << "|{";
		for (size_t i = 0; i < Node::MAX_DEGREE; i++) {
			if (i < n.key_cnt) {
				auto f = str.flags();
				// << hex

				// range on first row rule id on second
				str << "{ <range" << i << ">";
				auto d = n.get_dim(i);
				auto k = n.get_key(i);
				formaters.at(d)(str, k.key);
				if (n.is_compressed) {
					str << " D=" << names.at(d) << "(" << int(d) << ")";
				}
				str << " | ";
				if (k.value.is_valid())
					str << "p" << k.value.priority << " r" << k.value.rule_id;
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
				auto ch = this->child(n, i);
				print_to_stream(str, *ch);
			}
			// print connections to them
			for (uint8_t i = 0; i < n.key_cnt + 1; i++) {
				str << "    node" << id << ":ch" << int(i) << " -> " << "node"
						<< n.child_index[i] << ":label;" << std::endl;
			}
		}
		for (size_t i = 0; i < n.key_cnt; i++) {
			auto nl = this->get_next_layer_const(n, i);
			if (nl) {
				print_to_stream(str, *nl);
				str << "    node" << id << ":range" << int(i) << " -> "
						<< "node" << n.next_level[i] << ":label;" << std::endl;
			}
		}

		str << std::endl;

	}

	// serialize graph to string in dot format
	std::ostream& print_top(std::ostream &str, const BTree &t) {
		str << "digraph layered_btree {" << std::endl;
		str << "    " << "node [shape=record];" << std::endl;
		if (this->tree.root)
			print_to_stream(str, *this->tree.root);
		str << "}";
		return str;
	}

};

}
