#pragma once

#include <array>
#include <assert.h>
#include <stdint.h>
#include <stdexcept>
#include <atomic>

namespace pcv {

/**
 * The template used override the new/delete operators to use statically callocated memory
 **/
template<typename T, std::size_t _ITEM_CNT, bool THREAD_SAFE>
class alignas(64) StaticMempool final {

	// the T type is not used as we do not want to call constructor for unallocated memory
	// index of the next free node
	using mem_of_obj_t = std::array<uint8_t, sizeof(T)>;

	// Singly Linked List node
	struct Info;
	struct Info {
		Info * next_free;
	};
	// Singly Linked List
	static std::array<mem_of_obj_t, _ITEM_CNT> mempool __attribute__((aligned(64)));
	static std::array<Info, _ITEM_CNT> mempool_info;

	// pointer of the first item in the list of the free items
	static Info * m_first_free;

	// spinlock for allocation and deallocation
	static __attribute__((aligned(64))) std::atomic_flag lock;

	// staic constructor of this mempool which initializes the pointers
	// in the singly linked list
	static void init() {
		assert(mempool_info.end() != nullptr);
		m_first_free = &mempool_info[0];
		for (auto &item : mempool_info) {
			item.next_free = (&item) + 1;
		}
		mempool_info[mempool_info.size() - 1].next_free = mempool_info.end();
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
	static constexpr size_t ITEM_CNT = _ITEM_CNT;
	/*
	 * @return number of allocated objects
	 * */
	static size_t size() {
		size_t unused = 0;
		//std::cout << "end " << mempool_info.end() << std::endl;
		acquire_lock();
		Info * i = m_first_free;
		if (m_first_free == nullptr) {
			// this mempool was not even initialized yet
			return 0;
		}
		while (i != mempool_info.end()) {
			unused++;
			i = i->next_free;
			assert(unused <= ITEM_CNT);
		}
		release_lock();
		return ITEM_CNT - unused;
	}

	/*
	 * Allocate raw memory for specified type
	 * */
	static void* get() {
		acquire_lock();
		if (m_first_free == mempool_info.end()) {
			throw std::bad_alloc(); // out of memory
		} else if (m_first_free == nullptr) {
			init();
		}

		size_t indx = m_first_free - &mempool_info[0];
		assert(indx < ITEM_CNT);
		auto* obj = reinterpret_cast<void*>(&mempool[indx][0]);
		//std::cout << "allocate " << obj << " index" << indx << std::endl;
		auto tmp = m_first_free->next_free;
		assert(tmp != nullptr);
		m_first_free->next_free = nullptr;

		m_first_free = tmp;
		assert(m_first_free->next_free != nullptr);
		assert(m_first_free->next_free != m_first_free);
		assert(m_first_free->next_free->next_free != m_first_free);

		release_lock();
		return obj;
	}

	/*
	 * Prepend to list of free items
	 * */
	static void release(T* addr) {
		acquire_lock();
		size_t indx = addr - reinterpret_cast<T*>(&mempool[0][0]);
		//std::cout << "release " << addr << " index" << indx << std::endl;
		assert(indx < ITEM_CNT);

		Info * item = &mempool_info[indx];
		if (item->next_free != nullptr)
			throw std::runtime_error(
					std::string("Double free ") + std::to_string(indx));

		assert(m_first_free != nullptr);
		item->next_free = m_first_free;
		m_first_free = item;

		assert(m_first_free->next_free != m_first_free);
		assert(m_first_free->next_free->next_free != m_first_free);

		release_lock();
	}

	void operator=(StaticMempool const&) = delete;

	~StaticMempool() {
		// count free items
		// and check if all items were removed before deleting of this mempool
		if (m_first_free != mempool_info.end()) {
			auto * item = m_first_free;
			size_t cnt = 0;
			while (item != mempool_info.end()) {
				cnt++;
			}
			if (cnt == ITEM_CNT)
				return;
		}
		assert(0 && "There are still items in the mempool");
	}

	static constexpr T * getById(size_t id) {
		assert(id < ITEM_CNT);
		if (mempool_info[id].next_free != nullptr) {
			throw std::runtime_error(
					std::string("Item ") + std::to_string(id)
							+ " is deallocated");
		}
		return reinterpret_cast<T*>(&(mempool[id][0]));
	}

	static constexpr size_t getId(const T * addr) {
		size_t id = (addr - reinterpret_cast<const T*>(&mempool[0][0]));
		if (mempool_info[id].next_free != nullptr) {
			throw std::runtime_error(
					std::string("Item ") + std::to_string(id)
							+ " is deallocated");
		}
		assert(id < ITEM_CNT);
		return id;
	}
};

// static declarations for StaticMempool
template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
std::array<std::array<uint8_t, sizeof(T)>, ITEM_CNT> StaticMempool<T, ITEM_CNT,
		THREAD_SAFE>::mempool;

template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
std::array<typename StaticMempool<T, ITEM_CNT, THREAD_SAFE>::Info, ITEM_CNT> StaticMempool<
		T, ITEM_CNT, THREAD_SAFE>::mempool_info;

template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
typename StaticMempool<T, ITEM_CNT, THREAD_SAFE>::Info * StaticMempool<T,
		ITEM_CNT, THREAD_SAFE>::m_first_free(nullptr);

template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
std::atomic_flag StaticMempool<T, ITEM_CNT, THREAD_SAFE>::lock =
ATOMIC_FLAG_INIT;

template<typename T, std::size_t ITEM_CNT, bool THREAD_SAFE>
const size_t StaticMempool<T, ITEM_CNT, THREAD_SAFE>::ITEM_CNT;

template<typename T, std::size_t object_cnt, bool THREAD_SAFE = false>
class ObjectWithStaticMempool {
public:
	using _Mempool_t = StaticMempool<T, object_cnt, THREAD_SAFE>;

	static void* operator new(__attribute__((unused))   std::size_t sz) {
		return StaticMempool<T, object_cnt, THREAD_SAFE>::get();
	}

	static void operator delete(void* ptr) {
		StaticMempool<T, object_cnt, THREAD_SAFE>::release(
				reinterpret_cast<T*>(ptr));
	}
	static void* operator new[](std::size_t count) = delete;
};


}
