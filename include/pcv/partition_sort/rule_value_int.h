#pragma once

#include <pcv/common/range.h>

namespace pcv {

/*
 * Basic implementation of value of a rule
 * This value is returned as a result of a search operation
 * */
struct RuleValueInt {
	// the identifier of the rule stored in tree
	using rule_id_t = uint32_t;
	using priority_t = uint32_t;

	static constexpr rule_id_t INVALID_RULE = (1 << 24) - 1;

	// invalid rule = rule with rule_id = INVALID_RULE and should have priority set to 0
	priority_t priority :8;
	rule_id_t rule_id :24;

	RuleValueInt() :
			priority(0), rule_id(INVALID_RULE) {
	}
	RuleValueInt(priority_t priority_, rule_id_t rule_id_) :
			priority(priority_), rule_id(rule_id_) {
	}
	template<typename T, size_t D>
	static std::array<T, D> get_rule_mask(
			const std::pair<std::array<Range1d<T>, D>, RuleValueInt> rule) {
		std::array<T, D> res;
		for (size_t i = 0; i < D; i++) {
			res[i] = rule.first[i].get_mask_le();
		}
		return res;
	}
	inline bool is_valid() const {
		return rule_id != INVALID_RULE;
	}
	inline bool operator==(const RuleValueInt &other) const {
		return priority == other.priority && rule_id == other.rule_id;
	}
	inline bool operator!=(const RuleValueInt &other) const {
		return priority != other.priority || rule_id != other.rule_id;
	}
};

}
