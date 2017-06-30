/**
 * \file common_bc.h
 * BC适配器头文件.
 *
 *    \date 2012-9-4
 *  \author xiaoqing.lu
 */
#ifndef _COMMON_BC_H_
#define _COMMON_BC_H_

#include "thread.h"
#include "common_qiu.h"
#include "stdio.h"
#include "stdlib.h"
//#include "gist/agent.h"
//#include "gist/gist.h"
//#include "gist/factory.h"
#include "propcfg.h"
#ifdef WIN32_DDC
#include "include/stdemace.h"
#elif defined(IPCORE)
#include "IPCORE.h"
#else
//#include "include_vx/stdemace.h"
#endif
#include <time.h>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <memory>
//#include "gist/icd/icdman.h"
//#include "gist/icd/BrBasicParser.h"
#include "comm_bc.h"
#include "types.h"
#include "zynq_1553b_api.h"
#include "msgop.h"
//using namespace ICD;
using namespace Agent;
using namespace std;

typedef struct HeadInfo 
{
	size_t     len;
	enum_operate_type  type;
}stHeadInfo;

typedef unsigned char byte;
///设备信息
typedef struct _DEVICE_INFO 
{
	string hostAddr;
	int cardNum; ///< 逻辑设备号
	unsigned long interval; ///< 时间间隔，单位：us
	std::string address; ///< 地址
	std::string subAddress[32]; ///< 子地址
}Device_info;

///消息
class BC_Msg : public BC_Msg_info
{
public:
	BC_Msg();
	~BC_Msg();
	void Create( S16BIT numCard);///< 创建一个消息
	void Modify( S16BIT numCard);///< 修改一个消息
	short GPF;					///< 控制同步消息是否发送	
	unsigned long long m_irqCount;///< 当前消息的在中断中的次数
	U16BIT m_missFreq;			///< 漏发消息频率，每missFreq个中漏发一个，0:全发;1:停发
	//BrBasicParser* m_parser; //zhanghao close.
	//Block* m_block; //zhanghao close.

	//void InitTimecode(const ICDMan* icdman, const Interface* ref_if);  //zhanghao close.
	//void SetTimecode(Agent::ISysServ *svr);   //zhanghao close.

	void loadSimFile();		///< 载入仿真文件
	void SetBCData(S16BIT numCard, Agent::ISysServ *svr); ///< 设置数据
	void SetFileData();		///< 设置从文件读出的数据
	void SetCRC();			///< 设置校验和
	char* m_buff;			///< 缓存仿真文件内容
	U32BIT m_buffSize;	///< m_buff的长度
	U64BIT m_packCount;		///< 读缓存的次数
	char* m_pbuff; 			///< 保存读取文件缓存的当前位置
	char* m_CRCBuff; 		///< 包尾校验时保存包数据
	short m_CRCBuffSize;	///< m_CRCBuff的长度
	BOOL m_isBrokenMode; 	///< 是否处于故障模式

	U16BIT nowOpt;			///< 当前的通道和重试
private:
	U16BIT GetOpt();		///< 取得消息的通道和重试选项
};

///操作
class BC_Ops
{
private:
	static S16BIT s_op_id;
public:
	static S16BIT GetAOpID() { return s_op_id ++; }///< 获取一个新的操作ID
	void AddOp( S16BIT devNum, S16BIT codeType, S16BIT msgid = 0 );///< 添加操作
	void AddMsg( S16BIT devNum, BC_Msg &msg, S16BIT condition = ACE_CNDTST_ALWAYS );///< 添加消息
	void AddDelay( S16BIT devNum, U16BIT interval, S16BIT condition = ACE_CNDTST_ALWAYS );///< 添加延时
	S16BIT* GetArrayAddr();///< 获取操作列表的操作ID列表
	U16BIT GetSize() { return (U16BIT)m_lstOps.size(); } ///< 获取操作列表的长度
	BC_Ops():m_pOps( NULL ) {}
	~BC_Ops(){
		if( m_pOps != NULL )
			delete [] m_pOps;
	}
	static void init(){///< 初始化操作ID
		s_op_id = 400;
	}
private:
	std::vector<S16BIT> m_lstOps;///< 操作ID列表
	S16BIT* m_pOps;///< 操作ID内存指针
};

///判断信息
class FollowInfo{
public:
	FollowInfo(short pfollowMsgID, unsigned short pfollowValue, unsigned short pfollowMask, 
			   short pfollowDataindex, BOOL pisCheckSrvbit)
		:followMsgID(pfollowMsgID), 
		followValue(pfollowValue), 
		followMask(pfollowMask), 
		followDataindex(pfollowDataindex),
		isCheckSrvbit(pisCheckSrvbit){}
	///重载==操作符
	bool operator==( const FollowInfo f){
		if (followMsgID == f.followMsgID &&
			followValue == f.followValue &&
			followMask == f.followMask &&
			followDataindex == f.followDataindex &&
			isCheckSrvbit == f.isCheckSrvbit){
			return true;
		} 
		else{
			return false;
		}
	}
private:
	short followMsgID;			///< 根据哪个消息判断是否要发
	unsigned short followValue;	///< 判断值
	unsigned short followMask;	///< 判断MASK
	short followDataindex;		///< 判断该消息的第几个数据字
	BOOL isCheckSrvbit;			///< 是否判断消息的服务请求位
};

/// 缓存write进来的数据，以便在receiver线程中处理。所以这个数据结构要求是线程安全的
class BrDataBuffer
{
	map<U16BIT, queue< pair<stHeadInfo, const char *> > > map_buf; ///< 缓存dataheader和数据
	map<U16BIT, queue< pair<stHeadInfo, const char *> > >::iterator it;
	MXLock m_lock;///< 保证缓冲的读写时线程安全的
public:
	BrDataBuffer(){}
	~BrDataBuffer()
	{
		map_buf.clear();
	}
	
	void push(U16BIT index, const stHeadInfo &h , const char* d)
	{
		if (d)
		{
		       size_t len = h.len;
		       char *data = new char[len];
			memset((void *)data, 0, sizeof(char) * len);
			memcpy(data, (void *)d, len);
			if(map_buf.find(index) == map_buf.end())
			{
				queue< pair<stHeadInfo , const char *> > my_que;
				map_buf.insert(std::make_pair(index, my_que));
			}
		    AutoLock l(m_lock);	
		    map_buf.find(index)->second.push(std::make_pair(h, data));		
		}
	}

	   /// 获得队首的 dataheader
	stHeadInfo *get_front_header(U16BIT index)
	{
	       it = map_buf.find(index);
		if(it != map_buf.end())
		{
		    AutoLock l(m_lock);
		    stHeadInfo *h = (it->second.size() > 0) ? &(it->second.front().first) : NULL;
		    return h;
		}
		else
		{
		   return NULL;
		}
	}
	   
	/// 获得队首的数据
	const char * get_front_data(U16BIT index)
	{
		it = map_buf.find(index);
		if(it != map_buf.end())
		{
		   AutoLock l(m_lock);
		   const char *d = (it->second.size() > 0) ? (it->second.front().second) : NULL;
		   return d;
		}
		else
		{
		   return NULL;
		}
	}

	void pop(U16BIT index)
	{
		it = map_buf.find(index);
		if(it != map_buf.end())
		{
		   AutoLock l(m_lock);
		   delete []it->second.front().second;
		   it->second.pop();
		}
	}
	
	void clear()
	{
		map_buf.clear();
	}

	size_t size(U16BIT index)
	{
		size_t s = 0;
		it = map_buf.find(index);
		if(it != map_buf.end())
		{
		       AutoLock l(m_lock);
			s = it->second.size();
		}
		return s;
	}

	bool empty(U16BIT index)
	{
            it = map_buf.find(index);
	     if(it != map_buf.end())
	     {
	 	   AutoLock l(m_lock);
	 	  return it->second.empty();
	     }
	}
};

///BC适配器
class BrAda1553B_BC_COMM: public Agent::IAdapter
{
public:
	vector<BC_Msg> m_vec_msg; ///< 消息列表
	set<S16BIT> m_set_followMsgID; ///< 哪些消息是判断别的消息是否发送的依据
	multimap<S16BIT, S16BIT> m_multimap_followMsg; ///< <依据消息ID，被判断消息index>
	int m_minorFrameCount; ///< 小帧个数
	map<S16BIT, S16BIT> m_mapID; ///< <消息ID， 消息Index>
	set< pair<S16BIT, S16BIT> > m_set_msgPlace; ///< <小帧ID，消息ID>
	map< S16BIT, vector<S16BIT> > m_map_msgPlace; ///< <小帧ID，vec<消息ID>>
	set< S16BIT > m_set_msgID_inFrame; ///<排在小帧中的消息>
	BOOL m_isBrokenMode; ///< 是否处于故障模式
	string m_fileName;   ///< 配置文件名

	set<S16BIT> m_setSaveFlag; ///< <消息ID>
	MXLock my_Lock;

public:
	static std::map<S16BIT, BrAda1553B_BC_COMM*> s_adapt; ///< 通道号，对应的实例指针
	BrAda1553B_BC_COMM();
	virtual ~BrAda1553B_BC_COMM()
	{
		s_adapt.erase( device_info_bc.cardNum );
		work_stat = WS_EXIT;
	};

	//virtual long capability() const {return DC_WRITE | DC_IFID_REQ; }; zhanghao close.
	//virtual void set_interface(const Interface *if_obj);  zhanghao close.
	virtual U16BIT set_config(const std::string &cfg);
	virtual void set_address(const string &host_addr, const std::string &if_addr);
	virtual U16BIT source_init(); 
	virtual U16BIT start();
	virtual U16BIT stop();
	virtual U16BIT write(ctl_data_wrd_info *datahead);
	virtual void ioctl(U16BIT msg_id, msg_is_save flag, const char *save_path);
	//virtual ssize_t read( Agent::DevDataHeader *head, char *buffer,size_t);  zhanghao close.
	//virtual long version() { return DIV_ORIGIN; };   zhanghao close.

	static void _DECL MyISR( S16BIT DevNum, U32BIT dwIrqStatus );//中断处理函数

private:
	volatile enum {
		WS_INIT, WS_RUN, WS_STOP, WS_ERROR, WS_EXIT
	} work_stat;


	void MakeMinorOps( S16BIT devNum, BC_Ops* ops );///< 创建小帧里的操作
	void MakeMajorOps( S16BIT devNum, BC_Ops& ops );///< 创建大帧里的操作

public:
	static const int MAJOR_FRAME_ID;///< 大帧ID
	unsigned int MINOR_FRAME_TIME;	///< 小帧延时
	unsigned int MAJOR_FRAME_TIME;	///< 大帧延时
	FILE  *fp;
private:
	BrDataBuffer m_write_data_buf;	///< 用于缓存write进来的数据，以便在receive线程中处理
	BrDataBuffer m_read_data_buf;	///< 用于缓存将要被read读走的数据
	//const Interface* m_interface;  	///<接口指针  zhanghao close.

public:
	Agent::ISysServ * m_sys_service;///< 保存系统服务对象
	Device_info device_info_bc;		///< 设备信息
	unsigned long long m_itr_count;	///< 发生的中断个数
	short m_GPF;					///< 配置GPF用，对于消息来说取值为：2、3、4、5、6
	short m_itrMsgCount;			///< BC配置中断的个数
	map<short,short> m_mapGPF; 		///< <GPF, messId>使用该GPF的最后一个消息，用于同步判断消息的GPF重用
	map<string,string> m_item_value;   ///< 配置标题和数据的保存
	map<int,vector<string> >data_value;  ///< DATA[32]数据的保存
	map<int,vector<string> >member_value;  ///<保存bc_msg其它成员的值
	void ProcessVector(void);		///< 处理根据其他消息数据发送的消息
	void RecvAsyncMsg(short msgID);
	void deal_data(U16BIT messID);				///< 处理write进来的数据
	bool  readConfFile(string fileName); 	///<读入配置文件
	void CombSetConfig(const string & cfg);  ///<重新组合和设置配置文件
	void parseConfFile(void);		///< 解析配置文件
	static void print_err_str(S16BIT result);		///< 根据errorno输出错误信息
	void Sub_Data(string str,vector<string> & vecStr,char cc);
	void CombBcMsgData(void);
	void ParserData(string &str);
	void GetBcMsgData(int i, char  *data);
	bool get_item(const string &item, string &value);
};

#endif /*_ADA1553_BC_H_*/
