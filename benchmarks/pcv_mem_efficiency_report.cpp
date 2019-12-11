#include <vector>
#include <iostream>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/rule_parser/classbench_rule_parser.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

template<typename CLASSIFIER_T>
class PartitionSortClassifer_toJson {
public:
	using Node = typename CLASSIFIER_T::Node;
	std::ostream &out;

	PartitionSortClassifer_toJson(std::ostream &_out) :
			out(_out) {
	}

	void classifier_to_json(const CLASSIFIER_T &cls) {
		out << "[" << std::endl;
		bool first = true;
		for (auto ti : cls.trees) {
			if (ti->tree.root) {
				if (first) {
					first = false;
				} else {
					out << "," << std::endl;
				}
				tree_to_json(ti->tree);
			}
		}
		out << "]" << std::endl;
	}

	void tree_to_json(const typename CLASSIFIER_T::Tree &t) {
		auto id = Node::_Mempool_t::getId(t.root);
		out << "{" << "    \"root\": \"" << id << "\"," << std::endl;
		out << "    \"nodes\": {";
		nodes_to_json(*t.root, true);
		out << "}}";
	}
	void nodes_to_json(const Node &n, bool is_first) {
		if (!is_first)
			out << "," << std::endl;
		auto id = Node::_Mempool_t::getId(&n);
		out << "\"" << id << "\" : {" << std::endl;
		out << "    \"key_cnt\":" << unsigned(n.key_cnt) << "," << std::endl;
		out << "    \"children\": [";

		bool first = true;
		for (size_t i = 0; i < unsigned(n.key_cnt) + 1; i++) {
			auto c = n.child(i);
			if (c) {
				if (!first) {
					out << ", ";
				} else {
					first = false;
				}
				auto c_id = Node::_Mempool_t::getId(c);
				out << "\"" << c_id << "\"";
			}
		}
		out << "    ]," << std::endl;
		out << "    \"next_layer\": [";
		first = true;
		for (size_t i = 0; i < unsigned(n.key_cnt); i++) {
			auto c = n.get_next_layer(i);
			if (c) {
				if (!first) {
					out << ", ";
				} else {
					first = false;
				}
				auto c_id = Node::_Mempool_t::getId(c);
				out << "\"" << c_id << "\"";
			}
		}
		out << "     ]" << std::endl << "}";
		for (size_t i = 0; i < unsigned(n.key_cnt) + 1; i++) {
			auto c = n.child(i);
			if (c) {
				nodes_to_json(*c, false);
			}
		}
		for (size_t i = 0; i < unsigned(n.key_cnt); i++) {
			auto c = n.get_next_layer(i);
			if (c) {
				nodes_to_json(*c, false);
			}
		}
	}
};

/*
 * Benchmark which collect the statistics about memory efficiency in tree nodes
 * */
int main(int argc, const char *argv[]) {
	assert(argc == 1 + 1);
	const char *rule_file = argv[1];

	using BTree = BTreeImp<_BTreeCfg<uint16_t, IntRuleValue, 7, (1 << 16) - 1, 8>>;
	using Classifier = PartitionSortClassifer<BTree, 64, 10>;
	Classifier t;

	// load rules from the file
	auto _rules = parse_ruleset_file(rule_file);
	// load rules in to a classifier tree
	size_t i = 0;
	for (auto _r : _rules) {
		auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r.first);
		BTree::rule_spec_t r = { rule_to_array_16b(*__r), {
				(Classifier::priority_t) __r->cummulative_prefix_len(),
				(Classifier::rule_id_t) _r.second } };
		//std::cout << *__r << std::endl;
		t.insert(r);
		i++;
	}

	PartitionSortClassifer_toJson<Classifier> toJson(std::cout);
	toJson.classifier_to_json(t);
	return 0;
}
