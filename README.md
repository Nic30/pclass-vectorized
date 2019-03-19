# pclass-vectorized [![Build Status](https://travis-ci.org/Nic30/pclass-vectorized.svg?branch=master)](https://travis-ci.org/Nic30/pclass-vectorized)
Simple library of vectorized packet classification algorithms


## Content

* DPDK class. testing app

* sketch of layered b-tree class. alg. similar to [PartitonSort](https://github.com/sorrachai/PartitonSort) optimized for AVX2



### Layered B-trees

This data structure is composed of multiple trees. Each tree is divided in multiple levels where each level performs classification in a single dimension (field).

Layered B-tree is a B-tree where each item in node is an interval. Each interval may have pointer on tree in next level  in addition.

![Layered B-trees](/doc/layered_b-tree.png)

If the the item in current node matches and the next tree level pointer is invalid the last matching node with the rule specified is the matching node (and rule) in this tree.

In order to guarantee this the method from [PartitonSort](https://github.com/sorrachai/PartitonSort) and [SAX-PAC](https://dl.acm.org/citation.cfm?id=2626294) is used to split the rules in the trees.

The unique rules with the large number of dimensions may create long path trought the layers of tree. Each layer has only single node which is suoptimal. This path can be compresed if the dimension is specified for each item in the node. And there is enought of space for this as the data has to be aligned in order to be efficient with the AVX2.

