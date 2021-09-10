#pragma once

#include <array>
#include <assert.h>
#include <stdint.h>
#include <stdexcept>
#include <atomic>
#include <vector>

namespace pcv {

/**
 * The template used override the new/delete operators to use statically callocated memory
 **/
template<typename T, bool THREAD_SAFE>
class DynamicMempool {

	// the T type is not used as we do not want to call constructor for unallocated memory
	// index of the next free node
	struct alignas(alignof(T)) mem_of_obj_t {
		int8_t data[sizeof(T)];
	};

	// Singly Linked List node
	struct Info;
	struct Info {
		Info *next_free;
	};
	// Singly Linked List
	mem_of_obj_t *mempool;
	std::vector<Info> mempool_info;

	// pointer of the first item in the list of the free items (nullptr if no free item)
	Info *m_first_free;

	// spinlock for allocation and deallocation
	__attribute__((aligned(64))) std::atomic_flag lock;
	constexpr void acquire_lock() {
		if (THREAD_SAFE) {
			while (lock.test_and_set(std::memory_order_acquire))
				// acquire lock
				;// spin
		}
	}
	constexpr void release_lock() {
		if (THREAD_SAFE)
			lock.clear(std::memory_order_release);
	}
public:
	// constructor of this mempool which initializes the pointers
	// in the singly linked list
	DynamicMempool(size_t ITEM_CNT) :
			mempool(new mem_of_obj_t[ITEM_CNT]), mempool_info(ITEM_CNT), ITEM_CNT(
					ITEM_CNT) {
		assert(ITEM_CNT > 1);
		//assert(mempool_info.end() != nullptr);
		m_first_free = &mempool_info[0];
		for (auto &item : mempool_info) {
			item.next_free = (&item) + 1;
		}
		mempool_info[mempool_info.size() - 1].next_free = nullptr;
	}

public:
	size_t ITEM_CNT;
	/*
	 * @return number of allocated objects
	 * */
	size_t size() {
		size_t unused = 0;
		//std::cout << "end " << mempool_info.end() << std::endl;
		acquire_lock();
		Info *i = m_first_free;
		if (m_first_free == nullptr) {
			// this mempool was not even initialized yet
			return 0;
		}
		while (i) {
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
	template<typename ... Args>
	T* get(Args ... args) {
		acquire_lock();
		if (m_first_free == nullptr) {
			throw std::bad_alloc(); // out of memory
		}

		size_t indx = m_first_free - &mempool_info[0];
		assert(indx < ITEM_CNT);
		auto *obj = reinterpret_cast<void*>(&mempool[indx]);
		//std::cout << "allocate " << obj << " index" << indx << std::endl;
		auto tmp = m_first_free->next_free;
		assert(tmp != nullptr);
		m_first_free->next_free = nullptr;

		m_first_free = tmp;
		assert(m_first_free->next_free != nullptr);
		assert(m_first_free->next_free != m_first_free);
		assert(m_first_free->next_free->next_free != m_first_free);

		release_lock();

		return new (obj) T(args...);
	}

	/*
	 * Prepend to list of free items
	 * */
	void release(T *addr) {
		acquire_lock();
		static_cast<const T*>(addr)->~T();
		size_t indx = addr - reinterpret_cast<T*>(mempool);
		//std::cout << "release " << addr << " index" << indx << std::endl;
		assert(indx < ITEM_CNT && "Allocation of item which is not in this mempool");

		Info *item = &mempool_info[indx];
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

	void operator=(DynamicMempool const&) = delete;

	constexpr T* getById(size_t id) {
#ifndef NDEBUG
		assert(id < ITEM_CNT);
		if (mempool_info[id].next_free != nullptr) {
			throw std::runtime_error(
					std::string("Item ") + std::to_string(id)
							+ " is deallocated");
		}
#endif
		return reinterpret_cast<T*>(&(mempool[id]));
	}

	constexpr const T* getById(size_t id) const {
		return const_cast<DynamicMempool*>(this)->getById(id);
	}

	constexpr size_t getId(const T *addr) const {
		size_t id = (addr - reinterpret_cast<const T*>(&mempool[0]));
#ifndef NDEBUG
		if (mempool_info[id].next_free != nullptr) {
			throw std::runtime_error(
					std::string("Item ") + std::to_string(id)
							+ " is deallocated");
		}
		assert(id < ITEM_CNT);
#endif
		return id;
	}

	~DynamicMempool() {
		// count free items
		// and check if all items were removed before deleting of this mempool
		if (m_first_free != nullptr) {
			auto *item = m_first_free;
			size_t cnt = 0;
			while (item != nullptr) {
				cnt++;
				item = item->next_free;
			}
			if (cnt == ITEM_CNT)
				return;
		}
		assert(0 && "There are still items in the mempool");
	}
};

}
