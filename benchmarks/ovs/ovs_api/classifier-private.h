#pragma once

#include "classifier.h"
#undef atomic_init
#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak

#include <byteswap.h>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/partition_sort/partition_sort_classifier.h>

constexpr size_t DIM_CNT = 193;
using BTree = pcv::BTreeImp<uint16_t, DIM_CNT, 8, false>;
using Classifier = pcv::PartitionSortClassifer<BTree, 64, 10>;

struct classifier_priv {
	Classifier cls;
	std::unordered_map<Classifier::rule_id_t, const cls_rule *> to_rule;
	std::unordered_map<const struct cls_rule*, Classifier::rule_spec_t> to_pcv_rule;
};

namespace vec_build {
using pcv::Range1d;
using R = Range1d<uint16_t>;

/***************************************************************************************/
inline void push_8(const uint8_t & v, const uint8_t & m, int & i, R * dst) {
	dst[i++] = R::from_mask(v, m);
}
inline void pop_8(uint8_t & v, uint8_t & m, int & i, const R * src) {
	uint8_t low = src[i].low;
	uint8_t high = src[i].high;
	Range1d<uint8_t> r(low, high);
	v = r.low;
	m = r.get_mask_le();
	i++;
}
inline void push_8(const uint8_t & v, int & i, uint16_t * dst) {
	dst[i++] = v;
}
inline void pop_8(uint8_t & v, int & i, const uint16_t * src) {
	v = src[i++];
}

/***************************************************************************************/
inline void push_16(const uint16_t & v, const uint16_t & m, int & i, R* dst) {
	dst[i++] = R::from_mask(v, m);
}
inline void pop_16(uint16_t & v, uint16_t & m, int & i, const R* src) {
	uint8_t low = src[i].low;
	uint8_t high = src[i].high;
	Range1d<uint16_t> r(low, high);
	v = r.low;
	m = r.get_mask_le();
	i++;
}
inline void push_16be(const ovs_be16 & v, const ovs_be16 & m, int & i, R* dst) {
	dst[i++] = R::from_mask(__swab16p(&v), __swab16p(&m));
}
inline void pop_16be(ovs_be16 & v, ovs_be16 & m, int & i, const R* src) {
	auto s = src[i];
	s = s.to_be();
	v = s.low;
	m = s.get_mask_be();
}
inline void push_16(const uint16_t & v, int & i, uint16_t* dst) {
	dst[i++] = v;
}
inline void pop_16(uint16_t & v, int & i, const uint16_t* src) {
	v = src[i++];
}
inline void push_16be(const ovs_be16 & v, int & i, uint16_t* dst) {
	dst[i++] = __swab16p(&v);
}
inline void pop_16be(ovs_be16 & v, int & i, const uint16_t* src) {
	v = __swab16p(&src[i++]);
}

/***************************************************************************************/
inline void push_32(const uint32_t & v, const uint32_t & m, int & i, R* dst) {
	for (int i2 = 1; i2 >= 0; i2--)
		dst[i++] = R::from_mask(((uint16_t*) &v)[i2], ((uint16_t*) &m)[i2]);
}
inline void pop_32(uint32_t & v, uint32_t & m, int & i, const R* src) {
	uint32_t low = src[i].low;
	uint32_t high = src[i].high;
	low <<= 16;
	high <<= 16;
	low |= src[i + 1].low;
	high |= src[i + 1].high;

	Range1d<uint32_t> res(low, high);
	v = res.low;
	m = res.get_mask_le();
	i += 2;
}
inline void push_32be(const ovs_be32 & v, const ovs_be32 & m, int & i, R* dst) {
	uint32_t _v = __swab32p(&v);
	uint32_t _m = __swab32p(&m);
	for (int i2 = 0; i2 < 2; i2++)
		dst[i++] = R::from_mask(((uint16_t*) &_v)[i2], ((uint16_t*) &_m)[i2]);
}
inline void pop_32be(ovs_be32 & v, ovs_be32 & m, int & i, const R* src) {
	uint32_t low = src[i].low;
	uint32_t high = src[i].high;
	low <<= 16;
	high <<= 16;
	low |= src[i + 1].low;
	high |= src[i + 1].high;

	Range1d<uint32_t> res(low, high);
	res = res.to_be();
	v = res.low;
	m = res.get_mask_be();
	i += 2;
}

inline void push_32(const uint32_t & v, int & i, uint16_t* dst) {
	dst[i++] = v >> 16;
	dst[i++] = v & 0xffff;
}
inline void pop_32(uint32_t & v, int & i, const uint16_t* src) {
	v = src[i++];
	v <<= 16;
	v |= src[i++];
}
inline void push_32be(const ovs_be32 & v, int & i, uint16_t* dst) {
	uint32_t _v = __swab32p(&v);
	for (int i2 = 0; i2 < 2; i2++)
		dst[i++] = ((uint16_t*) &_v)[i2];
}
inline void pop_32be(ovs_be32 & v, int & i, const uint16_t* src) {
	v = src[i++];
	v <<= 16;
	v |= src[i++];
	__swab32s(&v);
}

/***************************************************************************************/
inline void push_eth_addr(const eth_addr & v, const eth_addr & m, int & i,
		R * dst) {

	for (int i2 = 2; i2 >= 0; i2--) {
		dst[i++] = R::from_mask(__swab16p(((uint16_t*) &v) + i2),
				__swab16p(((uint16_t*) &m) + i2));
	}
}
inline void pop_eth_addr(eth_addr & v, eth_addr & m, int & i, const R * src) {
	uint64_t low = 0;
	uint64_t high = 0;

	for (int i2 = 0; i2 < 3; i2++) {
		low |= src[i].low;
		high |= src[i].high;
		i++;
		// because of be, the first byte has to end on highest addr in this 64b word
		low <<= 16;
		high <<= 16;
	}
	Range1d<uint64_t> r(low, high);
	r = r.to_be();
	memcpy(&v, &r.low, sizeof(eth_addr));
	auto _m = r.get_mask_be();
	memcpy(&m, &_m, sizeof(eth_addr));
}

inline void push_eth_addr(const eth_addr & v, int & i, uint16_t * dst) {
	for (int i2 = 2; i2 >= 0; i2--) {
		dst[i++] = __swab16p(((uint16_t*) &v) + i2);
	}
}
inline void pop_eth_addr(eth_addr & v, int & i, const uint16_t * src) {
	uint64_t low = 0;

	for (int i2 = 0; i2 < 3; i2++) {
		low |= src[i];
		i++;
		low <<= 16;
	}
	low = bswap_64(low);
	memcpy(&v, &low, sizeof(eth_addr));
}

/***************************************************************************************/

inline void push_64be(const ovs_be64 & v, const ovs_be64 & m, int & i, R* dst) {
	uint64_t _v = bswap_64(v);
	uint64_t _m = bswap_64(m);
	for (int i2 = 0; i2 < 4; i2++)
		dst[i++] = R::from_mask(((uint16_t*) &_v)[i2], ((uint16_t*) &_m)[i2]);
}
inline void pop_64be(ovs_be64 & v, ovs_be64 & m, int & i, const R* src) {
	uint64_t low = 0;
	uint64_t high = 0;
	for (int i2 = 0; i2 < 4; i2++) {
		low <<= 16;
		high <<= 16;
		low |= src[i + i2].low;
		high |= src[i + i2].high;
	}

	Range1d<uint64_t> res(low, high);
	res = res.to_be();
	v = res.low;
	m = res.get_mask_be();
	i += 4;
}

inline void push_64be(const ovs_be64 & v, int & i, uint16_t* dst) {
	uint64_t _v = bswap_64(v);
	for (int i2 = 0; i2 < 4; i2++)
		dst[i++] = ((uint16_t*) &_v)[i2];
}
inline void pop_64be(ovs_be64 & v, int & i, const uint16_t* src) {
	uint64_t low = 0;
	for (int i2 = 0; i2 < 4; i2++) {
		low <<= 16;
		low |= src[i++];
	}
	low= bswap_64(low);
	v = low;
}

/***************************************************************************************/
inline void push_128be(const in6_addr & v, const in6_addr & m, int & i,
		R* dst) {
	for (int i2 = 0; i2 < 8; i2++)
		dst[i++] = R::from_mask(__swab16p(((uint16_t*) &v) + i2),
				__swab16p(((uint16_t*) &m) + i2));
}
inline void pop_128be(in6_addr & v, in6_addr & m, int & i, const R* src) {
	for (int i2 = 0; i2 < 8; i2++) {
		auto r = src[i++];
		r = r.to_be();
		((uint16_t*) &v)[i2] = r.low;
		((uint16_t*) &m)[i2] = r.get_mask_be();
	}
}
inline void push_128(const ovs_u128 & v, const ovs_u128 & m, int & i, R* dst) {
	for (int i2 = 7; i2 >= 0; i2--)
		dst[i++] = R::from_mask(((uint16_t*) &v)[i2], ((uint16_t*) &m)[i2]);
}
inline void pop_128(ovs_u128 & v, ovs_u128 & m, int & i, const R* src) {
	for (int i2 = 7; i2 >= 0; i2--) {
		auto r = src[i++];
		((uint16_t*) &v)[i2] = r.low;
		((uint16_t*) &m)[i2] = r.get_mask_le();
	}
}

inline void push_128be(const in6_addr & v, int & i, uint16_t* dst) {
	for (int i2 = 0; i2 < 8; i2++)
		dst[i++] = __swab16p(((uint16_t*) &v) + i2);
}
inline void pop_128be(in6_addr & v, int & i, const uint16_t* src) {
	for (int i2 = 0; i2 < 8; i2++) {
		((uint16_t*) &v)[i2] = __swab16p(&src[i++]);
	}
}
inline void push_128(const ovs_u128 & v, int & i, uint16_t* dst) {
	for (int i2 = 7; i2 >= 0; i2--)
		dst[i++] = ((uint16_t*) &v)[i2];
}
inline void pop_128(ovs_u128 & v, int & i, const uint16_t* src) {
	for (int i2 = 7; i2 >= 0; i2--) {
		((uint16_t*) &v)[i2] = src[i++];
	}
}

/***************************************************************************************/
inline void push_flow_tnl(const struct flow_tnl* f, const struct flow_tnl* m,
		int & i, R* dst) {
	push_32be(f->ip_dst, m->ip_dst, i, dst);
	push_128be(f->ipv6_dst, m->ipv6_dst, i, dst);
	push_32be(f->ip_src, m->ip_src, i, dst);
	push_128be(f->ipv6_src, m->ipv6_src, i, dst);
	assert(i == 20);
	push_64be(f->tun_id, m->tun_id, i, dst);
	push_16(f->flags, m->flags, i, dst);
	push_8(f->ip_tos, m->ip_tos, i, dst);
	push_8(f->ip_ttl, m->ip_ttl, i, dst);
	push_16be(f->tp_src, m->tp_src, i, dst);
	push_16be(f->tp_dst, m->tp_dst, i, dst);
	push_16be(f->gbp_id, m->gbp_id, i, dst);
	assert(i == 30);
	push_8(f->gbp_flags, m->gbp_flags, i, dst);
	push_8(f->erspan_ver, m->erspan_ver, i, dst);
	push_32(f->erspan_idx, m->erspan_idx, i, dst);
	push_8(f->erspan_dir, m->erspan_dir, i, dst);
	push_8(f->erspan_hwid, m->erspan_hwid, i, dst);
	// [TODO] metadata
}
inline void pop_flow_tnl(struct flow_tnl* f, struct flow_tnl* m, int & i,
		const R* a) {
	pop_32be(f->ip_dst, m->ip_dst, i, a);
	pop_128be(f->ipv6_dst, m->ipv6_dst, i, a);
	pop_32be(f->ip_src, m->ip_src, i, a);
	pop_128be(f->ipv6_src, m->ipv6_src, i, a);
	pop_64be(f->tun_id, m->tun_id, i, a);
	pop_16(f->flags, m->flags, i, a);
	pop_8(f->ip_tos, m->ip_tos, i, a);
	pop_8(f->ip_ttl, m->ip_ttl, i, a);
	pop_16be(f->tp_src, m->tp_src, i, a);
	pop_16be(f->tp_dst, m->tp_dst, i, a);
	pop_16be(f->gbp_id, m->gbp_id, i, a);
	pop_8(f->gbp_flags, m->gbp_flags, i, a);
	pop_8(f->erspan_ver, m->erspan_ver, i, a);
	pop_32(f->erspan_idx, m->erspan_idx, i, a);
	pop_8(f->erspan_dir, m->erspan_dir, i, a);
	pop_8(f->erspan_hwid, m->erspan_hwid, i, a);
	// [TODO] metadata
}

inline void push_flow_tnl(const struct flow_tnl* f, int & i, uint16_t* dst) {
	push_32be(f->ip_dst, i, dst);
	push_128be(f->ipv6_dst, i, dst);
	push_32be(f->ip_src, i, dst);
	push_128be(f->ipv6_src, i, dst);
	push_64be(f->tun_id, i, dst);
	push_16(f->flags, i, dst);
	push_8(f->ip_tos, i, dst);
	push_8(f->ip_ttl, i, dst);
	push_16be(f->tp_src, i, dst);
	push_16be(f->tp_dst, i, dst);
	push_16be(f->gbp_id, i, dst);
	push_8(f->gbp_flags, i, dst);
	push_8(f->erspan_ver, i, dst);
	push_32(f->erspan_idx, i, dst);
	push_8(f->erspan_dir, i, dst);
	push_8(f->erspan_hwid, i, dst);
	// [TODO] metadata
}
inline void pop_flow_tnl(struct flow_tnl* f, int & i, const uint16_t* a) {
	pop_32be(f->ip_dst, i, a);
	pop_128be(f->ipv6_dst, i, a);
	pop_32be(f->ip_src, i, a);
	pop_128be(f->ipv6_src, i, a);
	pop_64be(f->tun_id, i, a);
	pop_16(f->flags, i, a);
	pop_8(f->ip_tos, i, a);
	pop_8(f->ip_ttl, i, a);
	pop_16be(f->tp_src, i, a);
	pop_16be(f->tp_dst, i, a);
	pop_16be(f->gbp_id, i, a);
	pop_8(f->gbp_flags, i, a);
	pop_8(f->erspan_ver, i, a);
	pop_32(f->erspan_idx, i, a);
	pop_8(f->erspan_dir, i, a);
	pop_8(f->erspan_hwid, i, a);
	// [TODO] metadata
}

/***************************************************************************************/
inline void push_ovs_key_nsh(const ovs_key_nsh & v, const ovs_key_nsh & m,
		int & i, R * dst) {
	push_8(v.flags, m.flags, i, dst);
	push_8(v.ttl, m.ttl, i, dst);
	push_8(v.mdtype, m.mdtype, i, dst);
	push_8(v.np, m.np, i, dst);
	push_32be(v.path_hdr, m.path_hdr, i, dst);
}
inline void pop_ovs_key_nsh(ovs_key_nsh & v, ovs_key_nsh & m, int & i,
		const R * dst) {
	pop_8(v.flags, m.flags, i, dst);
	pop_8(v.ttl, m.ttl, i, dst);
	pop_8(v.mdtype, m.mdtype, i, dst);
	pop_8(v.np, m.np, i, dst);
	pop_32be(v.path_hdr, m.path_hdr, i, dst);
}

inline void push_ovs_key_nsh(const ovs_key_nsh & v, int & i, uint16_t * dst) {
	push_8(v.flags, i, dst);
	push_8(v.ttl, i, dst);
	push_8(v.mdtype, i, dst);
	push_8(v.np, i, dst);
	push_32be(v.path_hdr, i, dst);
}
inline void pop_ovs_key_nsh(ovs_key_nsh & v, int & i, const uint16_t * dst) {
	pop_8(v.flags, i, dst);
	pop_8(v.ttl, i, dst);
	pop_8(v.mdtype, i, dst);
	pop_8(v.np, i, dst);
	pop_32be(v.path_hdr, i, dst);
}

/***************************************************************************************/
inline void flow_from_array(const std::array<R, DIM_CNT> & a, struct flow * f,
		struct flow * m) {
	int i = 0;
	const R * r = &a[0];

	pop_flow_tnl(&f->tunnel, &m->tunnel, i, r);

	pop_64be(f->metadata, m->metadata, i, r);
	for (size_t i2 = 0; i2 < FLOW_N_REGS; i2++) {
		pop_32(f->regs[i2], m->regs[i2], i, r);
	}
	pop_32(f->skb_priority, m->skb_priority, i, r);
	pop_32(f->pkt_mark, m->pkt_mark, i, r);
	pop_32(f->dp_hash, m->dp_hash, i, r);
	pop_32(f->in_port.odp_port, m->in_port.odp_port, i, r);
	pop_32(f->recirc_id, m->recirc_id, i, r);
	pop_8(f->ct_state, m->ct_state, i, r);
	pop_8(f->ct_nw_proto, m->ct_nw_proto, i, r);
	pop_16(f->ct_zone, m->ct_zone, i, r);
	pop_32(f->ct_mark, m->ct_mark, i, r);
	pop_32be(f->packet_type, m->packet_type, i, r);
	pop_128(f->ct_label, m->ct_label, i, r);
	pop_32(f->conj_id, m->conj_id, i, r);
	pop_32(f->actset_output, m->actset_output, i, r);
	pop_eth_addr(f->dl_dst, m->dl_dst, i, r);
	pop_eth_addr(f->dl_src, m->dl_src, i, r);
	pop_16be(f->dl_type, m->dl_type, i, r);
	for (size_t i2 = 0; i2 < FLOW_MAX_VLAN_HEADERS; i2++) {
		pop_32be(f->vlans[i2].qtag, m->vlans[i2].qtag, i, r);
	}
	for (size_t i2 = 0; i2 < ROUND_UP(FLOW_MAX_MPLS_LABELS, 2); i2++) {
		pop_32be(f->mpls_lse[i2], m->mpls_lse[i2], i, r);
	}
	pop_32be(f->nw_src, m->nw_src, i, r);
	pop_32be(f->nw_dst, m->nw_dst, i, r);
	pop_32be(f->ct_nw_src, m->ct_nw_src, i, r);
	pop_32be(f->ct_nw_dst, m->ct_nw_dst, i, r);
	pop_128be(f->ipv6_src, m->ipv6_src, i, r);
	pop_128be(f->ipv6_dst, m->ipv6_dst, i, r);
	pop_128be(f->ct_ipv6_src, m->ct_ipv6_src, i, r);
	pop_128be(f->ct_ipv6_dst, m->ct_ipv6_dst, i, r);
	pop_32be(f->ipv6_label, m->ipv6_label, i, r);
	pop_8(f->nw_frag, m->nw_frag, i, r);
	pop_8(f->nw_tos, m->nw_tos, i, r);
	pop_8(f->nw_ttl, m->nw_ttl, i, r);
	pop_8(f->nw_proto, m->nw_proto, i, r);
	pop_128be(f->nd_target, m->nd_target, i, r);
	pop_eth_addr(f->arp_sha, m->arp_sha, i, r);
	pop_eth_addr(f->arp_tha, m->arp_tha, i, r);
	pop_16be(f->tcp_flags, m->tcp_flags, i, r);
	pop_ovs_key_nsh(f->nsh, m->nsh, i, r);

	pop_16be(f->tp_src, m->tp_src, i, r);
	pop_16be(f->tp_dst, m->tp_dst, i, r);
	pop_16be(f->ct_tp_src, m->ct_tp_src, i, r);
	pop_16be(f->ct_tp_dst, m->ct_tp_dst, i, r);
	pop_32be(f->igmp_group_ip4, m->igmp_group_ip4, i, r);
	assert(i == DIM_CNT);
}
inline void flow_to_array(const struct flow * f, const struct flow * m,
		std::array<R, DIM_CNT> & _r) {
	int i = 0;
	R * r = &_r[0];

	push_flow_tnl(&f->tunnel, &m->tunnel, i, r);
	assert(i == 36);

	push_64be(f->metadata, m->metadata, i, r);
	for (size_t i2 = 0; i2 < FLOW_N_REGS; i2++) {
		push_32(f->regs[i2], m->regs[i2], i, r);
	}
	push_32(f->skb_priority, m->skb_priority, i, r);
	push_32(f->pkt_mark, m->pkt_mark, i, r);
	push_32(f->dp_hash, m->dp_hash, i, r);
	push_32(f->in_port.odp_port, m->in_port.odp_port, i, r);
	push_32(f->recirc_id, m->recirc_id, i, r);
	push_8(f->ct_state, m->ct_state, i, r);
	push_8(f->ct_nw_proto, m->ct_nw_proto, i, r);
	push_16(f->ct_zone, m->ct_zone, i, r);
	push_32(f->ct_mark, m->ct_mark, i, r);
	push_32be(f->packet_type, m->packet_type, i, r);
	push_128(f->ct_label, m->ct_label, i, r);
	push_32(f->conj_id, m->conj_id, i, r);
	push_32(f->actset_output, m->actset_output, i, r);
	assert(i == 101);
	push_eth_addr(f->dl_dst, m->dl_dst, i, r);
	assert(i == 104);
	push_eth_addr(f->dl_src, m->dl_src, i, r);
	push_16be(f->dl_type, m->dl_type, i, r);
	for (size_t i2 = 0; i2 < FLOW_MAX_VLAN_HEADERS; i2++) {
		push_32be(f->vlans[i2].qtag, m->vlans[i2].qtag, i, r);
	}
	for (size_t i2 = 0; i2 < ROUND_UP(FLOW_MAX_MPLS_LABELS, 2); i2++) {
		push_32be(f->mpls_lse[i2], m->mpls_lse[i2], i, r);
	}
	assert(i == 120);
	push_32be(f->nw_src, m->nw_src, i, r);
	push_32be(f->nw_dst, m->nw_dst, i, r);
	push_32be(f->ct_nw_src, m->ct_nw_src, i, r);
	push_32be(f->ct_nw_dst, m->ct_nw_dst, i, r);
	push_128be(f->ipv6_src, m->ipv6_src, i, r);
	push_128be(f->ipv6_dst, m->ipv6_dst, i, r);
	push_128be(f->ct_ipv6_src, m->ct_ipv6_src, i, r);
	push_128be(f->ct_ipv6_dst, m->ct_ipv6_dst, i, r);
	push_32be(f->ipv6_label, m->ipv6_label, i, r);
	push_8(f->nw_frag, m->nw_frag, i, r);
	push_8(f->nw_tos, m->nw_tos, i, r);
	push_8(f->nw_ttl, m->nw_ttl, i, r);
	push_8(f->nw_proto, m->nw_proto, i, r);
	assert(i == 166);
	push_128be(f->nd_target, m->nd_target, i, r);
	push_eth_addr(f->arp_sha, m->arp_sha, i, r);
	push_eth_addr(f->arp_tha, m->arp_tha, i, r);
	push_16be(f->tcp_flags, m->tcp_flags, i, r);
	assert(i == 181);
	push_ovs_key_nsh(f->nsh, m->nsh, i, r);
	assert(i == 187);

	push_16be(f->tp_src, m->tp_src, i, r);
	push_16be(f->tp_dst, m->tp_dst, i, r);
	push_16be(f->ct_tp_src, m->ct_tp_src, i, r);
	push_16be(f->ct_tp_dst, m->ct_tp_dst, i, r);
	push_32be(f->igmp_group_ip4, m->igmp_group_ip4, i, r);
	assert(i == DIM_CNT);
}

inline void flow_from_array(const std::array<uint16_t, DIM_CNT> & a,
		struct flow * f) {
	int i = 0;
	const uint16_t * r = &a[0];

	pop_flow_tnl(&f->tunnel, i, r);

	pop_64be(f->metadata, i, r);
	for (size_t i2 = 0; i2 < FLOW_N_REGS; i2++) {
		pop_32(f->regs[i2], i, r);
	}
	pop_32(f->skb_priority, i, r);
	pop_32(f->pkt_mark, i, r);
	pop_32(f->dp_hash, i, r);
	pop_32(f->in_port.odp_port, i, r);
	pop_32(f->recirc_id, i, r);
	pop_8(f->ct_state, i, r);
	pop_8(f->ct_nw_proto, i, r);
	pop_16(f->ct_zone, i, r);
	pop_32(f->ct_mark, i, r);
	pop_32be(f->packet_type, i, r);
	pop_128(f->ct_label, i, r);
	pop_32(f->conj_id, i, r);
	pop_32(f->actset_output, i, r);
	pop_eth_addr(f->dl_dst, i, r);
	pop_eth_addr(f->dl_src, i, r);
	pop_16be(f->dl_type, i, r);
	for (size_t i2 = 0; i2 < FLOW_MAX_VLAN_HEADERS; i2++) {
		pop_32be(f->vlans[i2].qtag, i, r);
	}
	for (size_t i2 = 0; i2 < ROUND_UP(FLOW_MAX_MPLS_LABELS, 2); i2++) {
		pop_32be(f->mpls_lse[i2], i, r);
	}
	pop_32be(f->nw_src, i, r);
	pop_32be(f->nw_dst, i, r);
	pop_32be(f->ct_nw_src, i, r);
	pop_32be(f->ct_nw_dst, i, r);
	pop_128be(f->ipv6_src, i, r);
	pop_128be(f->ipv6_dst, i, r);
	pop_128be(f->ct_ipv6_src, i, r);
	pop_128be(f->ct_ipv6_dst, i, r);
	pop_32be(f->ipv6_label, i, r);
	pop_8(f->nw_frag, i, r);
	pop_8(f->nw_tos, i, r);
	pop_8(f->nw_ttl, i, r);
	pop_8(f->nw_proto, i, r);
	pop_128be(f->nd_target, i, r);
	pop_eth_addr(f->arp_sha, i, r);
	pop_eth_addr(f->arp_tha, i, r);
	pop_16be(f->tcp_flags, i, r);
	pop_ovs_key_nsh(f->nsh, i, r);

	pop_16be(f->tp_src, i, r);
	pop_16be(f->tp_dst, i, r);
	pop_16be(f->ct_tp_src, i, r);
	pop_16be(f->ct_tp_dst, i, r);
	pop_32be(f->igmp_group_ip4, i, r);
	assert(i == DIM_CNT);
}
inline void flow_to_array(const struct flow * f,
		std::array<uint16_t, DIM_CNT> & _r) {
	int i = 0;
	uint16_t * r = &_r[0];

	push_flow_tnl(&f->tunnel, i, r);

	push_64be(f->metadata, i, r);
	for (size_t i2 = 0; i2 < FLOW_N_REGS; i2++) {
		push_32(f->regs[i2], i, r);
	}
	push_32(f->skb_priority, i, r);
	push_32(f->pkt_mark, i, r);
	push_32(f->dp_hash, i, r);
	push_32(f->in_port.odp_port, i, r);
	push_32(f->recirc_id, i, r);
	push_8(f->ct_state, i, r);
	push_8(f->ct_nw_proto, i, r);
	push_16(f->ct_zone, i, r);
	push_32(f->ct_mark, i, r);
	push_32be(f->packet_type, i, r);
	push_128(f->ct_label, i, r);
	push_32(f->conj_id, i, r);
	push_32(f->actset_output, i, r);
	push_eth_addr(f->dl_dst, i, r);
	push_eth_addr(f->dl_src, i, r);
	push_16be(f->dl_type, i, r);
	for (size_t i2 = 0; i2 < FLOW_MAX_VLAN_HEADERS; i2++) {
		push_32be(f->vlans[i2].qtag, i, r);
	}
	for (size_t i2 = 0; i2 < ROUND_UP(FLOW_MAX_MPLS_LABELS, 2); i2++) {
		push_32be(f->mpls_lse[i2], i, r);
	}
	push_32be(f->nw_src, i, r);
	push_32be(f->nw_dst, i, r);
	push_32be(f->ct_nw_src, i, r);
	push_32be(f->ct_nw_dst, i, r);
	push_128be(f->ipv6_src, i, r);
	push_128be(f->ipv6_dst, i, r);
	push_128be(f->ct_ipv6_src, i, r);
	push_128be(f->ct_ipv6_dst, i, r);
	push_32be(f->ipv6_label, i, r);
	push_8(f->nw_frag, i, r);
	push_8(f->nw_tos, i, r);
	push_8(f->nw_ttl, i, r);
	push_8(f->nw_proto, i, r);
	push_128be(f->nd_target, i, r);
	push_eth_addr(f->arp_sha, i, r);
	push_eth_addr(f->arp_tha, i, r);
	push_16be(f->tcp_flags, i, r);
	push_ovs_key_nsh(f->nsh, i, r);

	push_16be(f->tp_src, i, r);
	push_16be(f->tp_dst, i, r);
	push_16be(f->ct_tp_src, i, r);
	push_16be(f->ct_tp_dst, i, r);
	push_32be(f->igmp_group_ip4, i, r);
	assert(i == DIM_CNT);
}

}
