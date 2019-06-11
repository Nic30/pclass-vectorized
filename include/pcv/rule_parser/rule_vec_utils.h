#pragma once
#include <pcv/common/range.h>

namespace pcv {
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
inline void push_16be(const uint16_t & v, const uint16_t & m, int & i, R* dst) {
	dst[i++] = R::from_mask(__swab16p(&v), __swab16p(&m));
}
inline void pop_16be(uint16_t & v, uint16_t & m, int & i, const R* src) {
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
inline void push_16be(const uint16_t & v, int & i, uint16_t* dst) {
	dst[i++] = __swab16p(&v);
}
inline void pop_16be(uint16_t & v, int & i, const uint16_t* src) {
	v = __swab16p(&src[i++]);
}

/***************************************************************************************/
inline void push_32(const uint32_t & v, const uint32_t & m, int & i, R* dst) {
	for (int i2 = 1; i2 >= 0; i2--)
		dst[i++] = R::from_mask(((uint16_t*) &v)[i2], ((uint16_t*) &m)[i2]);
}
inline void pop_32(uint32_t & v, uint32_t & m, int & i, const R* src) {
	uint32_t low = src[i+1].low;
	uint32_t high = src[i+1].high;
	low <<= 16;
	high <<= 16;
	low |= src[i].low;
	high |= src[i].high;

	Range1d<uint32_t> res(low, high);
	v = res.low;
	m = res.get_mask_le();
	i += 2;
}
inline void push_32be(const uint32_t & v, const uint32_t & m, int & i, R* dst) {
	uint32_t _v = __swab32p(&v);
	uint32_t _m = __swab32p(&m);
	for (int i2 = 0; i2 < 2; i2++)
		dst[i++] = R::from_mask(((uint16_t*) &_v)[i2], ((uint16_t*) &_m)[i2]);
}
inline void pop_32be(uint32_t & v, uint32_t & m, int & i, const R* src) {
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
	dst[i++] = v & 0xffff;
	dst[i++] = v >> 16;
}
inline void pop_32(uint32_t & v, int & i, const uint16_t* src) {
	v = src[i++];
	v <<= 16;
	v |= src[i++];
}
inline void pop_32(Range1d<uint32_t> & dst, size_t & i, const R * src) {
	uint32_t l = src[i + 1].low;
	l <<= 16;
	l |= src[i].low;
	uint32_t h = src[i + 1].high;
	h <<= 16;
	h |= src[i].high;
	i += 2;
	dst = {l, h};
}

inline void push_32be(const uint32_t & v, int & i, uint16_t* dst) {
	uint32_t _v = __swab32p(&v);
	for (int i2 = 0; i2 < 2; i2++)
		dst[i++] = ((uint16_t*) &_v)[i2];
}
inline void pop_32be(uint32_t & v, int & i, const uint16_t* src) {
	v = src[i++];
	v <<= 16;
	v |= src[i++];
	__swab32s(&v);
}

/***************************************************************************************/

template<typename eth_addr>
inline void push_eth_addr(const eth_addr & v, const eth_addr & m, int & i,
		R * dst) {
	for (int i2 = 2; i2 >= 0; i2--) {
		dst[i++] = R::from_mask(__swab16p(((uint16_t*) &v) + i2),
				__swab16p(((uint16_t*) &m) + i2));
	}
}

template<typename eth_addr>
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

template<typename eth_addr>
inline void push_eth_addr(const eth_addr & v, int & i, uint16_t * dst) {
	for (int i2 = 2; i2 >= 0; i2--) {
		dst[i++] = __swab16p(((uint16_t*) &v) + i2);
	}
}

template<typename eth_addr>
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
inline void push_64be(const uint64_t & v, const uint64_t & m, int & i, R* dst) {
	uint64_t _v = bswap_64(v);
	uint64_t _m = bswap_64(m);
	for (int i2 = 0; i2 < 4; i2++)
		dst[i++] = R::from_mask(((uint16_t*) &_v)[i2], ((uint16_t*) &_m)[i2]);
}
inline void pop_64be(uint64_t & v, uint64_t & m, int & i, const R* src) {
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

inline void push_64be(const uint64_t & v, int & i, uint16_t* dst) {
	uint64_t _v = bswap_64(v);
	for (int i2 = 0; i2 < 4; i2++)
		dst[i++] = ((uint16_t*) &_v)[i2];
}
inline void pop_64be(uint64_t & v, int & i, const uint16_t* src) {
	uint64_t low = 0;
	for (int i2 = 0; i2 < 4; i2++) {
		low <<= 16;
		low |= src[i++];
	}
	low= bswap_64(low);
	v = low;
}

/***************************************************************************************/
template<typename in6_addr>
inline void push_128be(const in6_addr & v, const in6_addr & m, int & i,
		R* dst) {
	for (int i2 = 0; i2 < 8; i2++)
		dst[i++] = R::from_mask(__swab16p(((uint16_t*) &v) + i2),
				__swab16p(((uint16_t*) &m) + i2));
}

template<typename in6_addr>
inline void pop_128be(in6_addr & v, in6_addr & m, int & i, const R* src) {
	for (int i2 = 0; i2 < 8; i2++) {
		auto r = src[i++];
		r = r.to_be();
		((uint16_t*) &v)[i2] = r.low;
		((uint16_t*) &m)[i2] = r.get_mask_be();
	}
}

template<typename uint128_t>
inline void push_128(const uint128_t & v, const uint128_t & m, int & i, R* dst) {
	for (int i2 = 7; i2 >= 0; i2--)
		dst[i++] = R::from_mask(((uint16_t*) &v)[i2], ((uint16_t*) &m)[i2]);
}

template<typename uint128_t>
inline void pop_128(uint128_t & v, uint128_t & m, int & i, const R* src) {
	for (int i2 = 7; i2 >= 0; i2--) {
		auto r = src[i++];
		((uint16_t*) &v)[i2] = r.low;
		((uint16_t*) &m)[i2] = r.get_mask_le();
	}
}

template<typename in6_addr>
inline void push_128be(const in6_addr & v, int & i, uint16_t* dst) {
	for (int i2 = 0; i2 < 8; i2++)
		dst[i++] = __swab16p(((uint16_t*) &v) + i2);
}

template<typename in6_addr>
inline void pop_128be(in6_addr & v, int & i, const uint16_t* src) {
	for (int i2 = 0; i2 < 8; i2++) {
		((uint16_t*) &v)[i2] = __swab16p(&src[i++]);
	}
}

template<typename uint128_t>
inline void push_128(const uint128_t & v, int & i, uint16_t* dst) {
	for (int i2 = 7; i2 >= 0; i2--)
		dst[i++] = ((uint16_t*) &v)[i2];
}

template<typename uint128_t>
inline void pop_128(uint128_t & v, int & i, const uint16_t* src) {
	for (int i2 = 7; i2 >= 0; i2--) {
		((uint16_t*) &v)[i2] = src[i++];
	}
}

}
}
