/**
 * \file alloc.h
 * 动态缓冲区管理.
 * 从原buffer.h中分离出来
 * 现在增加一种机制，可以使用固定的(循环)缓冲区而不要总是不停的分配和释放内存。
 *
 *    \date 2009-12-30
 *  \author anhu.xie
 */

#ifndef net_ALLOCATOR_H_
#define net_ALLOCATOR_H_
#include <map>

/**
 * 缓冲区管理器/存储分配器的抽象接口.
 * 这是面向网络收发的。
 * 在网络收发过程中，对缓冲区的需求是这样的：
 * 首先分配存储(allocate)，然后往分配的空间里存储数据(store)，接着数据会被使用者取走(retrieve)，最后，缓冲区会被释放(deallocate)。
 * 所以，我们定义了如下接口，并且假设各个调用会按照上述顺序发生。当然，存取时，可能每次只存取一部分数据，要多次才能完成存取。
 * 另外，也可能存储的数据会被多人反复使用。但是，需要保证的是，allocate发生在store之前，而且只有store完成之后，才能retrieve，
 * 而deallocate之后，则不能进行任何操作。
 */
class IAllocator {
public:
	/**
	 * 分配缓冲区
	 * \param len 要求的缓冲区长度
	 * \return 获得的缓冲区的地址。NULL表示不能分配指定大小的缓冲区
	 */
	virtual char *allocate(size_t len) = 0;
	/**
	 * 释放缓冲区
	 * \param ptr 要释放的缓冲区，必须是以前allocate()返回的值，否则结果不可预料
	 * \param len 缓冲区程度，也必须是调用allocate()时的长度
	 */
	virtual void deallocate(char *ptr, size_t len) = 0;
	/**
	 * 从缓冲区中取数据
	 * \param position 数据在缓冲区的中的(起始)位置
	 * \param data 存放数据的指针
	 * \param count 要获取的数据的数目，字节数
	 */
	virtual void retrieve(const char *position, void *data, size_t count) = 0;
	/**
	 * 向缓冲区中存放数据
	 * \param position 数据在缓冲区的中的(起始)位置
	 * \param data 要存放的数据的指针
	 * \param count 要数据的长度，字节数
	 */
	virtual void store(char *position, const void *data, size_t count) = 0;
	/// 析构函数，释放资源
	virtual ~IAllocator() {
	}
};

/**
 * 动态分配和释放空间的缓冲区管理器
 */
class DynaAllocator : public IAllocator {
	virtual char *allocate(size_t len) {
		return new char[len];
	}
	virtual void deallocate(char *ptr, size_t len) {
		delete []ptr;
	}
	virtual void retrieve(const char *position, void *data, size_t count) {
		memcpy(data, position, count);
	}
	virtual void store(char *position, const void *data, size_t count) {
		memcpy(position, data, count);
	}
};

/**
 * 循环缓冲区
 */
class CircularAllocator : public IAllocator {
private:
	char* m_Buffer;
	size_t m_BufSize;
	size_t m_AllocPos;
	size_t m_FreePos;
	std::map<char *, size_t> m_PendFree;
public:
	CircularAllocator(size_t sz) : m_Buffer(NULL), m_BufSize(sz), m_AllocPos(0), m_FreePos(0) {}
	/// 析构函数，释放资源
	~CircularAllocator() {
		Destroy();
	}
	/**
	 * 重新设置缓冲区.
	 * 收回所有已分配的缓冲区，需要的话，收缩或者扩展缓冲区空间到指定大小
	 * \param new_size 新的缓冲区大小。0表示大小不变，只回收已分配的空间。
	 */
	void Reset(size_t new_size = 0) {
		if ( new_size && new_size != m_BufSize ) {
			Destroy();
			m_Buffer = new char[new_size];
			m_BufSize = new_size;
		}
		else {
			m_AllocPos = 0;
			m_FreePos = 0;
		}
	}
	/// 清理
	void Destroy() {
		delete []m_Buffer;
		m_Buffer = NULL;
		m_AllocPos = 0;
		m_FreePos = 0;
	}
	/// 计算已用缓冲区长度
	size_t UsedCount() const {
		int count = m_AllocPos - m_FreePos;
		if ( count < 0 )
			count += m_BufSize;
		return count;
	}

public: // IAllocator接口
	virtual char *allocate(size_t len) {
		if ( m_Buffer == NULL && m_BufSize > 0 ) {
			m_Buffer = new char[m_BufSize]; // 延迟分配空间
		}
		if ( len >= m_BufSize )
			throw std::bad_alloc();
		if ( m_BufSize - UsedCount() <= len ) // 可用空间。注意一定是<=，因为m_WritePos==m_ReadPos表示空，缓冲区不能满！
			return NULL;
		size_t curPos = m_AllocPos;
		size_t nextPos = m_AllocPos + len;
		if ( nextPos >= m_BufSize )
			nextPos -= m_BufSize;
		m_AllocPos = nextPos;
		return m_Buffer + curPos;
	}
	virtual void deallocate(char *ptr, size_t len) {
		if ( len >= m_BufSize )
			throw std::bad_alloc();
		if ( UsedCount() < len || ptr < m_Buffer || ptr >= m_Buffer + m_BufSize ) // 比我们全部分配的空间大，或者不是我们分配的
			throw std::bad_alloc();
		if ( ptr != m_Buffer + m_FreePos ) { // 不是第一个分配的
			if ( m_PendFree.empty() )
				m_PendFree[ptr] = len;
			else { // 是否可以与前面的待释放块合并
				std::map<char *, size_t>::iterator next = m_PendFree.begin();
				while ( next != m_PendFree.end() && ptr > next->first ) { // 找一个比我们大的
					++next;
				}
				// next == m_PendFree.end() || ptr <= next->first
				char *merged_next = NULL;
				if ( next != m_PendFree.end() ) {// ptr <= next->first!
					if ( ptr + len == next->first ) {
						merged_next = next->first;
						len += next->second;
					}
				}
				if ( next != m_PendFree.begin() && (--next)->first + next->second == ptr ) // 合并前面那个！
					next->second += len;
				else
					m_PendFree[ptr] = len; // 插入！
				if ( merged_next )
					m_PendFree.erase(merged_next);
			}
			return;
		}
		if ( !m_PendFree.empty() ) { // 合并先释放的相邻块
			char *next_block = ptr + len;
			if ( next_block >= m_Buffer + m_BufSize )
				next_block -= m_BufSize;
			std::map<char *, size_t>::iterator next = m_PendFree.find(next_block);
			while ( next != m_PendFree.end() ) {
				len += next->second;
				m_PendFree.erase(next);
				next_block = ptr + len;
				if ( next_block >= m_Buffer + m_BufSize )
					next_block -= m_BufSize;
				next = m_PendFree.find(next_block);
			}
		}
		size_t dwEndPos = (m_FreePos + len);
		if ( dwEndPos >= m_BufSize ) // wrap
			dwEndPos -= m_BufSize;
		m_FreePos = dwEndPos;
	}
	virtual void retrieve(const char *position, void *data, size_t count) {
		if ( m_FreePos == 0 && m_AllocPos == 0 )
			return; // no data(after reset)!
		if ( position > m_Buffer + m_BufSize )
			position -= m_BufSize; // 循环回来
		if ( position < m_Buffer || position > m_Buffer + m_BufSize )
			throw std::bad_alloc(); // 不在我们的缓冲区范围内
		size_t nowrap = m_Buffer + m_BufSize - position; // 到缓冲区尾部的长度
		if ( nowrap >= count ) // no wrap
			memcpy(data, position, count);
		else { // wrapped!
			memcpy(data, position, nowrap);
			memcpy(reinterpret_cast<char*>(data) + nowrap, m_Buffer, count - nowrap);
		}
	}
	virtual void store(char *position, const void *data, size_t count) {
		if ( m_FreePos == 0 && m_AllocPos == 0 )
			return; // must re-allocate after reset!
		if ( position > m_Buffer + m_BufSize )
			position -= m_BufSize; // 循环回来了！
		if ( position < m_Buffer || position > m_Buffer + m_BufSize )
			throw std::bad_alloc(); // 不在我们的缓冲区范围内
		size_t nowrap = m_Buffer + m_BufSize - position;
		if ( nowrap >= count ) // no wrap
			memcpy(position, data, count);
		else { // wrapped!
			memcpy(position, data, nowrap);
			memcpy(m_Buffer, reinterpret_cast<const char*>(data) + nowrap, count - nowrap);
		}
	}
};


#endif /* net_ALLOCATOR_H_ */
