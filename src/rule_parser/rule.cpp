#include <pcv/rule_parser/rule.h>

namespace pcv {

namespace rule_conv_fn {
template<>
std::array<Range1d<uint16_t>, 7> rule_to_array(const Rule_Ipv4 & r) {
	std::array<Range1d<uint16_t>, 7> _r;
	using R = Range1d<uint16_t>;
	auto m = std::numeric_limits<uint16_t>::max();
	_r[0] = R((r.sip.low >> 16) & m, (r.sip.high >> 16) & m);
	_r[1] = R(r.sip.low & m, r.sip.high & m);
	_r[2] = R((r.dip.low >> 16) & m, (r.dip.high >> 16) & m);
	_r[3] = R(r.dip.low & m, r.dip.high & m);
	_r[4] = r.sport;
	_r[5] = r.dport;
	_r[6] = r.proto;

	return _r;
}
}

}
