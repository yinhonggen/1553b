/**
 * \file net/ntpdata.h
 *
 * ntp data parser & time set/get ultilies.
 *
 *    \date 2009-7-14
 *  \author anhu.xie
 */

#ifndef NTP_DATA_STRUCT_H_
#define NTP_DATA_STRUCT_H_

#ifdef __vxworks
#include <socklib.h>
#include <time.h>
#elif !defined(WIN32)
#include <sys/time.h>
#endif /* __vxworks */

#if defined(WIN32)
#include <winsock2.h>
#undef  ERROR
#define ERROR SOCKET_ERROR
#else
#include <arpa/inet.h>
#endif
#include <string.h>
#include <memory>
#include <limits>
#include "net2host.h"

#ifdef max
#undef max
#endif

/**
 * NTP时间模板.
 * NTP时间由两部分组成，整数部分是从1900年1月1日0点开始的秒数，小数部分代表微秒，用一个整数来按比例存放，即这个类型的整数能表达的最大值代表1秒。
 * 这是一个模版类：ntp中有32位和64位的时间，用此模版来表示。
 * \tparam SST 秒部分的数据类型，可能是带符号的
 * \tparam UST 秒部分数据类型的对应无符号数类型；小数部分总是使用无符号数
 */
template<typename SST, typename UST>
class NtpTime {
	SST sec; // second part, signed, maybe
	UST usec; // microsecond part, short.max() == 1s, always added to 'sec'(even when sec is negative)
public:
	static const unsigned int US_PER_SECOND = 1000000; // microseconds!
	static UST Encode(long src) {
		return host2net(src ? (UST)((double)src / US_PER_SECOND * std::numeric_limits<UST>::max()) : (UST)0);
	}
	static long Decode(UST src) {
		UST v = net2host(src);
		return v ? (long)((double)v / std::numeric_limits<UST>::max() * US_PER_SECOND) : 0l;
	}
	NtpTime(SST s, long us) : sec(host2net(s)), usec(Encode(us)) {}
	void set_sec(SST s) { sec = host2net(s); }
	void set_micro(UST u) { usec = Encode(u); }
	SST get_sec() const { return net2host(sec); }
	long get_micro() const { return Decode(usec); }
};

/// NTP短(32位)时间
typedef NtpTime<short, unsigned short> NtpShortTime;

/// NTP长(64位)时间，以及到UNIX时间的转换
class NtpTimeStamp : public NtpTime<long, unsigned long> {
	static const unsigned int TIME_T_OFFSET = 2208988800u; // difference between time_t and NTP 'seconds'
	typedef NtpTime<long, unsigned long> Parent;
public:
	NtpTimeStamp(long s, long us) : Parent(s ? s + TIME_T_OFFSET : 0, us) {}
	void set_sec(long s) { Parent::set_sec(s ? s + TIME_T_OFFSET : 0); }
	long get_sec() const { long a = Parent::get_sec(); return a ? a - TIME_T_OFFSET : 0; }
	void set_time(const struct timeval &src) {
		set_sec(src.tv_sec);
		set_micro(src.tv_usec);
	}
	void get_time(struct timeval &dst) const {
		dst.tv_sec = get_sec();
		dst.tv_usec = get_micro();
	}
};

/// NTP数据包
struct NtpData {
	unsigned char mode:3; // note: LSField first! so this is the 3rd field.
	unsigned char version_number:3;
	unsigned char leap_indicator:2; // this is the 1st field!
	unsigned char clock_stratum;
	unsigned char polling_interval;
	signed char clock_precision;
	NtpShortTime root_delay;
	NtpShortTime clock_dispersion;
	char reference_clock_id[4];
	NtpTimeStamp reference_update_time;
	NtpTimeStamp originate_time_stamp;
	NtpTimeStamp receive_time_stamp;
	NtpTimeStamp transmit_time_stamp;
	NtpData() : mode(5), version_number(4), leap_indicator(0),
		clock_stratum(0), polling_interval(10), clock_precision(-6),
		root_delay(0,123456), clock_dispersion(-1, 98765), reference_update_time(0,0),
		originate_time_stamp(0,0), receive_time_stamp(0,0), transmit_time_stamp(0,0)
	{
		memcpy(reference_clock_id,"LOCL", sizeof reference_clock_id);
	}
};

#endif /* NTP_DATA_STRUCT_H_ */
