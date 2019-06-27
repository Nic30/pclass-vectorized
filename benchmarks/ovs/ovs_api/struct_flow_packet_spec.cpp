#include "classifier-private.h"
#include <pcv/rule_parser/rule.h>

using namespace pcv::rule_vec_format;

using in_packet_position_t = typename Classifier::Search_t::in_packet_position_t;
typename Classifier::packet_spec_t struct_flow_packet_spec = {
		// tunnel.ip_dst
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ip_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ip_dst) + 2, 2,  1),
		// tunnel.ipv6_dst
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_dst) + 14, 2,  1),
		// tunnel.ip_src
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ip_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ip_src) + 2, 2,  1),
		// tunnel.ipv6_src
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ipv6_src) + 14, 2,  1),
		// tunnel.tun_id
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, tun_id) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, tun_id) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, tun_id) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, tun_id) + 6, 2,  1),
		// tunnel.flags
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, flags), 2,  0),
		// tunnel.ip_tos
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ip_tos), 1,  0),
		// tunnel.ip_ttl
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, ip_ttl), 1,  0),
		// tunnel.tp_src
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, tp_src), 2,  1),
		// tunnel.tp_dst
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, tp_dst), 2,  1),
		// tunnel.gbp_id
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, gbp_id), 2,  1),
		// tunnel.gbp_flags
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, gbp_flags), 1,  0),
		// tunnel.erspan_ver
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, erspan_ver), 1,  0),
		// tunnel.erspan_idx
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, erspan_idx) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, erspan_idx) + 0, 2,  0),
		// tunnel.erspan_dir
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, erspan_dir), 1,  0),
		// tunnel.erspan_hwid
		in_packet_position_t(offsetof(flow, tunnel) + offsetof(flow_tnl, erspan_hwid), 1,  0),
		// tunnel.metadata.present
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, present) + 6, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, present) + 4, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, present) + 2, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, present) + 0, 2,  0),
		// tunnel.metadata.opts
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 0, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 2, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 4, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 6, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 8, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 10, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 12, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 14, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 16, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 18, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 20, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 22, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 24, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 26, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 28, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 30, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 32, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 34, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 36, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 38, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 40, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 42, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 44, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 46, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 48, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 50, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 52, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 54, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 56, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 58, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 60, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 62, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 64, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 66, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 68, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 70, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 72, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 74, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 76, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 78, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 80, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 82, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 84, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 86, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 88, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 90, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 92, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 94, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 96, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 98, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 100, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 102, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 104, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 106, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 108, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 110, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 112, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 114, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 116, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 118, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 120, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 122, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 124, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 126, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 128, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 130, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 132, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 134, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 136, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 138, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 140, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 142, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 144, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 146, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 148, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 150, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 152, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 154, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 156, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 158, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 160, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 162, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 164, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 166, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 168, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 170, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 172, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 174, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 176, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 178, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 180, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 182, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 184, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 186, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 188, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 190, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 192, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 194, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 196, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 198, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 200, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 202, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 204, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 206, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 208, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 210, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 212, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 214, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 216, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 218, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 220, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 222, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 224, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 226, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 228, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 230, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 232, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 234, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 236, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 238, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 240, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 242, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 244, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 246, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 248, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 250, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 252, 2,  0),
		in_packet_position_t(offsetof(flow_tnl, metadata) + offsetof(tun_metadata, opts) + 254, 2,  0),
		// metadata
		in_packet_position_t(offsetof(flow, metadata) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, metadata) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, metadata) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, metadata) + 6, 2,  1),
		// regs
		in_packet_position_t(offsetof(flow, regs) + 0 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 0 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 4 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 4 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 8 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 8 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 12 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 12 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 16 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 16 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 20 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 20 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 24 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 24 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 28 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 28 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 32 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 32 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 36 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 36 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 40 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 40 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 44 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 44 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 48 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 48 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 52 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 52 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 56 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 56 + 0, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 60 + 2, 2,  0),
		in_packet_position_t(offsetof(flow, regs) + 60 + 0, 2,  0),
		// skb_priority
		in_packet_position_t(offsetof(flow, skb_priority) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, skb_priority) + 0, 2,  0),
		// pkt_mark
		in_packet_position_t(offsetof(flow, pkt_mark) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, pkt_mark) + 0, 2,  0),
		// dp_hash
		in_packet_position_t(offsetof(flow, dp_hash) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, dp_hash) + 0, 2,  0),
		// in_port
		in_packet_position_t(offsetof(flow, in_port) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, in_port) + 0, 2,  0),
		// recirc_id
		in_packet_position_t(offsetof(flow, recirc_id) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, recirc_id) + 0, 2,  0),
		// ct_state
		in_packet_position_t(offsetof(flow, ct_state), 1,  0),
		// ct_nw_proto
		in_packet_position_t(offsetof(flow, ct_nw_proto), 1,  0),
		// ct_zone
		in_packet_position_t(offsetof(flow, ct_zone), 2,  0),
		// ct_mark
		in_packet_position_t(offsetof(flow, ct_mark) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, ct_mark) + 0, 2,  0),
		// packet_type
		in_packet_position_t(offsetof(flow, packet_type) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, packet_type) + 2, 2,  1),
		// ct_label
		in_packet_position_t(offsetof(flow, ct_label) + 14, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 12, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 10, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 8, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 6, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 4, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, ct_label) + 0, 2,  0),
		// conj_id
		in_packet_position_t(offsetof(flow, conj_id) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, conj_id) + 0, 2,  0),
		// actset_output
		in_packet_position_t(offsetof(flow, actset_output) + 2, 2,  0),
		in_packet_position_t(offsetof(flow, actset_output) + 0, 2,  0),
		// dl_dst
		in_packet_position_t(offsetof(flow, dl_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, dl_dst) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, dl_dst) + 4, 2,  1),
		// dl_src
		in_packet_position_t(offsetof(flow, dl_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, dl_src) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, dl_src) + 4, 2,  1),
		// dl_type
		in_packet_position_t(offsetof(flow, dl_type), 2,  1),
		// vlans
		in_packet_position_t(offsetof(flow, vlans) + 0 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, vlans) + 0 + 2, 2,  1),
		in_packet_position_t(offsetof(flow, vlans) + 4 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, vlans) + 4 + 2, 2,  1),
		// mpls_lse
		in_packet_position_t(offsetof(flow, mpls_lse) + 0 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, mpls_lse) + 0 + 2, 2,  1),
		in_packet_position_t(offsetof(flow, mpls_lse) + 4 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, mpls_lse) + 4 + 2, 2,  1),
		// nw_src
		in_packet_position_t(offsetof(flow, nw_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nw_src) + 2, 2,  1),
		// nw_dst
		in_packet_position_t(offsetof(flow, nw_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nw_dst) + 2, 2,  1),
		// ct_nw_src
		in_packet_position_t(offsetof(flow, ct_nw_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ct_nw_src) + 2, 2,  1),
		// ct_nw_dst
		in_packet_position_t(offsetof(flow, ct_nw_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ct_nw_dst) + 2, 2,  1),
		// ipv6_src
		in_packet_position_t(offsetof(flow, ipv6_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_src) + 14, 2,  1),
		// ipv6_dst
		in_packet_position_t(offsetof(flow, ipv6_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_dst) + 14, 2,  1),
		// ct_ipv6_src
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_src) + 14, 2,  1),
		// ct_ipv6_dst
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, ct_ipv6_dst) + 14, 2,  1),
		// ipv6_label
		in_packet_position_t(offsetof(flow, ipv6_label) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, ipv6_label) + 2, 2,  1),
		// nw_frag
		in_packet_position_t(offsetof(flow, nw_frag), 1,  0),
		// nw_tos
		in_packet_position_t(offsetof(flow, nw_tos), 1,  0),
		// nw_ttl
		in_packet_position_t(offsetof(flow, nw_ttl), 1,  0),
		// nw_proto
		in_packet_position_t(offsetof(flow, nw_proto), 1,  0),
		// nd_target
		in_packet_position_t(offsetof(flow, nd_target) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 4, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 6, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 8, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 10, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 12, 2,  1),
		in_packet_position_t(offsetof(flow, nd_target) + 14, 2,  1),
		// arp_sha
		in_packet_position_t(offsetof(flow, arp_sha) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, arp_sha) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, arp_sha) + 4, 2,  1),
		// arp_tha
		in_packet_position_t(offsetof(flow, arp_tha) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, arp_tha) + 2, 2,  1),
		in_packet_position_t(offsetof(flow, arp_tha) + 4, 2,  1),
		// tcp_flags
		in_packet_position_t(offsetof(flow, tcp_flags), 2,  1),
		// nsh.flags
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, flags), 1,  0),
		// nsh.ttl
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, ttl), 1,  0),
		// nsh.mdtype
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, mdtype), 1,  0),
		// nsh.np
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, np), 1,  0),
		// nsh.path_hdr
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, path_hdr) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, path_hdr) + 2, 2,  1),
		// nsh.context
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 0 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 0 + 2, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 4 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 4 + 2, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 8 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 8 + 2, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 12 + 0, 2,  1),
		in_packet_position_t(offsetof(flow, nsh) + offsetof(ovs_key_nsh, context) + 12 + 2, 2,  1),
		// tp_src
		in_packet_position_t(offsetof(flow, tp_src), 2,  1),
		// tp_dst
		in_packet_position_t(offsetof(flow, tp_dst), 2,  1),
		// ct_tp_src
		in_packet_position_t(offsetof(flow, ct_tp_src), 2,  1),
		// ct_tp_dst
		in_packet_position_t(offsetof(flow, ct_tp_dst), 2,  1),
		// igmp_group_ip4
		in_packet_position_t(offsetof(flow, igmp_group_ip4) + 0, 2,  1),
		in_packet_position_t(offsetof(flow, igmp_group_ip4) + 2, 2,  1),
};

typename Classifier::formaters_t struct_flow_packet_formaters = {
		// tunnel.ip_dst
		rule_vec_format_ipv4_part,
		// tunnel.ipv6_dst
		rule_vec_format_ipv6_part,
		// tunnel.ip_src
		rule_vec_format_ipv4_part,
		// tunnel.ipv6_src
		rule_vec_format_ipv6_part,
		// tunnel.tun_id
		rule_vec_format_default<uint16_t>,
		// tunnel.flags
		rule_vec_format_default<uint16_t>,
		// tunnel.ip_tos
		rule_vec_format_default<uint16_t>,
		// tunnel.ip_ttl
		rule_vec_format_default<uint16_t>,
		// tunnel.tp_src
		rule_vec_format_default<uint16_t>,
		// tunnel.tp_dst
		rule_vec_format_default<uint16_t>,
		// tunnel.gbp_id
		rule_vec_format_default<uint16_t>,
		// tunnel.gbp_flags
		rule_vec_format_default<uint16_t>,
		// tunnel.erspan_ver
		rule_vec_format_default<uint16_t>,
		// tunnel.erspan_idx
		rule_vec_format_default<uint16_t>,
		// tunnel.erspan_dir
		rule_vec_format_default<uint16_t>,
		// tunnel.erspan_hwid
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.present
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[0-1]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[2-3]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[4-5]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[6-7]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[8-9]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[10-11]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[12-13]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[14-15]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[16-17]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[18-19]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[20-21]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[22-23]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[24-25]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[26-27]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[28-29]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[30-31]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[32-33]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[34-35]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[36-37]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[38-39]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[40-41]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[42-43]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[44-45]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[46-47]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[48-49]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[50-51]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[52-53]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[54-55]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[56-57]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[58-59]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[60-61]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[62-63]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[64-65]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[66-67]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[68-69]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[70-71]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[72-73]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[74-75]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[76-77]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[78-79]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[80-81]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[82-83]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[84-85]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[86-87]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[88-89]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[90-91]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[92-93]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[94-95]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[96-97]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[98-99]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[100-101]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[102-103]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[104-105]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[106-107]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[108-109]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[110-111]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[112-113]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[114-115]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[116-117]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[118-119]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[120-121]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[122-123]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[124-125]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[126-127]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[128-129]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[130-131]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[132-133]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[134-135]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[136-137]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[138-139]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[140-141]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[142-143]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[144-145]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[146-147]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[148-149]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[150-151]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[152-153]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[154-155]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[156-157]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[158-159]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[160-161]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[162-163]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[164-165]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[166-167]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[168-169]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[170-171]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[172-173]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[174-175]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[176-177]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[178-179]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[180-181]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[182-183]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[184-185]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[186-187]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[188-189]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[190-191]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[192-193]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[194-195]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[196-197]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[198-199]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[200-201]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[202-203]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[204-205]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[206-207]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[208-209]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[210-211]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[212-213]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[214-215]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[216-217]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[218-219]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[220-221]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[222-223]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[224-225]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[226-227]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[228-229]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[230-231]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[232-233]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[234-235]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[236-237]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[238-239]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[240-241]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[242-243]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[244-245]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[246-247]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[248-249]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[250-251]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[252-253]
		rule_vec_format_default<uint16_t>,
		// tunnel.metadata.opts[254-255]
		rule_vec_format_default<uint16_t>,
		// metadata
		rule_vec_format_default<uint16_t>,
		// regs[0]
		rule_vec_format_default<uint16_t>,
		// regs[1]
		rule_vec_format_default<uint16_t>,
		// regs[2]
		rule_vec_format_default<uint16_t>,
		// regs[3]
		rule_vec_format_default<uint16_t>,
		// regs[4]
		rule_vec_format_default<uint16_t>,
		// regs[5]
		rule_vec_format_default<uint16_t>,
		// regs[6]
		rule_vec_format_default<uint16_t>,
		// regs[7]
		rule_vec_format_default<uint16_t>,
		// regs[8]
		rule_vec_format_default<uint16_t>,
		// regs[9]
		rule_vec_format_default<uint16_t>,
		// regs[10]
		rule_vec_format_default<uint16_t>,
		// regs[11]
		rule_vec_format_default<uint16_t>,
		// regs[12]
		rule_vec_format_default<uint16_t>,
		// regs[13]
		rule_vec_format_default<uint16_t>,
		// regs[14]
		rule_vec_format_default<uint16_t>,
		// regs[15]
		rule_vec_format_default<uint16_t>,
		// skb_priority
		rule_vec_format_default<uint16_t>,
		// pkt_mark
		rule_vec_format_default<uint16_t>,
		// dp_hash
		rule_vec_format_default<uint16_t>,
		// in_port
		rule_vec_format_default<uint16_t>,
		// recirc_id
		rule_vec_format_default<uint16_t>,
		// ct_state
		rule_vec_format_default<uint16_t>,
		// ct_nw_proto
		rule_vec_format_default<uint16_t>,
		// ct_zone
		rule_vec_format_default<uint16_t>,
		// ct_mark
		rule_vec_format_default<uint16_t>,
		// packet_type
		rule_vec_format_default<uint16_t>,
		// ct_label
		rule_vec_format_default<uint16_t>,
		// conj_id
		rule_vec_format_default<uint16_t>,
		// actset_output
		rule_vec_format_default<uint16_t>,
		// dl_dst
		rule_vec_format_default<uint16_t>,
		// dl_src
		rule_vec_format_default<uint16_t>,
		// dl_type
		rule_vec_format_default<uint16_t>,
		// vlans[0]
		rule_vec_format_default<uint16_t>,
		// vlans[1]
		rule_vec_format_default<uint16_t>,
		// mpls_lse[0]
		rule_vec_format_default<uint16_t>,
		// mpls_lse[1]
		rule_vec_format_default<uint16_t>,
		// nw_src
		rule_vec_format_ipv4_part,
		// nw_dst
		rule_vec_format_ipv4_part,
		// ct_nw_src
		rule_vec_format_ipv4_part,
		// ct_nw_dst
		rule_vec_format_ipv4_part,
		// ipv6_src
		rule_vec_format_ipv6_part,
		// ipv6_dst
		rule_vec_format_ipv6_part,
		// ct_ipv6_src
		rule_vec_format_ipv6_part,
		// ct_ipv6_dst
		rule_vec_format_default<uint16_t>,
		// ipv6_label
		rule_vec_format_default<uint16_t>,
		// nw_frag
		rule_vec_format_default<uint16_t>,
		// nw_tos
		rule_vec_format_default<uint16_t>,
		// nw_ttl
		rule_vec_format_default<uint16_t>,
		// nw_proto
		rule_vec_format_default<uint16_t>,
		// nd_target
		rule_vec_format_ipv6_part,
		// arp_sha
		rule_vec_format_default<uint16_t>,
		// arp_tha
		rule_vec_format_default<uint16_t>,
		// tcp_flags
		rule_vec_format_default<uint16_t>,
		// nsh.flags
		rule_vec_format_default<uint16_t>,
		// nsh.ttl
		rule_vec_format_default<uint16_t>,
		// nsh.mdtype
		rule_vec_format_default<uint16_t>,
		// nsh.np
		rule_vec_format_default<uint16_t>,
		// nsh.path_hdr
		rule_vec_format_default<uint16_t>,
		// nsh.context[0]
		rule_vec_format_default<uint16_t>,
		// nsh.context[1]
		rule_vec_format_default<uint16_t>,
		// nsh.context[2]
		rule_vec_format_default<uint16_t>,
		// nsh.context[3]
		rule_vec_format_default<uint16_t>,
		// tp_src
		rule_vec_format_default<uint16_t>,
		// tp_dst
		rule_vec_format_default<uint16_t>,
		// ct_tp_src
		rule_vec_format_default<uint16_t>,
		// ct_tp_dst
		rule_vec_format_default<uint16_t>,
		// igmp_group_ip4
		rule_vec_format_default<uint16_t>,
};
typename Classifier::names_t struct_flow_packet_names = {
		"tunnel.ip_dst-0",                 //
		"tunnel.ip_dst-1",                 //
		"tunnel.ipv6_dst-0",               //
		"tunnel.ipv6_dst-1",               //
		"tunnel.ipv6_dst-2",               //
		"tunnel.ipv6_dst-3",               //
		"tunnel.ipv6_dst-4",               //
		"tunnel.ipv6_dst-5",               //
		"tunnel.ipv6_dst-6",               //
		"tunnel.ipv6_dst-7",               //
		"tunnel.ip_src-0",                 //
		"tunnel.ip_src-1",                 //
		"tunnel.ipv6_src-0",               //
		"tunnel.ipv6_src-1",               //
		"tunnel.ipv6_src-2",               //
		"tunnel.ipv6_src-3",               //
		"tunnel.ipv6_src-4",               //
		"tunnel.ipv6_src-5",               //
		"tunnel.ipv6_src-6",               //
		"tunnel.ipv6_src-7",               //
		"tunnel.tun_id-0",                 //
		"tunnel.tun_id-1",                 //
		"tunnel.tun_id-2",                 //
		"tunnel.tun_id-3",                 //
		"tunnel.flags",                    //
		"tunnel.ip_tos",                   //
		"tunnel.ip_ttl",                   //
		"tunnel.tp_src",                   //
		"tunnel.tp_dst",                   //
		"tunnel.gbp_id",                   //
		"tunnel.gbp_flags",                //
		"tunnel.erspan_ver",               //
		"tunnel.erspan_idx-0",             //
		"tunnel.erspan_idx-1",             //
		"tunnel.erspan_dir",               //
		"tunnel.erspan_hwid",              //
		"tunnel.metadata.present-0",       //
		"tunnel.metadata.present-1",       //
		"tunnel.metadata.present-2",       //
		"tunnel.metadata.present-3",       //
		"tunnel.metadata.opts[0-1]",       //
		"tunnel.metadata.opts[2-3]",       //
		"tunnel.metadata.opts[4-5]",       //
		"tunnel.metadata.opts[6-7]",       //
		"tunnel.metadata.opts[8-9]",       //
		"tunnel.metadata.opts[10-11]",     //
		"tunnel.metadata.opts[12-13]",     //
		"tunnel.metadata.opts[14-15]",     //
		"tunnel.metadata.opts[16-17]",     //
		"tunnel.metadata.opts[18-19]",     //
		"tunnel.metadata.opts[20-21]",     //
		"tunnel.metadata.opts[22-23]",     //
		"tunnel.metadata.opts[24-25]",     //
		"tunnel.metadata.opts[26-27]",     //
		"tunnel.metadata.opts[28-29]",     //
		"tunnel.metadata.opts[30-31]",     //
		"tunnel.metadata.opts[32-33]",     //
		"tunnel.metadata.opts[34-35]",     //
		"tunnel.metadata.opts[36-37]",     //
		"tunnel.metadata.opts[38-39]",     //
		"tunnel.metadata.opts[40-41]",     //
		"tunnel.metadata.opts[42-43]",     //
		"tunnel.metadata.opts[44-45]",     //
		"tunnel.metadata.opts[46-47]",     //
		"tunnel.metadata.opts[48-49]",     //
		"tunnel.metadata.opts[50-51]",     //
		"tunnel.metadata.opts[52-53]",     //
		"tunnel.metadata.opts[54-55]",     //
		"tunnel.metadata.opts[56-57]",     //
		"tunnel.metadata.opts[58-59]",     //
		"tunnel.metadata.opts[60-61]",     //
		"tunnel.metadata.opts[62-63]",     //
		"tunnel.metadata.opts[64-65]",     //
		"tunnel.metadata.opts[66-67]",     //
		"tunnel.metadata.opts[68-69]",     //
		"tunnel.metadata.opts[70-71]",     //
		"tunnel.metadata.opts[72-73]",     //
		"tunnel.metadata.opts[74-75]",     //
		"tunnel.metadata.opts[76-77]",     //
		"tunnel.metadata.opts[78-79]",     //
		"tunnel.metadata.opts[80-81]",     //
		"tunnel.metadata.opts[82-83]",     //
		"tunnel.metadata.opts[84-85]",     //
		"tunnel.metadata.opts[86-87]",     //
		"tunnel.metadata.opts[88-89]",     //
		"tunnel.metadata.opts[90-91]",     //
		"tunnel.metadata.opts[92-93]",     //
		"tunnel.metadata.opts[94-95]",     //
		"tunnel.metadata.opts[96-97]",     //
		"tunnel.metadata.opts[98-99]",     //
		"tunnel.metadata.opts[100-101]",   //
		"tunnel.metadata.opts[102-103]",   //
		"tunnel.metadata.opts[104-105]",   //
		"tunnel.metadata.opts[106-107]",   //
		"tunnel.metadata.opts[108-109]",   //
		"tunnel.metadata.opts[110-111]",   //
		"tunnel.metadata.opts[112-113]",   //
		"tunnel.metadata.opts[114-115]",   //
		"tunnel.metadata.opts[116-117]",   //
		"tunnel.metadata.opts[118-119]",   //
		"tunnel.metadata.opts[120-121]",   //
		"tunnel.metadata.opts[122-123]",   //
		"tunnel.metadata.opts[124-125]",   //
		"tunnel.metadata.opts[126-127]",   //
		"tunnel.metadata.opts[128-129]",   //
		"tunnel.metadata.opts[130-131]",   //
		"tunnel.metadata.opts[132-133]",   //
		"tunnel.metadata.opts[134-135]",   //
		"tunnel.metadata.opts[136-137]",   //
		"tunnel.metadata.opts[138-139]",   //
		"tunnel.metadata.opts[140-141]",   //
		"tunnel.metadata.opts[142-143]",   //
		"tunnel.metadata.opts[144-145]",   //
		"tunnel.metadata.opts[146-147]",   //
		"tunnel.metadata.opts[148-149]",   //
		"tunnel.metadata.opts[150-151]",   //
		"tunnel.metadata.opts[152-153]",   //
		"tunnel.metadata.opts[154-155]",   //
		"tunnel.metadata.opts[156-157]",   //
		"tunnel.metadata.opts[158-159]",   //
		"tunnel.metadata.opts[160-161]",   //
		"tunnel.metadata.opts[162-163]",   //
		"tunnel.metadata.opts[164-165]",   //
		"tunnel.metadata.opts[166-167]",   //
		"tunnel.metadata.opts[168-169]",   //
		"tunnel.metadata.opts[170-171]",   //
		"tunnel.metadata.opts[172-173]",   //
		"tunnel.metadata.opts[174-175]",   //
		"tunnel.metadata.opts[176-177]",   //
		"tunnel.metadata.opts[178-179]",   //
		"tunnel.metadata.opts[180-181]",   //
		"tunnel.metadata.opts[182-183]",   //
		"tunnel.metadata.opts[184-185]",   //
		"tunnel.metadata.opts[186-187]",   //
		"tunnel.metadata.opts[188-189]",   //
		"tunnel.metadata.opts[190-191]",   //
		"tunnel.metadata.opts[192-193]",   //
		"tunnel.metadata.opts[194-195]",   //
		"tunnel.metadata.opts[196-197]",   //
		"tunnel.metadata.opts[198-199]",   //
		"tunnel.metadata.opts[200-201]",   //
		"tunnel.metadata.opts[202-203]",   //
		"tunnel.metadata.opts[204-205]",   //
		"tunnel.metadata.opts[206-207]",   //
		"tunnel.metadata.opts[208-209]",   //
		"tunnel.metadata.opts[210-211]",   //
		"tunnel.metadata.opts[212-213]",   //
		"tunnel.metadata.opts[214-215]",   //
		"tunnel.metadata.opts[216-217]",   //
		"tunnel.metadata.opts[218-219]",   //
		"tunnel.metadata.opts[220-221]",   //
		"tunnel.metadata.opts[222-223]",   //
		"tunnel.metadata.opts[224-225]",   //
		"tunnel.metadata.opts[226-227]",   //
		"tunnel.metadata.opts[228-229]",   //
		"tunnel.metadata.opts[230-231]",   //
		"tunnel.metadata.opts[232-233]",   //
		"tunnel.metadata.opts[234-235]",   //
		"tunnel.metadata.opts[236-237]",   //
		"tunnel.metadata.opts[238-239]",   //
		"tunnel.metadata.opts[240-241]",   //
		"tunnel.metadata.opts[242-243]",   //
		"tunnel.metadata.opts[244-245]",   //
		"tunnel.metadata.opts[246-247]",   //
		"tunnel.metadata.opts[248-249]",   //
		"tunnel.metadata.opts[250-251]",   //
		"tunnel.metadata.opts[252-253]",   //
		"tunnel.metadata.opts[254-255]",   //
		"metadata-0",                      //
		"metadata-1",                      //
		"metadata-2",                      //
		"metadata-3",                      //
		"regs[0]-0",                       //
		"regs[0]-1",                       //
		"regs[1]-0",                       //
		"regs[1]-1",                       //
		"regs[2]-0",                       //
		"regs[2]-1",                       //
		"regs[3]-0",                       //
		"regs[3]-1",                       //
		"regs[4]-0",                       //
		"regs[4]-1",                       //
		"regs[5]-0",                       //
		"regs[5]-1",                       //
		"regs[6]-0",                       //
		"regs[6]-1",                       //
		"regs[7]-0",                       //
		"regs[7]-1",                       //
		"regs[8]-0",                       //
		"regs[8]-1",                       //
		"regs[9]-0",                       //
		"regs[9]-1",                       //
		"regs[10]-0",                      //
		"regs[10]-1",                      //
		"regs[11]-0",                      //
		"regs[11]-1",                      //
		"regs[12]-0",                      //
		"regs[12]-1",                      //
		"regs[13]-0",                      //
		"regs[13]-1",                      //
		"regs[14]-0",                      //
		"regs[14]-1",                      //
		"regs[15]-0",                      //
		"regs[15]-1",                      //
		"skb_priority-0",                  //
		"skb_priority-1",                  //
		"pkt_mark-0",                      //
		"pkt_mark-1",                      //
		"dp_hash-0",                       //
		"dp_hash-1",                       //
		"in_port-0",                       //
		"in_port-1",                       //
		"recirc_id-0",                     //
		"recirc_id-1",                     //
		"ct_state",                        //
		"ct_nw_proto",                     //
		"ct_zone",                         //
		"ct_mark-0",                       //
		"ct_mark-1",                       //
		"packet_type-0",                   //
		"packet_type-1",                   //
		"ct_label-0",                      //
		"ct_label-1",                      //
		"ct_label-2",                      //
		"ct_label-3",                      //
		"ct_label-4",                      //
		"ct_label-5",                      //
		"ct_label-6",                      //
		"ct_label-7",                      //
		"conj_id-0",                       //
		"conj_id-1",                       //
		"actset_output-0",                 //
		"actset_output-1",                 //
		"dl_dst-0",                        //
		"dl_dst-1",                        //
		"dl_dst-2",                        //
		"dl_src-0",                        //
		"dl_src-1",                        //
		"dl_src-2",                        //
		"dl_type",                         //
		"vlans[0]-0",                      //
		"vlans[0]-1",                      //
		"vlans[1]-0",                      //
		"vlans[1]-1",                      //
		"mpls_lse[0]-0",                   //
		"mpls_lse[0]-1",                   //
		"mpls_lse[1]-0",                   //
		"mpls_lse[1]-1",                   //
		"nw_src-0",                        //
		"nw_src-1",                        //
		"nw_dst-0",                        //
		"nw_dst-1",                        //
		"ct_nw_src-0",                     //
		"ct_nw_src-1",                     //
		"ct_nw_dst-0",                     //
		"ct_nw_dst-1",                     //
		"ipv6_src-0",                      //
		"ipv6_src-1",                      //
		"ipv6_src-2",                      //
		"ipv6_src-3",                      //
		"ipv6_src-4",                      //
		"ipv6_src-5",                      //
		"ipv6_src-6",                      //
		"ipv6_src-7",                      //
		"ipv6_dst-0",                      //
		"ipv6_dst-1",                      //
		"ipv6_dst-2",                      //
		"ipv6_dst-3",                      //
		"ipv6_dst-4",                      //
		"ipv6_dst-5",                      //
		"ipv6_dst-6",                      //
		"ipv6_dst-7",                      //
		"ct_ipv6_src-0",                   //
		"ct_ipv6_src-1",                   //
		"ct_ipv6_src-2",                   //
		"ct_ipv6_src-3",                   //
		"ct_ipv6_src-4",                   //
		"ct_ipv6_src-5",                   //
		"ct_ipv6_src-6",                   //
		"ct_ipv6_src-7",                   //
		"ct_ipv6_dst-0",                   //
		"ct_ipv6_dst-1",                   //
		"ct_ipv6_dst-2",                   //
		"ct_ipv6_dst-3",                   //
		"ct_ipv6_dst-4",                   //
		"ct_ipv6_dst-5",                   //
		"ct_ipv6_dst-6",                   //
		"ct_ipv6_dst-7",                   //
		"ipv6_label-0",                    //
		"ipv6_label-1",                    //
		"nw_frag",                         //
		"nw_tos",                          //
		"nw_ttl",                          //
		"nw_proto",                        //
		"nd_target-0",                     //
		"nd_target-1",                     //
		"nd_target-2",                     //
		"nd_target-3",                     //
		"nd_target-4",                     //
		"nd_target-5",                     //
		"nd_target-6",                     //
		"nd_target-7",                     //
		"arp_sha-0",                       //
		"arp_sha-1",                       //
		"arp_sha-2",                       //
		"arp_tha-0",                       //
		"arp_tha-1",                       //
		"arp_tha-2",                       //
		"tcp_flags",                       //
		"nsh.flags",                       //
		"nsh.ttl",                         //
		"nsh.mdtype",                      //
		"nsh.np",                          //
		"nsh.path_hdr-0",                  //
		"nsh.path_hdr-1",                  //
		"nsh.context[0]-0",                //
		"nsh.context[0]-1",                //
		"nsh.context[1]-0",                //
		"nsh.context[1]-1",                //
		"nsh.context[2]-0",                //
		"nsh.context[2]-1",                //
		"nsh.context[3]-0",                //
		"nsh.context[3]-1",                //
		"tp_src",                          //
		"tp_dst",                          //
		"ct_tp_src",                       //
		"ct_tp_dst",                       //
		"igmp_group_ip4-0",                //
		"igmp_group_ip4-1",                //
};

