# pclass-vectorized [![Build Status](https://travis-ci.org/Nic30/pclass-vectorized.svg?branch=master)](https://travis-ci.org/Nic30/pclass-vectorized)
Library of vectorized packet classification algorithms


## Content

* prototype of packet classification alg. similar to [PartitonSort](https://github.com/sorrachai/PartitonSort) optimized for AVX2
* DPDK rte_acl testing app
* OvS v2.10 classifier compatible wrapper / compatiblity layer


### Layered B-trees
Layered B-trees in this library is the multidimensional interval tree. 

Like the trie datastructure the Layered B-tree is diveded in to levels which performs the classification on specific number of bits.

In the Layered B-tree each such a level is B-tree. In trie each set of bits is used as index to next level of the tree. In Layered B-tree the value is searched in the B-tree on current level.

Each interval stored in B-tree may have pointer on tree in next level and value which represents the rule stored there.

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

