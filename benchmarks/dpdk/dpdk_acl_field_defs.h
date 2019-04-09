#pragma once

#include <rte_acl.h>
#include <rte_ip.h>

/*
 * Rule and trace formats definitions.
 */

enum {
	PROTO_FIELD_IPV4,
	SRC_FIELD_IPV4,
	DST_FIELD_IPV4,
	SRCP_FIELD_IPV4,
	DSTP_FIELD_IPV4,
	NUM_FIELDS_IPV4
};

///*
// * That effectively defines order of IPV4VLAN classifications:
// *  - PROTO
// *  //- VLAN (TAG and DOMAIN)
// *  - SRC IP ADDRESS
// *  - DST IP ADDRESS
// *  - PORTS (SRC and DST)
// */
//enum {
//	RTE_ACL_IPV4VLAN_PROTO,
//	RTE_ACL_IPV4VLAN_VLAN,
//	RTE_ACL_IPV4VLAN_SRC,
//	RTE_ACL_IPV4VLAN_DST,
//	RTE_ACL_IPV4VLAN_PORTS,
//	RTE_ACL_IPV4VLAN_NUM
//};

extern struct rte_acl_field_def ipv4_defs[NUM_FIELDS_IPV4];

#define	IPV6_ADDR_LEN	16
#define	IPV6_ADDR_U16	(IPV6_ADDR_LEN / sizeof(uint16_t))
#define	IPV6_ADDR_U32	(IPV6_ADDR_LEN / sizeof(uint32_t))

enum {
	PROTO_FIELD_IPV6,
	SRC1_FIELD_IPV6,
	SRC2_FIELD_IPV6,
	SRC3_FIELD_IPV6,
	SRC4_FIELD_IPV6,
	DST1_FIELD_IPV6,
	DST2_FIELD_IPV6,
	DST3_FIELD_IPV6,
	DST4_FIELD_IPV6,
	SRCP_FIELD_IPV6,
	DSTP_FIELD_IPV6,
	NUM_FIELDS_IPV6
};

extern struct rte_acl_field_def ipv6_defs[NUM_FIELDS_IPV6];

/*
  * Forward port info save in ACL lib starts from 1
  * since ACL assume 0 is invalid.
  * So, need add 1 when saving and minus 1 when forwarding packets.
  */
#define FWD_PORT_SHIFT 1

RTE_ACL_RULE_DEF(acl4_rule, RTE_DIM(ipv4_defs));
RTE_ACL_RULE_DEF(acl6_rule, RTE_DIM(ipv6_defs));
