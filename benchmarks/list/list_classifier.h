#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <sstream>

#include <pcv/common/range.h>
#include <pcv/rule_parser/rule.h>
#include <pcv/partition_sort/b_tree.h>

namespace pcv {

template<typename _Key_t, size_t _D>
class ListBasedClassifier {
private:
	using BTree = _BTree<_Key_t, _D>;
public:
	using key_t = _Key_t;
	using rule_id_t = typename BTree::rule_id_t;
	static constexpr size_t D = _D;
	using key_range_t = typename BTree::key_range_t;
	using key_vec_t = typename BTree::key_vec_t;
	// print functions and key names for the debug
	using priority_t = typename BTree::priority_t;
	using rule_value_t = typename BTree::rule_value_t;
	using rule_spec_t = typename BTree::rule_spec_t;

	using formater_t = std::function<void(std::ostream & str, const rule_spec_t & rule)>;
	std::vector<rule_spec_t> rules;
	bool rules_sorted;
	const formater_t formater;

	ListBasedClassifier() :
			rules_sorted(true), formater(_default_formater) {
	}
	ListBasedClassifier(const formater_t & _formater) :
		rules_sorted(true), formater(_formater) {
	}
	inline void insert(const rule_spec_t & r) {
		rules.push_back(r);
		rules_sorted = false;
	}

	inline void prepare() {
		std::sort(rules.begin(), rules.end(),
				[](const rule_spec_t & a, const rule_spec_t & b) {
					return a.second.priority > b.second.priority;
				});
		rules_sorted = true;
	}

	rule_value_t search(const key_vec_t & v) const {
		if (not rules_sorted) {
			throw std::runtime_error("rules not prepared for the lookup");
		}
		for (auto & r : rules) {
			bool match = true;
			auto vIt = v.begin();
			auto rIt = r.first.begin();
			for (; rIt != r.first.end(); ++rIt, ++vIt) {
				if (!rIt->in_range(*vIt)) {
					match = false;
					break;
				}
			}
			if (match)
				return r.second;
		}
		return rule_value_t();
	}
	inline void remove(const rule_spec_t & r) {
		rules.erase(std::remove(rules.begin(), rules.end(), r), rules.end());
	}

	static void _default_formater(std::ostream & str,
			const rule_spec_t & rule) {
		for (auto v : rule.first) {
			rule_vec_format::rule_vec_format_default<key_t>(str, v);
			str << " ";
		}
		str << rule.second.rule_id;
	}

	// serialize list to classbench rules
	friend std::ostream & operator<<(std::ostream & str,
			const ListBasedClassifier & lc) {
		if (not lc.rules_sorted)
			throw std::runtime_error("rules are not prepared");
		size_t i = 0;
		for (auto r : lc.rules) {
			str << i << ": ";
			lc.formater(str, r);
			str << std::endl;
			i++;
		}
		return str;
	}
	operator std::string() const {
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}
};

}
