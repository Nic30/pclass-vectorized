# pclass-vectorized [![Build Status](https://travis-ci.org/Nic30/pclass-vectorized.svg?branch=master)](https://travis-ci.org/Nic30/pclass-vectorized)
Library of packet classification algorithms suitable for OpenFlow switches


## Content

* [prototype of packet classification](https://github.com/Nic30/pclass-vectorized/tree/master/include/pcv/partition_sort) alg. similar to [PartitonSort](https://github.com/sorrachai/PartitonSort) optimized for AVX2 and OvS
* [DPDK rte_acl testing app](https://github.com/Nic30/pclass-vectorized/tree/master/benchmarks/dpdk)
* [OvS v2.10 classifier compatiblity layer](https://github.com/Nic30/pclass-vectorized/tree/master/benchmarks/ovs/ovs_api)
* [classbench-ng rule parser](https://github.com/Nic30/pclass-vectorized/tree/master/include/pcv/rule_parser)
* [collection of packet classification benchmarks](https://github.com/Nic30/pclass-vectorized/tree/master/benchmarks)


### How to build and how to use
This library uses a meson build system. The Travis-ci uses [.travis.yml](https://github.com/Nic30/pclass-vectorized/blob/master/.travis.yml) to build the library and run the tests.

Dependencies for build with OvS (Ubuntu 16.04-19.10):
```bash
sudo apt install g++-8 gcc-8 boost1.67 ninja-build python3-pip python3-setuptools libunbound-dev openssl
sudo pip3 install meson
```

Build
```bash
pushd benchmarks/ovs/
bash ovs_prepare.sh
popd
mkdir build
meson build
cd build
meson configure -Dovs_test_app=true
ninja
```

Tests
```bash
ninja test
```

How to build OvS switch with this library.

* after previous step you should have a `libpcv.so` and `build/benchmarks/ovs/ovs_pcv/` build directory
* `ovs_pcv/` contains 
* in generated makefile there should be variables `vswitchd_ovs_vswitchd_OBJECTS` `vswitchd_ovs_vswitchd_LDADD`
  In order to build ovs with this library you have to manually substitute the objects produced from `benchmarks/ovs/ovs_api/`, add include directories and set `CC=g++`. All described is far from ideal. Integration of custom build target to a OvS autotools build is planed.

### Layered B-trees
Layered B-trees in this library is the [multidimensional interval tree](https://github.com/Nic30/pclass-vectorized/blob/master/include/pcv/partition_sort/b_tree.h#L67). 

Like the trie datastructure the Layered B-tree is diveded in to levels which performs [the classification on specific subset of bits](https://github.com/Nic30/pclass-vectorized/blob/master/include/pcv/partition_sort/b_tree_search.h#L312) from packet headers.

In the Layered B-tree each such a level is B-tree. In trie each set of bits is used as index to next level of the tree. In Layered B-tree the value is searched in the B-tree on current level.

Each interval stored in B-tree may have pointer [on tree in next level](https://github.com/Nic30/pclass-vectorized/blob/master/include/pcv/partition_sort/b_tree_node.h#L38) and value which represents the rule stored there.




#### Path compression
To prevent the long paths trough the levels of tree which contains nodes only with the single key it is possible to utilize path compression. Path compression takes segment of such a nodes and replaces it with the single node which contains the keys from all the nodes on path. 

The compressed nodes behave like a vector of the keys.

![Tree path compression](/doc/tree_path_compression.png)


### Partition Sort classifier with Layered B-trees

[PartitonSort](https://github.com/sorrachai/PartitonSort) is an algorithm for the packet classification. It uses multidimensional RB-trees. The trees are build on demand for groups of the rules which does overlap each other so each tree contains only the rules which does not overlap.

The rule is iteratively inserted to each tree until it can be inserted to some. After the rule is inserted the order of fields for the tree is recomputed and the tree reshaped if required. The order of fields is resolved by greedy heuristic which takes number of unique values and number of the shared values between the rules in account [SAX-PAC](https://dl.acm.org/citation.cfm?id=2626294).

The trees are ordered byt the max priority rule stored in them so it is posible to avoid checking some trees if they can not contain the rule whith higher priority than was found recently.

In this library the multidimensional RB-tree is replaced with the Layered B-tree with path compression.

![Layered B-trees](/doc/partition_srot_with_layered_b-tree.png)


### Debuging features

* serialization of the classifier as a tree in dot format (`operator std::string()` on `BTreeImp`)
* reverse conversion of the classifier back to rules `_BTreeToRules`


### Packet classification algorithm design and problems addressed

* Main datastructure of classifier: list of b-trees (`O(log n)` lookup/update time but weakness to rule replication, ideal field match order resolution problem in `NP`)

* Rule replication workaround (the problem where an overlapping rule can exponentially increase memory consumption): 

  * Multiple classifiers used to separate overlapping rules as much as possible in cost of potentionally additional lookup time [0].

* High dimmensionality tree construction method: Initial field match order for each tree which is incrementally improved with each rule added unless the tree grows to specified size. After this threshold the field match order is resolved only for fields not used by this tree [1]. Tree shape resolving alg. based on dynamic-programming based interval sheduler.

* Lookup speed improvements:

  * memory access optimisations:
     
     * tree node optimised to fit in cache lines, AVX2

     * memory pool and index of object instead of pointers to minimise the memory used on pointers in tree-nodes

     * on demand read of match fields because of tree structure

     * path compression

  * interval (the key of tree-node) format optimised for cache locality and AVX2


[0] Kirill Kogan, Sergey Nikolenko, Ori Rottenstreich, William Culhane, and Patrick Eugster. 2014. SAX-PAC (Scalable And eXpressive PAcket Classification). SIGCOMM Comput. Commun. Rev. 44, 4 (August 2014), 15-26. DOI: https://doi.org/10.1145/2740070.2626294 

[1] S. Yingchareonthawornchai, J. Daly, A. X. Liu and E. Torng, "A sorted partitioning approach to high-speed and fast-update OpenFlow classification," 2016 IEEE 24th International Conference on Network Protocols (ICNP), Singapore, 2016, pp. 1-10. doi: 10.1109/ICNP.2016.7784429
