#pragma once
#include <vector>
#include <algorithm>
#include <pcv/common/range.h>

namespace pcv {
/*
 * @note based on https://www.geeksforgeeks.org/weighted-job-scheduling/
 *
 * @attention there is a difference between the original alg as the exactly same intervals are merged
 * 			and theirs weight is added
 * 			(the weight function is 1 for any interval)
 * */
template<typename VALUE_T>
class WeightedIntervalSchedulingSolver {
public:
	using Interval = Range1d<VALUE_T>;
	// @note the intervals will be sorted at the beginning
	std::vector<Interval> & intervals;
	int *table;

	WeightedIntervalSchedulingSolver(std::vector<Interval> & intervals) :
			intervals(intervals) {
		table = new int[intervals.size()];
	}

	static bool intervalComparataor(Interval s1, Interval s2) {
		return (s1.high < s2.high);
	}

	// Find the latest interval (in sorted array) that doesn't
	// conflict with the interval[i]
	int latestNonConflict(int i) {
		for (int j = i - 1; j >= 0; j--) {
			//if (intervals[j].high <= intervals[i].high)
			//	return j;
			// if the interval is exactly the same it is interpreted as non conflicting
			// because in our scenario the exactly same intervals can be merged
			if (intervals[j].high <= intervals[i].low
					or intervals[j] == intervals[i])
				return j;
		}
		return -1;
	}

	inline int get_weight(int i) {
		// if the interval is same as its predecessor increase it's gain
		if (i > 0 and intervals[i - 1] == intervals[i]) {
			return 2;
		} else {
			return 1;
		}
	}

	// The main function that returns the maximum possible
	// profit from given array of jobs
	int findMaxWeightIntervalSequence() {
		// Sort jobs according to finish time
		std::sort(intervals.begin(), intervals.end(),
				WeightedIntervalSchedulingSolver::intervalComparataor);

		// Create an array to store solutions of subproblems.  table[i]
		// stores the profit for jobs till intervals[i] (including intervals[i])
		table[0] = get_weight(0);

		// Fill entries in M[] using recursive property
		for (size_t i = 1; i < intervals.size(); i++) {
			// Find profit including the current interval
			int inclProf = get_weight(i);
			int l = latestNonConflict(i);
			if (l != -1)
				inclProf += table[l];

			// Store maximum of including and excluding
			table[i] = std::max(inclProf, table[i - 1]);
		}

		// Store result and free dynamic memory allocated for table[]
		int result = table[intervals.size() - 1];
		return result;
	}
	~WeightedIntervalSchedulingSolver() {
		delete[] table;
	}
};
}
