#include "classbench_rule_parser.h"

#include "dpdk_acl_field_defs.h"
#include "log.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define ACL_LEAD_CHAR		('@')
#define ROUTE_LEAD_CHAR		('R')
#define COMMENT_LEAD_CHAR	('#')
#define ACL_DENY_SIGNATURE	0xf0000000

#define GET_CB_FIELD(in, fd, base, lim, dlm)	do {            \
	unsigned long val;                                      \
	char *end;                                              \
	errno = 0;                                              \
	val = strtoul((in), &end, (base));                      \
	if (errno != 0 || end[0] != (dlm) || val > (lim))       \
		return -EINVAL;                               \
	(fd) = (typeof(fd))val;                                 \
	(in) = end + 1;                                         \
} while (0)
const char cb_port_delim[] = ":";

enum {
	CB_FLD_SRC_ADDR,
	CB_FLD_DST_ADDR,
	CB_FLD_SRC_PORT_LOW,
	CB_FLD_SRC_PORT_DLM,
	CB_FLD_SRC_PORT_HIGH,
	CB_FLD_DST_PORT_LOW,
	CB_FLD_DST_PORT_DLM,
	CB_FLD_DST_PORT_HIGH,
	CB_FLD_PROTO,
	CB_FLD_USERDATA,
	CB_FLD_NUM,
};

/* Bypass comment and empty lines */
static inline int is_bypass_line(char *buff) {
	int i = 0;

	/* comment line */
	if (buff[0] == COMMENT_LEAD_CHAR)
		return 1;
	/* empty line */
	while (buff[i] != '\0') {
		if (!isspace(buff[i]))
			return 0;
		i++;
	}
	return 1;
}

int add_rules(const char *rule_path, struct rte_acl_rule **proute_base,
		unsigned int *proute_num, struct rte_acl_rule **pacl_base,
		unsigned int *pacl_num, uint32_t rule_size,
		int (*parser)(char *, struct rte_acl_rule*, int)) {
	uint8_t *acl_rules, *route_rules;
	struct rte_acl_rule *next;
	unsigned int acl_num = 0, route_num = 0, total_num = 0;
	unsigned int acl_cnt = 0, route_cnt = 0;
	char buff[LINE_MAX];
	FILE *fh = fopen(rule_path, "rb");
	unsigned int i = 0;
	int val;

	if (fh == NULL)
		rte_exit(EXIT_FAILURE, "%s: Open %s failed\n", __func__, rule_path);

	while ((fgets(buff, LINE_MAX, fh) != NULL)) {
		if (buff[0] == ROUTE_LEAD_CHAR)
			route_num++;
		else if (buff[0] == ACL_LEAD_CHAR)
			acl_num++;
	}

	//if (0 == route_num)
	//	rte_exit(EXIT_FAILURE, "Not find any route entries in %s!\n",
	//			rule_path);

	val = fseek(fh, 0, SEEK_SET);
	if (val < 0) {
		rte_exit(EXIT_FAILURE, "%s: File seek operation failed\n", __func__);
	}

	acl_rules = (uint8_t*) calloc(acl_num, rule_size);

	if (NULL == acl_rules)
		rte_exit(EXIT_FAILURE, "%s: failed to malloc memory\n", __func__);

	route_rules = (uint8_t*) calloc(route_num, rule_size);

	if (NULL == route_rules)
		rte_exit(EXIT_FAILURE, "%s: failed to malloc memory\n", __func__);

	i = 0;
	while (fgets(buff, LINE_MAX, fh) != NULL) {
		i++;

		if (is_bypass_line(buff))
			continue;

		char s = buff[0];

		/* Route entry */
		if (s == ROUTE_LEAD_CHAR)
			next =
					(struct rte_acl_rule *) (route_rules + route_cnt * rule_size);

		/* ACL entry */
		else if (s == ACL_LEAD_CHAR)
			next = (struct rte_acl_rule *) (acl_rules + acl_cnt * rule_size);

		/* Illegal line */
		else
			rte_exit(EXIT_FAILURE, "%s Line %u: should start with leading "
					"char %c or %c\n", rule_path, i, ROUTE_LEAD_CHAR,
			ACL_LEAD_CHAR);

		if (parser(buff + 1, next, s == ROUTE_LEAD_CHAR) != 0)
			rte_exit(EXIT_FAILURE, "%s Line %u: parse rules error\n", rule_path,
					i);

		if (s == ROUTE_LEAD_CHAR) {
			/* Check the forwarding port number */
			next->data.userdata += FWD_PORT_SHIFT;
			route_cnt++;
		} else {
			next->data.userdata = ACL_DENY_SIGNATURE + acl_cnt;
			acl_cnt++;
		}

		next->data.priority = RTE_ACL_MAX_PRIORITY - total_num;
		next->data.category_mask = -1;
		total_num++;
	}

	fclose(fh);

	*pacl_base = (struct rte_acl_rule *) acl_rules;
	*pacl_num = acl_num;
	*proute_base = (struct rte_acl_rule *) route_rules;
	*proute_num = route_cnt;
	return 0;
}

/*
 * Parse ClassBench rules file.
 * Expected format:
 * '@'<src_ipv4_addr>'/'<masklen> <space> \
 * <dst_ipv4_addr>'/'<masklen> <space> \
 * <src_port_low> <space> ":" <src_port_high> <space> \
 * <dst_port_low> <space> ":" <dst_port_high> <space> \
 * <proto>'/'<mask>
 */
static int parse_ipv4_net(const char *in, uint32_t *addr, uint32_t *mask_len) {
	uint8_t a, b, c, d, m;

	GET_CB_FIELD(in, a, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, b, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, c, 0, UINT8_MAX, '.');
	GET_CB_FIELD(in, d, 0, UINT8_MAX, '/');
	GET_CB_FIELD(in, m, 0, sizeof(uint32_t) * CHAR_BIT, 0);

	addr[0] = IPv4(a, b, c, d);
	mask_len[0] = m;

	return 0;
}

int parse_cb_ipv4vlan_rule(char *str, struct rte_acl_rule *v,
		int has_userdata) {
	int i, rc;
	char *s, *sp, *in[CB_FLD_NUM];
	static const char *dlm = " \t\n";
	int dim = has_userdata ? CB_FLD_NUM : CB_FLD_USERDATA;
	s = str;

	for (i = 0; i != dim; i++, s = NULL) {
		in[i] = strtok_r(s, dlm, &sp);
		if (in[i] == NULL)
			return -EINVAL;
	}

	rc = parse_ipv4_net(in[CB_FLD_SRC_ADDR],
			&v->field[SRC_FIELD_IPV4].value.u32,
			&v->field[SRC_FIELD_IPV4].mask_range.u32);
	if (rc != 0) {
		acl_log("failed to read source address/mask: %s\n",
				in[CB_FLD_SRC_ADDR]);
		return rc;
	}

	rc = parse_ipv4_net(in[CB_FLD_DST_ADDR],
			&v->field[DST_FIELD_IPV4].value.u32,
			&v->field[DST_FIELD_IPV4].mask_range.u32);
	if (rc != 0) {
		acl_log("failed to read destination address/mask: %s\n",
				in[CB_FLD_DST_ADDR]);
		return rc;
	}

	GET_CB_FIELD(in[CB_FLD_SRC_PORT_LOW], v->field[SRCP_FIELD_IPV4].value.u16,
			0, UINT16_MAX, 0);
	GET_CB_FIELD(in[CB_FLD_SRC_PORT_HIGH],
			v->field[SRCP_FIELD_IPV4].mask_range.u16, 0, UINT16_MAX, 0);

	if (strncmp(in[CB_FLD_SRC_PORT_DLM], cb_port_delim, sizeof(cb_port_delim))
			!= 0)
		return -EINVAL;

	GET_CB_FIELD(in[CB_FLD_DST_PORT_LOW], v->field[DSTP_FIELD_IPV4].value.u16,
			0, UINT16_MAX, 0);
	GET_CB_FIELD(in[CB_FLD_DST_PORT_HIGH],
			v->field[DSTP_FIELD_IPV4].mask_range.u16, 0, UINT16_MAX, 0);

	if (strncmp(in[CB_FLD_DST_PORT_DLM], cb_port_delim, sizeof(cb_port_delim))
			!= 0)
		return -EINVAL;

	if (v->field[SRCP_FIELD_IPV4].mask_range.u16
			< v->field[SRCP_FIELD_IPV4].value.u16
			|| v->field[DSTP_FIELD_IPV4].mask_range.u16
					< v->field[DSTP_FIELD_IPV4].value.u16)
		return -EINVAL;

	GET_CB_FIELD(in[CB_FLD_PROTO], v->field[PROTO_FIELD_IPV4].value.u8, 0,
			UINT8_MAX, '/');
	GET_CB_FIELD(in[CB_FLD_PROTO], v->field[PROTO_FIELD_IPV4].mask_range.u8, 0,
			UINT8_MAX, 0);

	if (has_userdata)
		GET_CB_FIELD(in[CB_FLD_USERDATA], v->data.userdata, 0, UINT32_MAX, 0);

	return 0;
}

/*
 * Parses IPV6 address, exepcts the following format:
 * XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX (where X - is a hexedecimal digit).
 */
int parse_ipv6_addr(const char *in, const char **end,
		uint32_t v[IPV6_ADDR_U32], char dlm) {
	uint32_t addr[IPV6_ADDR_U16];

	GET_CB_FIELD(in, addr[0], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[1], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[2], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[3], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[4], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[5], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[6], 16, UINT16_MAX, ':');
	GET_CB_FIELD(in, addr[7], 16, UINT16_MAX, dlm);

	*end = in;

	v[0] = (addr[0] << 16) + addr[1];
	v[1] = (addr[2] << 16) + addr[3];
	v[2] = (addr[4] << 16) + addr[5];
	v[3] = (addr[6] << 16) + addr[7];

	return 0;
}

static int parse_ipv6_net(const char *in, struct rte_acl_field field[4]) {
	int32_t rc;
	const char *mp;
	uint32_t i, m, v[4];
	const uint32_t nbu32 = sizeof(uint32_t) * CHAR_BIT;

	/* get address. */
	rc = parse_ipv6_addr(in, &mp, v, '/');
	if (rc != 0)
		return rc;

	/* get mask. */
	GET_CB_FIELD(mp, m, 0, CHAR_BIT * sizeof(v), 0);

	/* put all together. */
	for (i = 0; i != RTE_DIM(v); i++) {
		if (m >= (i + 1) * nbu32)
			field[i].mask_range.u32 = nbu32;
		else
			field[i].mask_range.u32 = m > (i * nbu32) ? m - (i * 32) : 0;

		field[i].value.u32 = v[i];
	}

	return 0;
}
int parse_cb_ipv6_rule(char *str, struct rte_acl_rule *v, int has_userdata) {
	int i, rc;
	char *s, *sp, *in[CB_FLD_NUM];
	static const char *dlm = " \t\n";
	int dim = has_userdata ? CB_FLD_NUM : CB_FLD_USERDATA;
	s = str;

	for (i = 0; i != dim; i++, s = NULL) {
		in[i] = strtok_r(s, dlm, &sp);
		if (in[i] == NULL)
			return -EINVAL;
	}

	rc = parse_ipv6_net(in[CB_FLD_SRC_ADDR], v->field + SRC1_FIELD_IPV6);
	if (rc != 0) {
		acl_log("failed to read source address/mask: %s\n",
				in[CB_FLD_SRC_ADDR]);
		return rc;
	}

	rc = parse_ipv6_net(in[CB_FLD_DST_ADDR], v->field + DST1_FIELD_IPV6);
	if (rc != 0) {
		acl_log("failed to read destination address/mask: %s\n",
				in[CB_FLD_DST_ADDR]);
		return rc;
	}

	/* source port. */
	GET_CB_FIELD(in[CB_FLD_SRC_PORT_LOW], v->field[SRCP_FIELD_IPV6].value.u16,
			0, UINT16_MAX, 0);
	GET_CB_FIELD(in[CB_FLD_SRC_PORT_HIGH],
			v->field[SRCP_FIELD_IPV6].mask_range.u16, 0, UINT16_MAX, 0);

	if (strncmp(in[CB_FLD_SRC_PORT_DLM], cb_port_delim, sizeof(cb_port_delim))
			!= 0)
		return -EINVAL;

	/* destination port. */
	GET_CB_FIELD(in[CB_FLD_DST_PORT_LOW], v->field[DSTP_FIELD_IPV6].value.u16,
			0, UINT16_MAX, 0);
	GET_CB_FIELD(in[CB_FLD_DST_PORT_HIGH],
			v->field[DSTP_FIELD_IPV6].mask_range.u16, 0, UINT16_MAX, 0);

	if (strncmp(in[CB_FLD_DST_PORT_DLM], cb_port_delim, sizeof(cb_port_delim))
			!= 0)
		return -EINVAL;

	if (v->field[SRCP_FIELD_IPV6].mask_range.u16
			< v->field[SRCP_FIELD_IPV6].value.u16
			|| v->field[DSTP_FIELD_IPV6].mask_range.u16
					< v->field[DSTP_FIELD_IPV6].value.u16)
		return -EINVAL;

	GET_CB_FIELD(in[CB_FLD_PROTO], v->field[PROTO_FIELD_IPV6].value.u8, 0,
			UINT8_MAX, '/');
	GET_CB_FIELD(in[CB_FLD_PROTO], v->field[PROTO_FIELD_IPV6].mask_range.u8, 0,
			UINT8_MAX, 0);

	if (has_userdata)
		GET_CB_FIELD(in[CB_FLD_USERDATA], v->data.userdata, 0, UINT32_MAX, 0);

	return 0;
}
