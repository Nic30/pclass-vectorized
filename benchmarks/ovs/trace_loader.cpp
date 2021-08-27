#pragma once
#include "trace_loader.h"

#include <boost/lexical_cast.hpp>

namespace pcv {
namespace ovs {

class uint32_from_hex {   // For use with boost::lexical_cast
	uint32_t value;
public:
	operator uint32_t() const {
		return value;
	}
	friend std::istream& operator>>(std::istream &in,
			uint32_from_hex &outValue) {
		in >> std::hex >> outValue.value;
		return in;
	}
};

OvsTraceLoder::OvsTraceLoder() :
		current("") {
}

std::string OvsTraceLoder::get_id() {
	size_t i = 0;
	for (; i < current.size() && (isalnum(current[i]) | current[i] == '_');
			i++) {
	}
	std::string id = current.substr(0, i);
	current = current.substr(i);
	return id;
}

std::string OvsTraceLoder::get_hex_digit() {
	size_t i = 0;
	for (; i < current.size() && (isxdigit(current[i])); i++) {
	}
	std::string s = current.substr(0, i);
	current = current.substr(i);
	return s;
}

std::string OvsTraceLoder::get_dec_digit() {
	size_t i = 0;
	for (; i < current.size() && (isdigit(current[i])); i++) {
	}
	std::string s = current.substr(0, i);
	current = current.substr(i);
	return s;
}

void OvsTraceLoder::parse_eth_mac(struct eth_addr &res) {
	for (size_t i = 0; i < 6; i++) {
		auto digit = get_hex_digit();
		res.ea[i] = boost::lexical_cast<uint32_from_hex>(digit);
		if (i != 6 - 1) {
			if (current[0] != ':')
				throw std::runtime_error("missing ':' in eth_mac");
			current = current.substr(1);
		}
	}
}

void OvsTraceLoder::parse_in_port(union flow_in_port &res) {
	auto d = get_dec_digit();
	res.ofp_port = boost::lexical_cast<uint32_t>(d);
}

void OvsTraceLoder::parse_ipv4(ovs_be32 &res) {
	auto _res = reinterpret_cast<uint8_t*>(&res);
	for (size_t i = 0; i < 4; i++) {
		auto d = get_dec_digit();
		_res[i] = boost::lexical_cast<uint32_t>(d);
		if (i != 4 - 1) {
			if (current[0] != '.')
				throw std::runtime_error("missing '.' in ipv4");
			current = current.substr(1);
		}
	}
}

void OvsTraceLoder::skip_ws_and_comma() {
	size_t i = 0;
	for (; i < current.size(); i++) {
		auto c = current[i];
		if (c != ' ' && c != ',' && c != '\n')
			break;
	}
	if (i != 0)
		current = current.substr(i);
}

void OvsTraceLoder::parse_ipv6(struct in6_addr &res) {
	auto _res = reinterpret_cast<uint8_t*>(&res);

	for (size_t i = 0; i < 8; i++) {
		auto digit = get_hex_digit();
		_res[i] = boost::lexical_cast<uint32_from_hex>(digit);

		if (i != 8 - 1) {
			if (current[0] != ':')
				throw std::runtime_error("missing ':' in ipv6");
			current = current.substr(1);
		}
	}
}

bool OvsTraceLoder::startswith(const std::string &main_str,
		const std::string &to_match) {
	return main_str.find(to_match) == 0;
}

struct flow OvsTraceLoder::parse_trace_record(const std::string &s) {
	current = s;
	struct flow f;
	memset(&f, 0, sizeof f);
	while (current.size()) {
		std::string id = get_id();
		if (id == "dl_dst") {
			parse_eth_mac(f.dl_dst);
		} else if (id == "dl_src") {
			parse_eth_mac(f.dl_src);
		} else if (id == "in_port") {
			parse_in_port(f.in_port);
		} else if (id == "nw_src") {
			parse_ipv4(f.nw_src);
		} else if (id == "nw_dst") {
			parse_ipv4(f.nw_dst);
		} else if (id == "ipv6_src") {
			parse_ipv6(f.ipv6_src);
		} else if (id == "ipv6_dst") {
			parse_ipv6(f.ipv6_dst);
			//} else if (id == "vlan_tci") {
			//	f.vlans[0].tci
		} else if (startswith(id, "reg")) {
			std::cerr << "reg " << id << " not implemented" << std::endl;
			// protocol shorcuts
		} else if (id == "ip") {
			f.dl_type = 0x0800;
		} else if (id == "ipv6") {
			f.dl_type = 0x86dd;
		} else if (id == "icmp") {
			f.dl_type = 0x0800;
		} else if (id == "icmp6") {
			f.dl_type = 0x86dd;
			f.nw_proto = 58;
		} else if (id == "tcp") {
			f.dl_type = 0x0800;
			f.nw_proto = 6;
		} else if (id == "tcp6") {
			f.dl_type = 0x86dd;
			f.nw_proto = 6;
		} else if (id == "udp") {
			f.dl_type = 0x0800;
			f.nw_proto = 17;
		} else if (id == "udp6") {
			f.dl_type = 0x86dd;
			f.nw_proto = 17;
		} else if (id == "sctp") {
			f.dl_type = 0x0800;
			f.nw_proto = 132;
		} else if (id == "sctp6") {
			f.dl_type = 0x86dd;
			f.nw_proto = 132;
		} else if (id == "arp") {
			f.dl_type = 0x0806;
		} else if (id == "rarp") {
			f.dl_type = 0x8035;
		} else if (id == "mpls") {
			f.dl_type = 0x8847;
		} else if (id == "mplsm") {
			f.dl_type = 0x8848;
		} else {
			throw std::runtime_error("NotImplemented" + id);
		}
		skip_ws_and_comma();
	}
	return f;
}

std::vector<struct flow> OvsTraceLoder::load_from_file(
		const std::string &file_name) {
	std::vector<struct flow> traces;
	std::ifstream in(file_name.c_str());
	if (!in)
		throw std::runtime_error(std::string("Cannot open: ") + file_name);

	std::string str;
	while (std::getline(in, str)) {
		if (str.size() > 0)
			traces.push_back(parse_trace_record(str));
	}
	return traces;
}

}
}
