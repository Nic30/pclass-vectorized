#pragma once
#include <rte_acl.h>
#include <rte_ether.h>

#include "dpdk_acl_field_defs.h"

#define uint32_t_to_char(ip, a, b, c, d) do {\
		*a = (unsigned char)(ip >> 24 & 0xff);\
		*b = (unsigned char)(ip >> 16 & 0xff);\
		*c = (unsigned char)(ip >> 8 & 0xff);\
		*d = (unsigned char)(ip & 0xff);\
	} while (0)

static inline void
print_one_ipv4_rule(struct acl4_rule *rule, int extra)
{
	unsigned char a, b, c, d;

	uint32_t_to_char(rule->field[SRC_FIELD_IPV4].value.u32,
			&a, &b, &c, &d);
	printf("%hhu.%hhu.%hhu.%hhu/%u ", a, b, c, d,
			rule->field[SRC_FIELD_IPV4].mask_range.u32);
	uint32_t_to_char(rule->field[DST_FIELD_IPV4].value.u32,
			&a, &b, &c, &d);
	printf("%hhu.%hhu.%hhu.%hhu/%u ", a, b, c, d,
			rule->field[DST_FIELD_IPV4].mask_range.u32);
	printf("%hu : %hu %hu : %hu 0x%hhx/0x%hhx ",
		rule->field[SRCP_FIELD_IPV4].value.u16,
		rule->field[SRCP_FIELD_IPV4].mask_range.u16,
		rule->field[DSTP_FIELD_IPV4].value.u16,
		rule->field[DSTP_FIELD_IPV4].mask_range.u16,
		rule->field[PROTO_FIELD_IPV4].value.u8,
		rule->field[PROTO_FIELD_IPV4].mask_range.u8);
	if (extra)
		printf("0x%x-0x%x-0x%x ",
			rule->data.category_mask,
			rule->data.priority,
			rule->data.userdata);
}

static inline void
print_one_ipv6_rule(struct acl6_rule *rule, int extra)
{
	unsigned char a, b, c, d;

	uint32_t_to_char(rule->field[SRC1_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf("%.2x%.2x:%.2x%.2x", a, b, c, d);
	uint32_t_to_char(rule->field[SRC2_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf(":%.2x%.2x:%.2x%.2x", a, b, c, d);
	uint32_t_to_char(rule->field[SRC3_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf(":%.2x%.2x:%.2x%.2x", a, b, c, d);
	uint32_t_to_char(rule->field[SRC4_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf(":%.2x%.2x:%.2x%.2x/%u ", a, b, c, d,
			rule->field[SRC1_FIELD_IPV6].mask_range.u32
			+ rule->field[SRC2_FIELD_IPV6].mask_range.u32
			+ rule->field[SRC3_FIELD_IPV6].mask_range.u32
			+ rule->field[SRC4_FIELD_IPV6].mask_range.u32);

	uint32_t_to_char(rule->field[DST1_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf("%.2x%.2x:%.2x%.2x", a, b, c, d);
	uint32_t_to_char(rule->field[DST2_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf(":%.2x%.2x:%.2x%.2x", a, b, c, d);
	uint32_t_to_char(rule->field[DST3_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf(":%.2x%.2x:%.2x%.2x", a, b, c, d);
	uint32_t_to_char(rule->field[DST4_FIELD_IPV6].value.u32,
		&a, &b, &c, &d);
	printf(":%.2x%.2x:%.2x%.2x/%u ", a, b, c, d,
			rule->field[DST1_FIELD_IPV6].mask_range.u32
			+ rule->field[DST2_FIELD_IPV6].mask_range.u32
			+ rule->field[DST3_FIELD_IPV6].mask_range.u32
			+ rule->field[DST4_FIELD_IPV6].mask_range.u32);

	printf("%hu : %hu %hu : %hu 0x%hhx/0x%hhx ",
		rule->field[SRCP_FIELD_IPV6].value.u16,
		rule->field[SRCP_FIELD_IPV6].mask_range.u16,
		rule->field[DSTP_FIELD_IPV6].value.u16,
		rule->field[DSTP_FIELD_IPV6].mask_range.u16,
		rule->field[PROTO_FIELD_IPV6].value.u8,
		rule->field[PROTO_FIELD_IPV6].mask_range.u8);
	if (extra)
		printf("0x%x-0x%x-0x%x ",
			rule->data.category_mask,
			rule->data.priority,
			rule->data.userdata);
}

static inline void
dump_ipv4_rules(struct acl4_rule *rule, int num, int extra)
{
	int i;

	for (i = 0; i < num; i++, rule++) {
		printf("\t%d:", i + 1);
		print_one_ipv4_rule(rule, extra);
		printf("\n");
	}
}

static inline void
dump_ipv6_rules(struct acl6_rule *rule, int num, int extra)
{
	int i;

	for (i = 0; i < num; i++, rule++) {
		printf("\t%d:", i + 1);
		print_one_ipv6_rule(rule, extra);
		printf("\n");
	}
}
