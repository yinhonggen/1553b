#ifndef __COMMUNICATION_H_
#define __COMMUNICATION_H_

/*
 *  Comm.h
 *
 * 快视项目
 * 通信接口设计
 *
 *  Created by anhu.xie on 2009-01-24.
 * Copyright (C) 2004 - 2011 北京国科环宇空间技术有限公司 保留所有权利
 *
 */
#include <vector>
#include <set>
#include <utility>
#include "thread.h"
//#include "pub/datagram.h"
//#include "platform/types.h"
//#include "gist/header.h"
// 兼容性设置
#ifdef WIN32

#ifdef _COMM_IMPL_
#define _COMM_API_ __declspec(dllexport)
#else
#define _COMM_API_ __declspec(dllimport)
#endif

#include <WinSock2.h>
#undef errno
#define errno WSAGetLastError()

typedef int socklen_t;
#if _MSC_VER >= 1700
#undef EWOULDBLOCK
#undef ECONNREFUSED
#undef ENETUNREACH
#undef EHOSTUNREACH
#undef ETIMEDOUT
#endif
#undef EINTR
#undef ECONNABORTED
#define SHUT_RDWR SD_BOTH
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ECONNREFUSED WSAECONNREFUSED
#define ENETUNREACH WSAENETUNREACH
#define	EHOSTUNREACH WSAEHOSTUNREACH
#define ETIMEDOUT WSAETIMEDOUT
#define EINTR WSAEINTR
#define ECONNABORTED WSAECONNABORTED

#else /* _MSC_VER */

#include <sys/socket.h>
#include <arpa/inet.h>
#define _COMM_API_
typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
inline void Sleep(unsigned int ms) { usleep(ms * 1000); }
inline int closesocket(SOCKET s) { return ::close(s); }

#endif /* _MSC_VER */

// 内部实现用：管理数据缓冲区！
class data_rep;
// 通信(信道)管理内部实现的数据结构！
class comm_impl;

enum NET_MESSAGE {
	ON_CONNECT = 0x1, // 新连接
	ON_CLOSE = 0x2 ,   // 断开
};

/**
 * 网络接口使用数据包的缓冲区管理，从网络读写数据使用的数据结构。
 * 主要功能是负责我们分配的内存空间的自动释放：
 * 在此类对象(及其拷贝)的生命周期内，可以随意访问其关联缓冲区内的数据；
 * 在对象生命周期结束时，会自动删除它分配的内存空间。
 */
class _COMM_API_ BrBufferMan {
	friend class comm_impl;
	data_rep *rep; /// 关联的缓冲区(存放数据)
public:
	// 对象管理
	BrBufferMan(const BrBufferMan&);
	BrBufferMan& operator=(const BrBufferMan&);
	~BrBufferMan();
	
	explicit BrBufferMan(size_t mem_len);/// 分配缓冲区
	
	char *get_buffer() const; /// 取得缓冲区地址
	
	size_t get_length() const;/// 缓冲区长度
};

/*
 * 实际使用的数据包结构！
 *
 * 注意以下用到的包头类型(HEADER_TYPE)是一个使用者定义的类型，必须是GenericHeader的子类！
 */
template<class HEADER_TYPE>
class BrPackage : public BrBufferMan {
	HEADER_TYPE *header; ///包头
	char *data;          /// 包数据区
public:
	/** 从已定义具体类型的包头和已有数据构造数据包。用于处理已有的数据。
	 默认情况下，数据会被拷贝到新分配的内存区域，
	 所以用户给出的包头和数据区没有必要是连在一起的，可以在不同的地方。
	 这可以方便处理多层次包头的情况。
	 此对象会自己释放自己分配的内存。
	 */
	BrPackage(HEADER_TYPE *hdr, char*dat, bool copy = true) : BrBufferMan(copy ? hdr->pack_len : 0) {
		char *buffer = copy ? get_buffer() : 0;
		if ( buffer ) {
			header = (HEADER_TYPE*)buffer;
			data = buffer + sizeof(HEADER_TYPE);
			memcpy(header, hdr, sizeof(HEADER_TYPE));
			memcpy(data, dat, hdr->pack_len - sizeof(HEADER_TYPE));
		}
		else {
			header = hdr;
			data = dat;
		}
	}

	/** 从数据流读取数据包时使用。此时会分配内存并负责最终的释放。
	 用于获取网络上读取的数据。*/
	BrPackage(BrBufferMan databuf = BrBufferMan(0)) : BrBufferMan(databuf) {
		char *buffer = get_buffer();
		if ( buffer ) { // 有时我们返回空数据包！
			header = (HEADER_TYPE*)buffer;
			data = buffer + sizeof(HEADER_TYPE);
		}
		else {
			header = 0;
			data = 0;
		}
	}

	
	// 获取包头，完整的数据包长度等信息可以在这里得到
	HEADER_TYPE *get_header() const { return header; }
	// 获取数据本身，数据的长度可以根据包头计算出来
	char *get_data() const { return data; }
	/// 缓冲区长度
	size_t get_length() const {
		size_t buf_len = BrBufferMan::get_length();
		if ( buf_len || !header )
			return buf_len;
		return header->pack_len;
	}
};

// TCP/IP网络地址
class _COMM_API_ BrAddress {
	sockaddr_in inet_a;
public:
	BrAddress(unsigned short port, const char *ip = 0); /// 对于Listen来说，IP不是必须的！
	BrAddress(sockaddr_in a) : inet_a(a) {}
	sockaddr *operator &() const { return (sockaddr*)&inet_a; }
	sockaddr_in addr() const { return inet_a; }  /// 返回SOCKET网络地址
	size_t name_len() const { return sizeof inet_a; } ///长度
};

// 统计信息
struct BrIOStat;
// 通信过程可能发生的异常
struct  BrSockError : public os_error{ // 底层winsock错误
	BrSockError(int err) : os_error(err) {}
};
// 通信层(winsock之上)错误
struct  BrCommError : BrSockError {
	BrCommError(int e) : BrSockError(e) {
		switch ( reason() )
		{
		case CE_INVALID_ARG:
			msg = "参数错误。比如，指定的Channel不是我们返回的值。";
			break;
		case CE_INVALID_CHN:
			msg = "信道已经关闭，或者不是我们返回的值";
		case CE_INCOMP_SYNC_FLAG:
			msg = "同步码不一致：一个“通信”已经使用过一个跟当前指定同步码不同的同步码";
			break;
		case CE_ASYNC_ERROR:
			msg = "后台读写操作出错";
			break;
		case CE_INVALID_STAT:
			msg = "程序状态错误，比如尚未初始化(connect/listen/accept)就开始读写数据";
			break;
		case CE_CONNECT_CLOSE:
			msg = "连接已关闭";
		default:
			break;
		}
	}
	enum ErrorCode { // 和SockError不一样的错误代码
		CE_INVALID_ARG = 0xE0000000, // 参数错误。比如，指定的Channel不是我们返回的值。
		CE_INVALID_CHN, // 信道已经关闭，或者不是我们返回的值
		CE_INCOMP_SYNC_FLAG, // 同步码不一致：一个“通信”已经使用过一个跟当前指定同步码不同的同步码
		CE_CONNECT_CLOSE,	// 连接已经关闭
		CE_ASYNC_ERROR, // 后台读写操作出错
		CE_INVALID_STAT // 程序状态错误，比如尚未初始化(connect/listen/accept)就开始读写数据
	};
};
/**
 *“通信”
 * 通信的基本单位是“包”，而不是字节流
 */
class _COMM_API_ BrComm {
protected:
	comm_impl *comm_desc; // 使用指针仅仅是为了从接口中隐藏comm_impl的定义！

	BrComm(size_t gen_buf_size, size_t chn_buf_size, comm_impl * = 0); // 也许派生类使用的“通信实现”类会有不同
protected:
	void OnConnection(const BrAddress &peer) {} // 完成连接时的回调!
public:
	typedef SOCKET Channel; // 信道标识
	typedef void (* BrNetCallback)(NET_MESSAGE message,Channel ch,void * pobj);
	bool RegisterCallback(BrNetCallback pfun);
	enum {
		ANY_CHANNEL = INVALID_SOCKET,
		ALL_CHANNEL = -10,
	};
	~BrComm();
	/**
	* 发送数据，发送数据是异步的，如果没有指定信道就不知道数据最后从哪里发出
	* @param header 要发送的数据包的包头，包含同步标志和包长信息
	* @param data 要发送数据包的数据指针
	* @param ch 选定的信道，如果需要在数据包在下一个空闲的信道发送，使用ANY_CHANNEL
	* @return 当前发送队列长度
	* @exception SockError 底层(winsock)通信异常
	*/
	template<class HT> size_t write(HT *header, char*data, Channel channel = ANY_CHANNEL)
	{
		return write(reinterpret_cast<char*>(header), sizeof(HT), data, header->pack_len - sizeof (HT), channel);
	}
	/**
	 * 接收数据
	 * 特别约定：每个“通信”，如果有多个“信道”的话，各通道能接收的所有包的同步字段的值都是相同的！
	 * 并且一个“通信”上后续的“读”，所使用的同步字段必须和前面使用的相同。
	 * @param head 要发送的数据包的包头结构(及其包头同步字段的值)
	 * 读数据包现在实际有两种情况，一种包长是固定的，因此数据内并没有包含长度字段；另外一种，数据内包含了长度字段。
	 * 一个“通信”只能同时被用于一种情况，而且对于包长是固定的情况，所读取的所有数据包都必须是同一个包长。
	 * 注意：不论哪种情况，返回的数据包都是包含包长字段的！
	 * 如果Read时参数head.pack_len被赋予一个大于零的值，“通信”过程就认为是要读一个固定长度的数据包，
	 * 此时输入的head.pack_len即为原始包长，包含同步字段(不包含长度字段)。
	 * @param ch 选定的信道，如果不关心数据来自于哪个信道，使用Comm::ANY_CHANNEL以表明任何一个已有数据的信道数据包都可以
	 * @return 接收到的完整数据包，如果Pacakge::GetLength() == 0，则说明对方已关闭发送通道，(此信道)不会再有数据包了
	 * @exception SockError 底层(winsock)通信异常
	 */
	template<class HT> BrPackage<HT> read(const HT &head, Channel ch) {
		BrBufferMan packdata = read(&head, ch, true);
		BrPackage<HT> pack(packdata);
		return pack;
	}
	/**
	 * 读数据包的另外一种形式
	 * 当期望从任意一个信道接收数据，而且希望知道收到的数据究竟来自于哪个信道时，使用这个方法
	 * @param head 要发送的数据包的包头结构(及其包头同步字段的值)
	 * 关于读取定长包的说明，见前一个Read。
	 * @return 接收到的完整数据包，以及收到数据的信道。
	 * 如果Pacakge::GetLength() == 0，则说明我们目前连接的所有通道的发送端都已被对方关闭，不会再有数据包了
	 * @exception SockError 底层(winsock)通信异常
	 */
	template<class HT> std::pair<BrPackage<HT>,Channel> read(const HT &head) 
	{
		BrComm::Channel chn = BrComm::ANY_CHANNEL;
		BrBufferMan packdata = read(&head, chn, true);
		BrPackage<HT> pack(packdata);
		return std::make_pair(pack,chn);
	}

	/**
	* 接收数据(非阻塞)
	* 读取已经收到的数据包，如果当前没有现成的数据包在，则返回空
	* @param head 要发送的数据包的包头结构(及其包头同步字段的值)，见前面Read()的说明
	* @param ch 选定的信道，如果不关心数据来自于哪个信道，使用Comm::ANY_CHANNEL以表明任何一个已有数据的信道数据包都可以
	* @return 接收到的完整数据包，如果Pacakge::GetLength() == 0，则目前没有可用的数据包
	* @exception BrSockError 底层(winsock)通信异常
	* @exception BrCommError 如果缓冲区为空且连接已经关闭则抛出CE_CONNECT_CLOSE异常
	*/
	template<class HT> BrPackage<HT> noblock_read(const HT &head, Channel ch = BrComm::ANY_CHANNEL) 
	{
		BrBufferMan packdata = read(&head,ch,false);
		BrPackage<HT> pack(packdata);
		return pack;
	}
	/**
	 * 批量读取
	 * 一次读取多个数据包，直到接收队列已取空，或者缓冲区已经放不下更多的数据包。
	 * 批量读取不等待，如果接收缓冲区没有数据会立即返回。
	 * 批量读取只能读取来自一个通道的数据包。
	 * @param buffer 存放数据包的缓冲区
	 * @param buf_len 缓冲区的长度
	 * @param head 要发送的数据包的包头结构(及其包头同步字段的值)，见前面Read()的说明
	 * @param ch 选定的信道，如果不关心数据来自于哪个信道，使用Comm::ANY_CHANNEL以表明任何一个已有数据的信道数据包都可以
	 * @param wait_data 目前还没有数据的话，是否要等待数据的到来
	 * @return 接收到的完整数据包数组，空的数组表示没有数据或者缓冲区的长度不够存放一个数据包
	 * @exception SockError 底层(winsock)通信异常
	 */
	template<class HT>
	std::vector<BrPackage<HT> > batch_read(char *buffer, size_t buf_len, const HT &head, Channel ch, bool wait_data = true) {
		size_t nread = read(&head, ch, buffer, buf_len, wait_data);
		std::vector<BrPackage<HT> > v;
		while ( nread > 0 ) {
			HT *hd = reinterpret_cast<HT*>(buffer);
			v.push_back(BrPackage<HT>(hd, buffer + sizeof(HT), false));
			nread -= hd->pack_len;
			buffer += hd->pack_len;
		}
		return v;
	}
	/*
	 * 获取IO操作的统计信息

	 * @param input_stat 存放“接收”信息的变量
	 * @param output_stat 存放“发送”信息的变量
	 */
	void get_iostat(BrIOStat &input_stat, BrIOStat &output_stat);
	/**
	 * 获取异步操作的错误信息
	 * @param chn 要查询错误信息的通道。如果为ANY_CHANNEL，返回第一个有错误并且没有返回过错误的通道信息。
	 * @return Winsock错误码，以及发生错误的通道。没有错误时错误码返回0。
	 */
	std::pair<int, Channel> fetch_sock_error(Channel chn = ANY_CHANNEL);
	/**
	 * 获取对端地址
	 * @param chn 选定通道
	 * @return 对端地址。如果找不到给定的通道，返回地址ANY。
	 */
	BrAddress get_peer(Channel chn);

	/**
	 * 获取当前客户端的地址及端口
	 * @param local 
	 */
	BrAddress get_local();

	/**
	 * 关闭通信，会扔掉所有未处理完的包，关闭使用资源
	 */
	void close();
	/**
	 * 清空读写队列。队列中的数据被丢弃
	 */
	void clear();
private:
	/**
	 * 发送数据包
	 * 发送是先发送包头，然后紧接着发送数据。
	 * @param packet 要发送的数据包缓冲区
	 * @param channel 输入：要发送数据的信道；输出：实际发送数据的信道(输入ANY_CHANNEL时)
	 * @return 当前发送队列长度
	 */
	size_t write(const char *head, size_t head_len, const char *data, size_t data_len, Channel channel);
	/**
	 * 接收数据包
	 * 要求的数据格式：先是包头，然后紧跟着是数据。包头必须是GenericHeader格式，头两个字段分别是标志和数据包长度。
	 * 特别约定：每个“通信”，如果有多个“信道”的话，各通道能接收的所有包的同步字段的值都是相同的！
	 * @param head 要接收数据包的通用包头信息，要使用到其中的sync_flag，数据的长度由读取数据的pack_len确定
	 * @param channel 输入：要接收数据的信道；输出：实际接收数据的信道(输入ANY_CHANNEL时)
	 * @param sync	如果没有数据是否等待，true：等待；false：不等待直接返回
	 * @return 收到的数据，内存在读取过程中分配，在返回对象(及其所有拷贝)生命周期结束的时候会自动释放内存
	 */
	BrBufferMan read(const BrGenericHeader *head, Channel &channel, bool sync);

	/**
	 * 为了批量读取数据提供的另一个实现
	 * 此方法会尽力读取最多的数据包，直到缓冲区不能存放更多的数据包，或者接收缓冲已经没有数据包了才返回。
	 * @param head 同上一个方法
	 * @param channel 同上
	 * @param buffer 用于存放返回的数据包的缓冲区
	 * @param size 缓冲区长度
	 * @return 返回的数据包的总长度。数据包以一个紧跟一个的方式存放，每个数据包的开头都是一个GenericHeader，
	 * 其中包含了包的长度。从缓冲区开始根据总长度和各包长度可以把全部包分离出来。
	 */
	size_t read(const BrGenericHeader *head, Channel &channel, char *buffer, size_t size, bool sync);
};



// 以服务器角色发起的"通信"
class _COMM_API_ BrServerComm : public BrComm {
public:
	/**
	 * @param gen_buf_size	保存未指定通道的待发数据的发送缓冲长度
	 * @param chn_buf_size	每个通道的缓冲长度（接收缓冲和发送缓冲都是这个长度）
	 */
	BrServerComm(size_t gen_buf_size = 1024*1024*8, size_t chn_buf_size = 1024*1024*2);
	/**
	 * 服务器端的一个重要功能就是监听和接收连接
	 * 监听
	 * @param sap 监听地址(服务访问点，可以是ip+port，也可以只有port，此时监听所有本地ip地址)
	 * @exception SockError Winsock错误，CommError通信逻辑错误。
	 */
	void listen(const BrAddress &sap);

	// 是否需要显式的accept服务？
	/**
	 * 接收客户端发起的连接
	 * @return 已建立连接的信道标识。如果返回ANY_CHANNEL，则说明连接不成功，但是系统会继续尝试连接。
	 * @exception SockError Winsock错误，CommError通信逻辑错误。
	 */
	BrComm::Channel accept();
	/**
	 * 关闭指定的信道
	 */
	void close(BrComm::Channel s);
	/**
	* 关闭全部信道
	*/
	void close() { return BrComm::close(); }

	/**
	 * 遍历所有活动的信道
	 */
	std::set< BrComm::Channel > enum_channels();
};
// 以客户端角色发起的"通信"
class _COMM_API_ BrClientComm : public BrComm {
public:
	/**
	 * @param gen_buf_size	保存未指定通道的待发数据的发送缓冲长度
	 * @param chn_buf_size	每个通道的缓冲长度（接收缓冲和发送缓冲都是这个长度）
	 */
	BrClientComm(size_t gen_buf_size = 1024*1024*8, size_t chn_buf_size = 1024*1024*2);
	/**
	 * 客户端主动建立连接
	 * @param sap 通信对方的地址(服务访问点，应该是ip+port)
	 * @return 已建立连接的信道标识
	 */
	BrComm::Channel connect(const BrAddress &sap);
	/**
	 * 建立连接前，可强制绑定本地端口及地址
	 * @param local 本地端口/地址
	 */
	void bind(const BrAddress &local);
	
};

/**
 * 面向包的组播发送类
 */
class BrDGPackSender : public BrDataGramSender
{
public:
	BrDGPackSender(unsigned short port, const char *multicast_ip, const char *if_ip):
		BrDataGramSender(port, multicast_ip, if_ip)
	{

	}
	template<class HT> void send( HT *header, const char *data)
	{
		char *buf = new char[header->pack_len];
		memmove(buf, header, sizeof(HT));
		memmove(buf + sizeof(HT), data, header->pack_len - sizeof(HT));
		size_t sended_all = 0;
		do
		{
			int sended = BrDataGramSender::send(buf, header->pack_len);
			if (sended < 0)
			{
				delete []buf;
				
				throw BrSockError(BrDataGramSender::GetClearError());
			}
			sended_all += sended;
		} while( sended_all < header->pack_len );
		delete []buf;
	}
};
// 统计信息
struct BrIOStat {
	int64_t total_packets; // 收发的数据包数
	int64_t total_bytes; // socket操作完成的字节数
	int64_t bytes_in_packet; // 收发的包中字节总数
	int64_t bytes_in_queue; // 仍然在队列中的字节数
	int64_t bytes_drop; // 扔掉的字节数：接收不同步，发送出错都会导致丢弃数据！
};

#endif // __COMMUNICATION_H_
