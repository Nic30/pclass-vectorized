#include "dpdk_acl_field_defs.h"

// input_index: all input fields, except the very first one, must be in groups of 4 consecutive bytes.
// The input index specifies to which input group that field belongs to.
struct rte_acl_field_def ipv4_defs[NUM_FIELDS_IPV4] = {
    {
    	.type = RTE_ACL_FIELD_TYPE_BITMASK,
    	.size = sizeof(uint8_t),
    	.field_index = PROTO_FIELD_IPV4,
    	.input_index = 0,
    	.offset = offsetof(struct ipv4_tuple, proto),
    },
    {
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC_FIELD_IPV4,
		.input_index = 1,
		.offset = offsetof(struct ipv4_tuple, sip),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST_FIELD_IPV4,
		.input_index = 2,
		.offset = offsetof(struct ipv4_tuple, dip),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = SRCP_FIELD_IPV4,
		.input_index = 3,
		.offset = offsetof(struct ipv4_tuple, srcp),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = DSTP_FIELD_IPV4,
		.input_index = 3,
		.offset = offsetof(struct ipv4_tuple, dstp),
	},

};

struct rte_acl_field_def ipv6_defs[NUM_FIELDS_IPV6] = {
	{
		.type = RTE_ACL_FIELD_TYPE_BITMASK,
		.size = sizeof(uint8_t),
		.field_index = PROTO_FIELD_IPV6,
		.input_index = PROTO_FIELD_IPV6,
		.offset = 0,
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC1_FIELD_IPV6,
		.input_index = SRC1_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, src_addr) -
			offsetof(struct ipv6_hdr, proto),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC2_FIELD_IPV6,
		.input_index = SRC2_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, src_addr) -
			offsetof(struct ipv6_hdr, proto) + sizeof(uint32_t),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC3_FIELD_IPV6,
		.input_index = SRC3_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, src_addr) -
			offsetof(struct ipv6_hdr, proto) + 2 * sizeof(uint32_t),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = SRC4_FIELD_IPV6,
		.input_index = SRC4_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, src_addr) -
			offsetof(struct ipv6_hdr, proto) + 3 * sizeof(uint32_t),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST1_FIELD_IPV6,
		.input_index = DST1_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, dst_addr)
				- offsetof(struct ipv6_hdr, proto),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST2_FIELD_IPV6,
		.input_index = DST2_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, dst_addr) -
			offsetof(struct ipv6_hdr, proto) + sizeof(uint32_t),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST3_FIELD_IPV6,
		.input_index = DST3_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, dst_addr) -
			offsetof(struct ipv6_hdr, proto) + 2 * sizeof(uint32_t),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_MASK,
		.size = sizeof(uint32_t),
		.field_index = DST4_FIELD_IPV6,
		.input_index = DST4_FIELD_IPV6,
		.offset = offsetof(struct ipv6_hdr, dst_addr) -
			offsetof(struct ipv6_hdr, proto) + 3 * sizeof(uint32_t),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = SRCP_FIELD_IPV6,
		.input_index = SRCP_FIELD_IPV6,
		.offset = sizeof(struct ipv6_hdr) -
			offsetof(struct ipv6_hdr, proto),
	},
	{
		.type = RTE_ACL_FIELD_TYPE_RANGE,
		.size = sizeof(uint16_t),
		.field_index = DSTP_FIELD_IPV6,
		.input_index = SRCP_FIELD_IPV6,
		.offset = sizeof(struct ipv6_hdr) -
			offsetof(struct ipv6_hdr, proto) + sizeof(uint16_t),
	},
};
