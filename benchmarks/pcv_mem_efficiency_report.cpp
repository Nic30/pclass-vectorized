#include <vector>
#include <iostream>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/partition_sort_classifier.h>
#include <pcv/partition_sort/b_tree_node_navigator.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/partition_sort/rule_value_int.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

template<typename BTree>
class PartitionSortClassiferTree_toJson: _BTreeNodeNavigator<BTree> {
public:
	using Node = typename BTree::Node;
	std::ostream &out;
	PartitionSortClassiferTree_toJson(BTree &t, std::ostream &_out) :
			_BTreeNodeNavigator<BTree>(t), out(_out) {
	}
	void tree_to_json() {
		auto id = this->tree.node_allocator.getId(this->tree.root);
		out << "{" << "    \"root\": \"" << id << "\"," << std::endl;
		out << "    \"nodes\": {";
		nodes_to_json(*this->tree.root, true);
		out << "}}";
	}
	void nodes_to_json(const Node &n, bool is_first) {
		if (!is_first)
			out << "," << std::endl;
		auto id = this->tree.node_allocator.getId(&n);
		out << "\"" << id << "\" : {" << std::endl;
		out << "    \"key_cnt\":" << unsigned(n.key_cnt) << "," << std::endl;
		out << "    \"children\": [";

		bool first = true;
		for (size_t i = 0; i < unsigned(n.key_cnt) + 1; i++) {
			auto c = this->child(n, i);
			if (c) {
				if (!first) {
					out << ", ";
				} else {
					first = false;
				}
				auto c_id = this->tree.node_allocator.getId(c);
				out << "\"" << c_id << "\"";
			}
		}
		out << "    ]," << std::endl;
		out << "    \"next_layer\": [";
		first = true;
		for (size_t i = 0; i < unsigned(n.key_cnt); i++) {
			auto c = this->get_next_layer_const(n, i);
			if (c) {
				if (!first) {
					out << ", ";
				} else {
					first = false;
				}
				auto c_id = this->tree.node_allocator.getId(c);
				out << "\"" << c_id << "\"";
			}
		}
		out << "     ]" << std::endl << "}";
		for (size_t i = 0; i < unsigned(n.key_cnt) + 1; i++) {
			auto c = this->child(n, i);
			if (c) {
				nodes_to_json(*c, false);
			}
		}
		for (size_t i = 0; i < unsigned(n.key_cnt); i++) {
			auto c = this->get_next_layer_const(n, i);
			if (c) {
				nodes_to_json(*c, false);
			}
		}
	}
};

template<typename CLASSIFIER_T>
class PartitionSortClassifer_toJson {
public:
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
				PartitionSortClassiferTree_toJson<typename CLASSIFIER_T::Tree> t_to_json(
						ti->tree, out);
				t_to_json.tree_to_json();
			}
		}
		out << "]" << std::endl;
	}

};

/*
 * Benchmark which collect the statistics about memory efficiency in tree nodes
 * */
int main(int argc, const char *argv[]) {
	assert(argc == 1 + 1);
	const char *rule_file = argv[1];

	using BTree = BTreeImp<_BTreeCfg<uint16_t, RuleValueInt, 7, (1 << 16) - 1, 8>>;
	using Classifier = PartitionSortClassifer<BTree, 64, 10>;
	typename Classifier::NodeAllocator mem(1024);
	Classifier t(mem);

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
