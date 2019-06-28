#pragma once
#include <vector>
#include <array>

#include <pcv/partition_sort/weighted_interval_scheduling_solver.h>
#include <pcv/common/range.h>

namespace pcv {

template<typename rule_spec_t, size_t DIM_CNT, typename key_t>
class GreedyDimensionOrderResolver {
	std::vector<rule_spec_t> & rules;
	// dimensions which are used by this rules
	// and are not been locked yet
	std::array<bool, DIM_CNT> dim_mask;
	const std::array<unsigned, DIM_CNT> & original_order;

public:
	/*
	 *
	 * @param original_order the order which is used as lowest priority key while sorting
	 * 	 to keep the order as stable as possible
	 * */
	GreedyDimensionOrderResolver(std::vector<rule_spec_t> & rules_,
			const std::array<unsigned, DIM_CNT> & original_order_) :
			rules(rules_), original_order(original_order_) {
		for (size_t i = 0; i < DIM_CNT; i++) {
			dim_mask[i] = false;
		}
	}
	int get_dimension_sel_score(unsigned d) {
		// and are most specific so the dimensions with the largest number
		// of the shared values will be resolved first in classifier

		// [note] the problem of picking of the largest sequence of non overlapping
		//        intervals is the Intervals Scheduling Problem (Weighted Job Scheduling Problem)

		std::vector<Range1d<key_t>> values(rules.size());
		for (size_t i = 0; i < rules.size(); i++) {
			values[i] = rules[i].first[d];
		}

		WeightedIntervalSchedulingSolver<key_t> sched(values);
		return sched.findMaxWeightIntervalSequence();
	}

	std::pair<std::array<unsigned, DIM_CNT>, size_t> resolve() {
		// resolve which dimensions are used
		for (auto & _r : rules) {
			auto & r = _r.first;
			for (size_t i = 0; i < DIM_CNT; i++) {
				dim_mask[i] |= not r[i].is_wildcard();
			}
		}
		// collect the indexes of the dimensions which are used
		std::array<unsigned, DIM_CNT> res_dim_order;
		std::vector<unsigned> pending_dims;
		size_t dim_resolved_cnt = 0;
		for (unsigned i = 0; i < DIM_CNT; i++) {
			if (dim_mask[i])
				pending_dims.push_back(i);
		}
		//auto dim_mask_bck = dim_mask;

		// for each used dimension pick one which was not locked by heuristic
		//while (pending_dims.size()) {
		//	//std::array<bool, DIM_CNT> dim_selection_score;
		//	//const size_t DIMs = pending_dims.size();
		//	//std::fill(dim_selection_score.begin(), dim_selection_score.begin() + DIMs, 0);
		//	auto best_dim_to_select = pending_dims[0];
		//	auto best_score = 0;
		//	for (auto d : pending_dims) {
		//		auto score = get_dimension_sel_score(d);
		//		if (score > best_score) {
		//			best_score = score;
		//			best_dim_to_select = d;
		//		}
		//	}
		//}
		std::vector<std::pair<unsigned, int>> scores;
		for (size_t i = 0; i < pending_dims.size(); i++) {
			auto d = pending_dims[i];
			auto score = get_dimension_sel_score(d);
			scores.push_back( { d, score });
		}
		std::sort(scores.begin(), scores.end(),
				[this](const std::pair<unsigned, int> & a, const std::pair<unsigned, int> & b) {
					if (a.second != b.second) {
						return a.second > b.second;
					} else {
						// use original order
						auto &oo = original_order;
						auto oa = std::find(oo.begin(),oo.end(), a.first);
						auto ob = std::find(oo.begin(),oo.end(), b.first);
						return oa < ob;
					}
				});
		for (auto s : scores) {
			res_dim_order[dim_resolved_cnt++] = s.first;
		}
		size_t used_dim_cnt = dim_resolved_cnt;
		// the rest of the dimensions which are not used by any the rules
		// will be added in orders based on their previous order
		for (unsigned i = 0; i < DIM_CNT; i++) {
			auto orig_d = original_order[i];
			assert(orig_d < DIM_CNT);
			if (not dim_mask[orig_d])
				res_dim_order[dim_resolved_cnt++] = orig_d;
		}
#ifndef NDEBUG
		// assert that the result contains only unique values and all dimensions
		for (unsigned i = 0; i < DIM_CNT; i++) {
			assert(
					std::find(res_dim_order.begin(), res_dim_order.end(), i)
							!= res_dim_order.end());
		}
#endif
		assert(dim_resolved_cnt == DIM_CNT);
		return {res_dim_order, used_dim_cnt};
	}

};

}
