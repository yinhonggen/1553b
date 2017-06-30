/**
 * \file net/ntpdata.h
 *
 * ntp data parser & time set/get ultilies.
 *
 *    \date 2009-7-14
 *  \author anhu.xie
 */

#ifndef NET_HOST_ORDER_H_
#define NET_HOST_ORDER_H_

#if defined(WIN32)
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )
#else
#include <arpa/inet.h>
#endif

// 下面模版里用到的工具函数，用Specialization实现另外一种形式的多态
template <typename T> inline T host2net(T d) { return htonl(d); }
template<> inline unsigned short host2net(unsigned short d) { return htons(d); }
template<> inline short host2net(short d) { return htons(d); }
template<> inline char host2net(char d) { return d; }
template<> inline unsigned char host2net(unsigned char d) { return d; }

template <typename T> inline T net2host(T d) { return ntohl(d); }
template<> inline unsigned short net2host(unsigned short d) { return ntohs(d); }
template<> inline short net2host(short d) { return ntohs(d); }
template<> inline char net2host(char d) { return d; }
template<> inline unsigned char net2host(unsigned char d) { return d; }

#endif /* NET_HOST_ORDER_H_ */
