/**
 * \file buffer.h
 * 动态缓冲区管理.
 * 最初的想法源自0062项目
 *
 *    \date 2009-8-23
 *  \author anhu.xie
 */

#ifndef net_BUFFER_H_
#define net_BUFFER_H_

#include <queue>
#include "alloc.h"
#include "thread.h"

/**
 * 网络接口使用数据包的缓冲区管理，从网络读写数据使用的数据结构.
 * 主要功能是负责我们分配的内存空间的自动释放：
 * 在此类对象(及其拷贝)的生命周期内，可以随意访问其关联缓冲区内的数据；
 * 在对象生命周期结束时，会自动删除它分配的内存空间。
 */
class AutoBuffer {
	class DataRef;
	friend class DataRef;
	DataRef *rep; // 关联的缓冲区(存放数据)
public:
	/// 对象管理
	AutoBuffer(const AutoBuffer&);
	AutoBuffer& operator=(const AutoBuffer&);
	virtual ~AutoBuffer();
	/// 默认构造函数
	AutoBuffer();
	/// 分配缓冲区
	AutoBuffer(IAllocator &allocator, size_t mem_len);
	// 取得缓冲区地址
	// 如果使用循环缓冲这样的类型，缓冲区地址对使用者没有意义。所以不返回缓冲区地址，必须用RetrieveData()、StoreData()来存取数据。
// 	char *GetBuffer() const;
	/// 获取数据
	void RetrieveData(void *buffer, size_t offset, size_t count) const;
	/// 存放数据
	void StoreData(size_t offset, const void *data, size_t count);
	/// 缓冲区长度
	size_t GetLength() const;
};

/**
 * 实际保存数据的结构.
 * 关键在于内存管理，使用户不必关心内存的问题。
 */
class AutoBuffer::DataRef : MXLock {
	friend class AutoBuffer;
	IAllocator &allocator; // 存储管理器
	char *data_ptr; // 分配的缓冲区指针
	size_t size; // 分配的缓冲区长度
	long ref_count; // 对象引用计数

	DataRef(IAllocator &m, size_t len);
	// 增加引用
	size_t reference();
	// 减少引用，如果已经没有引用了，释放相关内存
	size_t de_reference();
};

/**
 * 使用循环缓冲的存储类
 */
class CircularStore : public CircularAllocator {
	typedef CircularAllocator Parent;
	SyncEvent not_full;
	MXLock alloc_lock;
	MXLock free_lock;
public:
	CircularStore(size_t buflen) : CircularAllocator(buflen) {}
	bool wait_space(unsigned long time_out) {
		return not_full.wait(time_out);
	}
	virtual char *allocate(size_t len) {
		AutoLock al(alloc_lock);
		char *p = Parent::allocate(len);
		return p;
	}
	virtual void deallocate(char *ptr, size_t len) {
		AutoLock al(free_lock);
		Parent::deallocate(ptr, len);
		not_full.signal();
	}
};

/**
 * Agent内部使用的消息队列
 */
template <class M>
class MsgQue : public MXLock {
	std::deque<M> que;
	CondEvent ready;
public:
	MsgQue() : ready(false) {}
	/// 判断队列是否空
	bool empty() const { return que.empty(); }
	/// 清除队列
	void clear() { que.clear(); }
	/// 等待队列中数据
	bool wait(unsigned int u_t_o = 1000000) {
		AutoLock al(*this);
		bool r = que.size() > 0 || ready.wait(this, u_t_o);
		return r;
	}
	/// 数据放入队列
	void push(const M &m) {
		AutoLock al(*this);
		que.push_back(m);
		ready.signal();
	}
	/// 从队列中取走数据
	M pop() {
		AutoLock al(*this);
		M r = que.front();
		que.pop_front();
		return r;
	}
};

// DataRef实现

// 构造函数
inline AutoBuffer::DataRef::DataRef(IAllocator &m, size_t len) : allocator(m), data_ptr(m.allocate(len)), size(len), ref_count(0) {}
// 增加引用
inline size_t AutoBuffer::DataRef::reference() {
	AutoLock al(*this);
	return ref_count++;
}
// 减少引用，如果已经没有引用了，释放相关内存
inline size_t AutoBuffer::DataRef::de_reference() {
	AutoLock al(*this);
	if ( --ref_count == 0 ) {
		// !!!不能使用delete this，因为我们还要访问MXLock子对象！
		allocator.deallocate(data_ptr, size);
		data_ptr = NULL;
		size = 0;
	}
	return ref_count;
}


// AutoBuffer实现

inline AutoBuffer::AutoBuffer() : rep(NULL) {
}

inline AutoBuffer::AutoBuffer(IAllocator &allocator, size_t len) : rep(len > 0 ? new DataRef(allocator, len) : NULL) {
	if ( rep ) {
		if ( rep->data_ptr )
			rep->reference();
		else {
			delete rep;
			rep = NULL;
		}
	}
}

inline AutoBuffer::AutoBuffer(const AutoBuffer &src) : rep(src.rep) {
	if ( rep )
		rep->reference();
}

inline AutoBuffer &AutoBuffer::operator=(const AutoBuffer &src) {
	if ( rep )
		if ( rep->de_reference() == 0 ) {
			delete rep;
		}
	rep = src.rep;
	if ( rep )
		rep->reference();
	return *this;
}

inline AutoBuffer::~AutoBuffer() {
	if ( rep ) {
		if ( rep->de_reference() == 0 ) {
			delete rep;
		}
		rep = 0;
	}
}

// 缓冲区地址
//inline char *AutoBuffer::GetBuffer() const {
//	return rep ? rep->data_ptr : NULL;
//}

// 获取数据
inline void AutoBuffer::RetrieveData(void *buffer, size_t offset, size_t count) const {
	if ( rep )
		rep->allocator.retrieve(rep->data_ptr + offset, buffer, count);
	else
		throw std::runtime_error("no data to retrieve");
}
// 存放数据
inline void AutoBuffer::StoreData(size_t offset, const void *data, size_t count) {
	if ( rep )
		rep->allocator.store(rep->data_ptr + offset, data, count);
	else
		throw std::runtime_error("no memory allocated");
}
// 缓冲区长度
inline size_t AutoBuffer::GetLength() const {
	return rep ? rep->size : 0;
}

#endif /* net_BUFFER_H_ */
