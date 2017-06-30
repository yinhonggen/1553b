#pragma once

#include "thread.h"
//#include "../../../../../include/gist/agent.h"
//#include "../../../../../include/gist/gist.h"
//#include "../../../../../include/gist/factory.h"


#include <time.h>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <queue>
#include <set>

using namespace std;

#include "RT_DataSource.h"
#include "ConfigSplit.h"
#include "RT_StateWord.h"

#ifndef WIN32
#include "time.h"
#endif

#ifndef IPCORE
#ifdef __vxworks
extern "C" void aceEnumCards();
extern "C" int ddccm(int initFamily, unsigned char bShowTable);
static class DDCDriverLoader {
public:
	DDCDriverLoader() {
		//		aceEnumCards();
		ddccm(1, 1);
	}
} load_ddc_driver;
#endif
#endif

/// 假设矢量字的地址33
const S16BIT  VECTOR_ADDR  = 33;

/// 1.应对根据收到的消息做处理的情况，通用RT不能达到的用这种方式。
/// 提供如下两个接口

/// 接收到消息后的函数
typedef void (* FUN_MSGRECV)(S16BIT wSa,S16BIT len,U16BIT data[]);

/// 修改子地址的数据
typedef void (* FUN_SUBADDRDATA)(S16BIT wSa,S16BIT len,U16BIT data[]);

/// 向外提供的接口
typedef struct _IntfaceFun 
{
	FUN_MSGRECV fun_msg;
	FUN_SUBADDRDATA  funData;
}_IntfaceFun;

class BrAda1553B_RT ;


/**
* 继承实现,Adapter接口。
* 
* 实现1553B协议下，完成通用RT的各个功能
*/
class BrAda1553B_RT: public Agent::IAdapter_V2,public Thread
{
public:

	/// 设置子地址数据源，可以大于32个字
	void SetSubAddrData(S16BIT subAddr,int len,U16BIT data[]);

	/// 当收到数据触发
	virtual void onMsgRecv(S16BIT wSa,S16BIT len,U16BIT data[]){
		return;
	};

public:

	BrAda1553B_RT(void);

	~BrAda1553B_RT(void);

public:
	///地址
	S16BIT iAddress ;

	///板卡号
	S16BIT cardNum;

      ///是否保存消息的标记
	S16BIT IsSaveFlag;

	FILE  *fp;

	/// 配置文件解析后的信息

public:

	/// 子地址，用map是为了方便查找,对应的数据内容
	map<S16BIT,_addrData> m_SubAddr; 

	/// 每个子地址的周期
	map<S16BIT,_CycleIndex>  m_SubAddrCyle; 

	/// 需要在数据被读走后刷新的子地址。
	set<S16BIT> m_forceSubAddr;

	/// 服务请求位周期变化
	_CycleIndex m_chgCyle_s;

	/// 子地址对应的变化消息
	map<RT_MSG, vector<S16BIT> > m_chgMsg_subAddr;	

	/// 收到指定消息，服务请求位置1
	set<RT_MSG> m_chgMsg_subState;

	/// 配置了时间码的子地址,校时和上报时间都在这里
	set<S16BIT> m_mapTimeCode;	

	/// 配置了三取二的消息数据源
	map<RT_MSG,vector<S16BIT> > m_mapThreeMsg;

	/// 数据拷贝对应的子地址
	map<RT_MSG,vector<S16BIT> > m_ResourceCopy_subAddr; 

	///状态字标识位
	RT_StateWord  m_StateWord;

	///  路由处理的函数
private:

	/* zhanghao close.
	/// 解析路由a
	GIST::BR_OPERATION_R parse_rout_a(const char *data, size_t len );

	/// 解析路由a,新的路由a，在路由的源地址、源子地址、目的地址、目的子地址，当中指定RT地址子地址
	GIST::BR_OPERATION_R parse_rout_a(const char *data, size_t len ,S16BIT subAddr);

	/// 解析路由b
	GIST::BR_OPERATION_R parse_rout_b(const char *data, size_t len );

	/// 解析路由c
	GIST::BR_OPERATION_R parse_rout_c(const char *data, size_t len );

	/// 故障路由处理
	GIST::BR_OPERATION_R parse_rout_error(const string & src_subAddr);
	*/
private:

	/// 启动时间
	_time_rt m_startTime;

	//BrTimeTriple  m_time_br;  zhanghao close.

	/// 计数器
	int m_TimeCount;

	/// 保存同步信号
	map<int,SyncEvent *> event_map;

	/// 定时器同步变量
	SyncEvent *m_tim_mutex;

	/// 操作1553B板卡时的锁
	MXLock m_Lock_card;

	///清理
	void clear();

	/// 根据消息，改变子地质数据
	int MsgMake_chgMsgAddr(vector<S16BIT> vecSubAddr);

	/// 根据消息改变服务请求位
	int MsgMake_chgMsgSq(const RT_MSG & vecRtMsg, S16BIT subAddr);

	/// 根据消息，拷贝数据源
	int MsgMake_chgMsgCopy(RT_MSG & rt,const MSGSTRUCT & msg,const vector<S16BIT> & vecSubAddr);

	/// 配置了三取二
	/// vector中的所有子地址都需要拷贝三取二的源
	int MsgMake_chgMsgThree(const vector<S16BIT> & vecSubAddr,int wWC,U16BIT * pData);

	/// 读消息并解析
	size_t ReadMsg();

	/// 更新数据块
	/// 返回是否更新完成
	/// 周期刷新会使用这里
	bool UpdataBlkData(S16BIT subAddr,int updataLen);

	/// 强制刷新
	void UpdataBlkData_force(S16BIT subAddr,int updataLen);

	/// 更新状态字
	void WriteStatus();

	/// 处理消息的数据字
	int MakeMsg_Parse(MSGSTRUCT * pMsg,U16BIT & wCmdWrd);

	/// 根据周期完成的事情
	void SizeCyle_Parse();

public:
	/// 中断处理相关
	static std::map<S16BIT, BrAda1553B_RT*> s_adapt;

	/// 中断处理函数
	static void _DECL MyISR( S16BIT DevNum, U32BIT dwIrqStatus );

	/// 处理的消息的个数
	unsigned long long m_msg_count; 

private:

	/// 配置解析对象
	RTconfigSeq  m_configSeq ; 

	bool m_bConfigError;

	/// ICDMan对象
	//const ICD::ICDMan * m_IcdMan;  zhanghao close.

	/// 接口指针
	//const Interface* m_interface;  zhanghao close.

	SyncEvent m_sync; ///< 用于子线程在停止后通知主线程

	bool m_Is_write_data;
public:
	/// 下面的函数是adapter的标准接口
	//virtual long version() { return DIV_V2; }

	//virtual long capability() const ;

	virtual void set_config(const std::string &cfg);

	virtual void set_address(const string &host_addr, const std::string &if_addr);
	virtual U16BIT source_init(SyncEvent *tim_mutex);
	virtual U16BIT  start();
	virtual U16BIT stop();

	//virtual ssize_t read(DataTime *time, Agent::DevDataHeader *head, char *buffer,size_t);  zhanghao close.

	//virtual std::multimap<std::string, std::string> get_bus_addr();

	//virtual void write_data(const char *data, size_t len, const std::pair<std::string, std::string> *channel);
	virtual int write(U16BIT subaddr, const char *data, size_t len);

	virtual void do_job();

	virtual unsigned long get_timer_interval();

	//virtual void set_interface(const Interface *if_obj); zhanghao close.

       virtual void  ioctl(U16BIT subaddr, msg_is_save flag, const char *save_path);
        
	/*void start() {
		Thread::start("1553B_commu_RT");
	}*/
};
