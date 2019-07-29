#pragma once

#include <stddef.h>

namespace pcv {
/*
 *
 * @tparam _Key_t the type of key which will be used by the nodes in the tree
 * @tparam _Value_t the value stored in tree, take a look at IntRuleValue for a reference
 * @tparam _D maximal number of levels of the tree (number of fields in packet/dimensions)
 * @tparam _MAX_NODE_CNT maximal number of nodes for mempool preallocations
 * @tparam _T parameter which specifies the number of items per node
 * @tparam _PATH_COMPRESSION is true the path compression is enabled
 * 		and some of the nodes may be compressed as described
 * */
template<typename _Key_t, typename _Value_t, size_t _D, size_t _MAX_NODE_CNT =
		65535, size_t _T = 4, bool _PATH_COMPRESSION = true>
class _BTreeCfg {
public:
	using Key_t = _Key_t;
	using Value_t = _Value_t;
	static constexpr size_t D = _D;
	static constexpr size_t MAX_NODE_CNT = _MAX_NODE_CNT;
	static constexpr size_t T = _T;
	static constexpr bool PATH_COMPRESSION = _PATH_COMPRESSION;
};

}
