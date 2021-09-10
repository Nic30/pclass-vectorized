#pragma once

#include <array>
#include <assert.h>
#include <stdint.h>
#include <stdexcept>
#include <atomic>
#include <map>
#include <stdlib.h>

namespace pcv {

/**
 * The template used override the new/delete operators to use statically allocated memory
 **/
template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
class StaticMempool final {

	typedef std::array<uint8_t, sizeof(T)> mem_of_obj_t __attribute__((aligned(64)));
	//using mem_of_obj_t = std::array<uint8_t, sizeof(T)> ;

	static std::map<T*, size_t> addr_to_id;
	static std::array<T*, ITEM_CNT> id_to_addr;

	// sentinel for the allocation

	// spinlock for allocation and deallocation
	static __attribute__((aligned(64)))          std::atomic_flag lock;

	// staic constructor of this mempool which initializes the pointers
	// in the singly linked list
	static void init() {
		for (size_t i = 0; i < id_to_addr.size(); i++) {
			id_to_addr[i] = nullptr;
		}
	}

	static constexpr void acquire_lock() {
		if (THREAD_SAFE) {
			while (lock.test_and_set(std::memory_order_acquire))
				// acquire lock
				;// spin
		}
	}
	static constexpr void release_lock() {
		if (THREAD_SAFE)
			lock.clear(std::memory_order_release);
	}
public:

	static void* get() {
		for (size_t i = 0; i < id_to_addr.size(); i++) {
			if (id_to_addr[i] == nullptr) {
				auto o = reinterpret_cast<T*>(aligned_alloc(64,
						sizeof(mem_of_obj_t)));
				if (o == nullptr)
					throw std::bad_alloc();
				id_to_addr[i] = o;
				addr_to_id[o] = i;
				return o;
			}
		}
		throw std::bad_alloc();
	}

	static void release(T* addr) {
		auto i = addr_to_id.find(addr);
		free(id_to_addr[i->second]);
		id_to_addr[i->second] = nullptr;
		addr_to_id.erase(i);
	}

	void operator=(StaticMempool const&) = delete;

	~StaticMempool() {
		for (auto i : id_to_addr) {
			assert(i == 0 && "Memory leak");
		}
	}

//protected:
	static constexpr T * getById(size_t id) {
		auto obj = id_to_addr[id];
		assert(obj);
		return obj;
	}

	static constexpr size_t getId(const T * addr) {
		return addr_to_id.find(const_cast<T*>(addr))->second;
	}

};

template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
std::array<T*, ITEM_CNT> StaticMempool<T, ITEM_CNT, THREAD_SAFE>::id_to_addr;
template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
std::map<T*, size_t> StaticMempool<T, ITEM_CNT, THREAD_SAFE>::addr_to_id;

template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
std::atomic_flag StaticMempool<T, ITEM_CNT, THREAD_SAFE>::lock =
ATOMIC_FLAG_INIT;

template<typename T, std::size_t object_cnt, bool THREAD_SAFE = false>
class ObjectWithStaticMempool {
public:
	using _Mempool_t = StaticMempool<T, object_cnt, THREAD_SAFE>;

	static void* operator new(__attribute__((unused))                std::size_t sz) {
		return StaticMempool<T, object_cnt, THREAD_SAFE>::get();
	}

	static void operator delete(void* ptr) {
		StaticMempool<T, object_cnt, THREAD_SAFE>::release(
				reinterpret_cast<T*>(ptr));
	}
	static void* operator new[](std::size_t count) = delete;
};

}
