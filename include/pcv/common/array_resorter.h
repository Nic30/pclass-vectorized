#pragma once
#include <set>

namespace pcv {

template<typename T, size_t ITEM_CNT>
inline void assert_all_items_unique(std::array<T, ITEM_CNT> &arr) {
	std::set<typename T::value_type> item_set;
	for (auto i : arr)
		item_set.insert(i);
	assert(item_set.size() == ITEM_CNT);
}
/*
 * Resort the trees if maximum priority of the tree has changed
 * (sorting alg is stable = the order of equal items is not changed)
 *
 * @param changed_item_index the index of the item which did changed
 * */
template<typename ITEM_T, size_t MAX_SIZE, typename KEY_GETTER>
void array_resort(std::array<ITEM_T, MAX_SIZE> &arr, size_t actual_size,
		const size_t changed_item_index) {

	KEY_GETTER key;
	if (actual_size == 1)
		return; // nothing to sort

	auto tmp = arr[changed_item_index];

	auto p = key(tmp);
	// try to move it to the left it has larger max priority rule
	if (changed_item_index > 0) {
		size_t i = changed_item_index;
		// while predecesor has lower value of key
		// find the index of the first item with the same or greater key value
		while (i >= 1 and key(arr[i - 1]) < p) {
			i--;
		}
		if (i != changed_item_index) {
			// shift all items with the lower max priority one to right
			for (size_t i2 = changed_item_index; i2 > i; i2--) {
				arr[i2] = std::move(arr[i2 - 1]);
			}
			// put the actual item on correct place
			arr[i] = std::move(tmp);
#ifndef NDEBUG
			assert_all_items_unique(arr);
#endif
			return;
		}
	}

	// try to move the item to the right if it has lower key value
	if (changed_item_index < actual_size - 1) {
		size_t i = changed_item_index;
		// while successor has greater priority
		while (i < actual_size - 1 and key(arr[i + 1]) > p) {
			i++;
		}
		if (i != changed_item_index) {
			// shift all items with the greater priority one to left
			for (size_t i2 = changed_item_index; i2 < i; i2++) {
				arr[i2] = std::move(arr[i2 + 1]);
			}
			// put the actual item on correct place
			arr[i] = std::move(tmp);
#ifndef NDEBUG
			assert_all_items_unique(arr);
#endif
			return;
		}
	}
}

}
