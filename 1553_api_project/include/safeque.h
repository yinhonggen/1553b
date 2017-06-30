#ifndef __SAFE_QUE_H
#define __SAFE_QUE_H
#include <wtypes.h>
const size_t BUFFER_SIZE_LIMIT = 1024 * 1024 * 1024;
class CSafeQueException {
	// BufSize不能超过BUFFER_SIZE_LIMIT。
};
class BrSafeQue
{

public:
	BrSafeQue();
	~BrSafeQue();

private:
	BYTE* m_pbyBuffer;
	DWORD m_dwBufSize;
	DWORD m_dwWritePos;
	DWORD m_dwReadPos;

	CRITICAL_SECTION m_objSync;

	inline BOOL IsPosLE( DWORD a, DWORD b )
	{
		return (b - a) <= m_dwBufSize * 2;
	}

	inline BOOL IsPosGE( DWORD a, DWORD b )
	{
		return (a - b) <= m_dwBufSize * 2;
	}

	inline void EnterCritical()
	{
		EnterCriticalSection( &m_objSync );
	}

	inline void ExitCritical()
	{
		LeaveCriticalSection( &m_objSync );
	}

	inline void RoundWriteMemory( const BYTE* pbySrc, DWORD dwItemPos, DWORD dwSize )
	{
		if ( dwItemPos + dwSize <= m_dwBufSize )
		{
			memcpy( m_pbyBuffer + dwItemPos, pbySrc, dwSize );
		}
		else
		{
			memcpy( m_pbyBuffer + dwItemPos, pbySrc, m_dwBufSize - dwItemPos );
			memcpy( m_pbyBuffer, pbySrc + m_dwBufSize - dwItemPos,
				dwSize - (m_dwBufSize - dwItemPos) );
		}
	}

	inline void RoundReadMemory( BYTE* pbyDest, DWORD dwItemPos, DWORD dwSize )
	{
		if ( dwItemPos + dwSize <= m_dwBufSize )
		{
			memcpy( pbyDest, m_pbyBuffer + dwItemPos, dwSize );
		}
		else
		{
			memcpy( pbyDest, m_pbyBuffer + dwItemPos, m_dwBufSize - dwItemPos  );
			memcpy( pbyDest + m_dwBufSize - dwItemPos,
				m_pbyBuffer, dwSize - (m_dwBufSize - dwItemPos)  );
		}
	}

public:
	void reset();		///重置缓冲
	BOOL init( DWORD dwbufsize ); ///初始化缓冲，dwbufsize 缓冲大小
	void destroy();  /// 销毁缓冲

	BOOL read_block( BYTE* pbyData, DWORD dwSize ); ///读块数据，如果数据不够dwSize则认为失败。
	BOOL write_block( const BYTE* pbyData, DWORD dwSize );///写块数据，如果数据不够dwSize则认为失败。

	DWORD read( BYTE* pbyData, DWORD dwSize ); ///读数据，返回获取的数据大小
	DWORD write( const BYTE* pbyData, DWORD dwSize ); ///读数据，返回写入的数据大小

	DWORD  get_read_pos();  ///当前读位置
	DWORD  get_write_pos(); ///当前写位置

	DWORD  get_size();  ///返回缓冲大小
	float get_usage();  ///返回缓冲使用率
};

#endif // __SAFE_QUE_H