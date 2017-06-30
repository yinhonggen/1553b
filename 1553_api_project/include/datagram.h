#ifndef __DATAGRAM_N_MULTICAST_H__
#define __DATAGRAM_N_MULTICAST_H__

#ifdef __vxworks
#include <socklib.h>
#include <sigLib.h>
#endif

#if defined(WIN32)
#include <winsock2.h>
#pragma comment(lib,"WS2_32.lib")
#include <Ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <memory>

/**
 * generial socket in internet domain.
 * datagram or stream based.
 */
class INetSock {
public:
#ifdef WIN32
	typedef SOCKET sock_t;
	static inline void close(sock_t s) { closesocket(s); }
	static inline int get_errno() { return GetLastError(); }
	typedef long ssize_t;
	typedef unsigned long in_addr_t;
	typedef int socklen_t;
#define SO_REUSEPORT SO_REUSEADDR
#undef EINTR
#define EINTR WSAEINTR
#undef EMSGSIZE
#define EMSGSIZE WSAEMSGSIZE
#else
	typedef int sock_t;
	static inline int get_errno() { return errno; }
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif
private:
	int m_errno;
	bool m_has_error;
protected:
	sock_t m_sock;
#if defined(__vxworks) || defined(WIN32)
	// remap signatures to make them compatiable with UNIX. UNIX sig. is better;)
	static inline unsigned long inet_addr(const char *a) { return ::inet_addr(const_cast<char*>(a)); }
	static inline int setsockopt(int s, int l, int o_name, void *o_value, socklen_t len) {
		return ::setsockopt(s, l, o_name, reinterpret_cast<char*>(o_value), len);
	}
	static inline int recvfrom (int s, void *buf, int bufLen, int flags, struct sockaddr *from, socklen_t *pFromLen) {
		return ::recvfrom(s, reinterpret_cast<char*>(buf), bufLen, flags, from, reinterpret_cast<int*>(pFromLen));
	}
	static inline int sendto(int s, const void *buf, int bufLen, int flags, struct sockaddr *to, int tolen) {
		return ::sendto(s, reinterpret_cast<char*>(const_cast<void*>(buf)), bufLen, flags, to, tolen);
	}
#endif /* __vxworks || WIN32 */
public:
	INetSock(bool datagram = true) : m_errno(0), m_has_error(false), m_sock(INVALID_SOCKET) {
		m_sock = socket(AF_INET, datagram ? SOCK_DGRAM : SOCK_STREAM, 0);
		if ( m_sock == INVALID_SOCKET )
			SetError(get_errno());
	}
	~INetSock() {
		if ( m_sock != INVALID_SOCKET ) {
			close(m_sock);
			m_sock = INVALID_SOCKET;
		}
	}
	int GetError() const { return m_has_error ? m_errno : 0; }
	int GetClearError() { int r = GetError(); m_errno = 0; m_has_error = false; return r; }
	void SetError(int err) { m_errno = err; m_has_error = true; }
	int CheckError(int e) { if ( e == SOCKET_ERROR ) SetError(get_errno()); return e; }
	bool HasError() const { return m_has_error; }
	operator bool() const { return ! HasError(); }
};

/**
 * Datagram Sender.
 * send data to uni_cast or multi_cast UDP port.
 */
class BrDataGramSender : public INetSock {
protected:
	struct sockaddr_in m_target_addr; // remember the destination SAP, and don't repeat @ every write!
public:
	/**
	 * @param port UDP_port to send the data
	 * @param multicast_ip destination ip address, it can be uni_cast ip actually
	 * @param if_ip the ip of the interface to send the multicast data.
	 * it's not needed when unicast (or default interface) is used.
	 */
	BrDataGramSender(unsigned short port, const char *multicast_ip, const char *if_ip = NULL)
	{
		m_target_addr.sin_family = AF_INET;
		m_target_addr.sin_port = htons(port);
		m_target_addr.sin_addr.s_addr = multicast_ip ? inet_addr(multicast_ip) : INADDR_ANY;
		if ( IN_MULTICAST(ntohl(m_target_addr.sin_addr.s_addr)) ) { // IN_MULTICAST is a host operation, while s_addr is in net-order
			in_addr_t local_ip = if_ip ? inet_addr(if_ip) : INADDR_ANY;
			if ( m_sock != INVALID_SOCKET && local_ip != INADDR_ANY ) {
				struct in_addr addr;
				addr.s_addr = local_ip;
				CheckError(setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)));
			}
		}
	}
	ssize_t send(const void *data, size_t length) {
		return HasError() ? SOCKET_ERROR : CheckError(sendto(m_sock, data, length, 0, reinterpret_cast<sockaddr*>(&m_target_addr), sizeof m_target_addr));
	}
};

class BrDataGramReceiver : public INetSock {
	struct ip_mreq m_mreq; // multicast joining request parameter
	bool m_multicast; // actually joined the multicast?
	void _init(unsigned short port, const in_addr_t &dest_in_addr, const in_addr_t &if_in_addr) {
		if ( HasError() )
			return;
		bool multicast_dst = IN_MULTICAST(ntohl(dest_in_addr));

		int buflen = 1024 * 64;
		CheckError(setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *)&buflen, sizeof buflen));
		if ( HasError() )
			return;
		int reuse = true;
		CheckError(setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse));
		if ( HasError() )
			return;
		sockaddr_in bind_addr;
#if defined(__vxworks) || defined(__MACH__)
		bind_addr.sin_len = sizeof bind_addr;
#endif
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_port = htons(port);
		bind_addr.sin_addr.s_addr = multicast_dst ? INADDR_ANY : if_in_addr;
		CheckError(bind(m_sock, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)));
		if ( HasError() )
			return;
		if ( ! multicast_dst )
			return;
		m_mreq.imr_interface.s_addr = if_in_addr;
		m_mreq.imr_multiaddr.s_addr = dest_in_addr;

		CheckError(setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m_mreq, sizeof(m_mreq)));
		m_multicast = !HasError();
	}

public:
	BrDataGramReceiver(unsigned short port, const in_addr &dest_addr, const in_addr &if_addr) : m_multicast(false) {
		_init(port, dest_addr.s_addr, if_addr.s_addr);
	}
	/**
	 * @param port UDP_port to receive the data
	 * @param multicast_ip destination ip address, it can be uni_cast ip(or none/NULL at all) actually
	 * @param if_ip the ip of the interface to receive the multi_cast data. 
	 * it's not needed when uni_cast (or default interface) is used. 
	 */
	BrDataGramReceiver(unsigned short port, const char *dest_ip, const char *if_ip = NULL) : m_multicast(false)
	{
		_init(port, dest_ip ? inet_addr(dest_ip) : INADDR_ANY, if_ip ? inet_addr(if_ip) :  INADDR_ANY);
	}
	~BrDataGramReceiver() { // leave the group when finished--or VxWorks OS will fail:( UNIX will auto-leave when the socket is closed.
		if ( m_multicast && m_sock != INVALID_SOCKET )
			CheckError(setsockopt(m_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &m_mreq, sizeof(m_mreq)));
	}
	ssize_t recv(void *buffer, size_t length, struct sockaddr *address = NULL, socklen_t *address_len = NULL) {
		// recvfrom阻塞后(只能)靠析构对象本身唤醒。由于close()是在基(父)类的析构函数中调用的，此时派生类对象已不存在，
		// 如果此时调用CheckError()设置对象属性，会出现修改已释放(甚至已分配给别人的)内存的情况，破坏内存。
		if ( HasError() )
			return SOCKET_ERROR;
		int nr = recvfrom(m_sock, buffer, length, 0, address, address_len);
		if ( nr == SOCKET_ERROR ) { // 如果recvfrom是由析构函数(在另外的线程中)的close()唤醒，任何成员都已经无效！
			int err = get_errno(); // when close()ed, vxworks gets EPIPE, and WIN32 gets EINTR!
			if ( err != EINTR && err != EPIPE )
				SetError(err);
		}
		return nr;
	}
};

#endif /* __DATAGRAM_N_MULTICAST_H__ */
