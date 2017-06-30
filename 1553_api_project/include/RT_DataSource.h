#pragma once
#include <time.h>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <queue>
#include <set>
using namespace std;
#include "Define.h"
#include "SA_SimFile.h"
#include "ThreeCatch.h"
#include "common_qiu.h"
//#include "../../../../../include/gist/gist.h"
//#include "../../../../../include/gist/agent.h"

//using namespace ICD;
class DataSource;

// 配置文件数据源的结构
typedef struct _fileConfig
{
	//string m_strFileName;
	//bool bIsBagBag; // 是否有小包	
	//SimlBag m_bagBag;// 小包类
	string fileName; // 文件名称
	U16BIT defWord;  // 不足32字时的补值
	bool bAffect;	 // 小包是否有效
	SimlBag bagBag;	 // 小包
	_fileConfig()
	{
		bAffect = false;
	}
}_fileConfig;


// 时间码的接口
class time_Inter
{
public:
	time_Inter() {

	};
	virtual ~time_Inter(){

	};

public:
	/*  zhanghao close.
	// 时间码转换,编码,将time编码到pData中
	// 返回编码后的长度
	virtual int ConvertTime_Code(char * pData,int len,const GIST::BrTime & time){
		return true;
	};

	// 时间码转换,解码,从pData中解析出time
	// 返回解析后的长度
	virtual int ConvertTime_Encode(const char * pData,int len,GIST::BrTime & time){
		return true;
	};
	*/
};

// 子地址的数据内容结构              
typedef struct _addrData 
{
	U16BIT dataBuff[32 +1];   //32个字的缓存
	DataSource * pDataSource; // 数据源信息

	S16BIT blockID_TX; 
	S16BIT blockID_RX; 
	bool flushFlag ; // 刷新标记

	_addrData()
	{
		flushFlag = false;
		blockID_RX = 0;
		blockID_TX =0;
		memset((void *)dataBuff,0,sizeof(S16BIT) * 33);
	}
} _addrData;

class DataSource;

// CRC校验
S16BIT CheckoutCRC(U16BIT * pData,S16BIT len);

// 累加和校验
S16BIT CheckoutADD(U16BIT * pDAta,S16BIT len);

// 校验和处理函数
typedef S16BIT (* FunCheckout)(U16BIT * pData,S16BIT len);

// 校验函数表
const FunCheckout g_FunArr[3] = {NULL,CheckoutCRC,CheckoutADD};

// 
// 子地址配置信息结构
class SA_Config
{
public:
	SA_Config(){
		saAddr = 0;
		memset((void*)defDataArry,0,sizeof(U16BIT)*32);
		timeCode.codeType = TIME_CODE_error;
		theckOut.type = _ERROR;
		threeObj.eType = _three_error;
		wordLen = 32;
		bNoCgh = false;
		m_bForceFlush = false;
		IsBigWord = false;
	};

	~SA_Config(){
	};

public:
	S16BIT saAddr; // 子地址

	// 时间码
	_TimeCode timeCode;

	// 三取二配置
	_three_struct  threeObj;

	// 校验和
	_CheckOutStu theckOut;

	// 是否大端
	bool IsBigWord;
public:
	// 子地址数据长度
	S16BIT wordLen; 

	// 文件数据源
	_fileConfig fileCfg;

	// 默认值的缓存
	U16BIT defDataArry[32];   //32个字的缓存

	// 消息拷贝
	// set<RT_MSG> m_msgCopy; 

	// 数据变化配置项
public:

	// 消息改变
	// set<RT_MSG> m_msgChg;

	// 周期变化
	_CycleIndex m_CyleInfo;

	// 固定值
	bool bNoCgh;

	// 取走后更新
	bool m_bForceFlush;
};

// 状态字的配置
class StateWord_Config
{
public:
	StateWord_Config(){
		bConfigFlag =false;
	};
	~StateWord_Config();

	bool bConfigFlag;
public:
	int ServiceRequest;// 服务请求位
	int SystemFlag;   // 子系统特征位
	int TerimterFlag; // 终端标识位
	int BusyFlal ;// 忙位

public:
	// 服务请求为周期变化
	_CycleIndex m_CyleInfo;

	// 服务请求位消息触发
	set<RT_MSG> m_msgChg;
};


// 矢量字的配置
class VectorWord_Config
{
public:
	VectorWord_Config();
	~VectorWord_Config();
};

class WriteDataBuffer
{
	  map<U16BIT, queue< pair<size_t, U16BIT *> > > map_buf;
	  map<U16BIT, queue< pair<size_t, U16BIT *> > >::iterator it;
	  //U16BIT data[32+1]; // //32个字的缓存
	  MXLock m_lock;///< 保证缓冲的读写时线程安全的
public:
	WriteDataBuffer(){}
	~WriteDataBuffer()
	{
		map_buf.clear();
	}
	void push(U16BIT index, size_t len, const char* d)
	{
		if (d)
		{
		       U16BIT *data = new U16BIT[32];
			size_t  data_size_1553 = len;
			bool is_odd_size = len % 2 > 0;
			if (is_odd_size)
			{
			    data_size_1553 = len + 1;
			}
			char *data1553 = const_cast<char*>(d);
			if(is_odd_size)
			{
                           data1553 = new char[data_size_1553];
				memset(data1553, 0, data_size_1553);
				memcpy(data1553, d, len);
			}
			memset((void *)data, 0, sizeof(data));
			memcpy(data, (unsigned char *)data1553, data_size_1553);
			if (is_odd_size)
			{
			    delete[] data1553;
			}
			if(map_buf.find(index) == map_buf.end())
			{
				queue< pair<size_t , U16BIT *> > my_que;
				map_buf.insert(std::make_pair(index, my_que));
			}
		    AutoLock l(m_lock);	
		    map_buf.find(index)->second.push(std::make_pair(data_size_1553, data));		
		}
	}

	 /// 获得队首的 dataheader
	size_t *get_front_header(U16BIT index)
	{
	       it = map_buf.find(index);
		if(it != map_buf.end())
		{
		    AutoLock l(m_lock);
		    size_t *msg_len = (it->second.size() > 0) ? &(it->second.front().first) : NULL;
		    return msg_len;
		}
		else
		{
		   return NULL;
		}
	}
	   
	/// 获得队首的数据
	U16BIT * get_front_data(U16BIT index)
	{
		it = map_buf.find(index);
		if(it != map_buf.end())
		{
		   AutoLock l(m_lock);
		   U16BIT *d = (it->second.size() > 0) ? (it->second.front().second) : NULL;
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
	void clear(U16BIT index)
	{
		it = map_buf.find(index);
		if(it != map_buf.end())
		{
		   AutoLock l(m_lock);
		   while (it->second.size() > 0)
		   {
		          delete [] (it->second.front().second);
			   it->second.pop();
		   }
		}
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


// 
// 子地址的数据源,这里提供子地址的数据内容
// 1.文件数据源
// 2.消息拷贝数据源
// 3.指令数据源
class DataSource
{
public:
	DataSource();

	DataSource(S16BIT addr,S16BIT subAddr,S16BIT cardNum);

	~DataSource();

	// 初始化
	void init();

	// 清理
	void clear(); 

private:

	// 拷贝消息数据源
	void CopyData_msg(U16BIT * pData,U16BIT copyLen);

	// 拷贝指令数据源
	void CopyData_ins(U16BIT * pData,U16BIT copyLen);

	// 拷贝文件数据源
	void CopyData_file(U16BIT * pData,U16BIT copyLen);

public:
	//打开文件
	void OpenSimFile(S16BIT addr,S16BIT subAddr,S16BIT cardNum);

	// 拷贝指定长度的仿真数据
	void CopyData(U16BIT * pData,U16BIT copyLen);
	
       void CopyData(S16BIT subAddr, U16BIT * pData,U16BIT copyLen);
	// 拷贝数据源
	void SetSource(const MSGSTRUCT & msg);

	// 拷贝数据源
	void SetSource(const char * pData,int len);

	// 拷贝数据源
	void SetSource(S16BIT subAddr, const char * pData,int len);

	// 设置时间数据源
	void SetTimeSource();

	// 清除消息拷贝标记,指令数据源标记
	void EnSimSource();
	//判断写入的buff是否为空
       bool WriteBuffIsEmpty(S16BIT subAddr);
	// 本地时间校时
	//void ShipTimeCheck(char * pData,int dataLen);

	// 设置三取二的数据源
	void SetThreeSource(U16BIT * pData,S16BIT dataLen);
private:
	// 加入时间码,这里不针对大于32个字的情况，默认在缓存的首两个字
	void GetTimeData(U16BIT timeData[]);

	// 加入校验值
	void GetCheckValue(U16BIT paramData[],S16BIT len);

	// 得到GIST船上时
	void GetTimeData_GIST(U16BIT timeData[]);

public:
	// 设置校验
	// cheType : 校验的类型 
	// cheIndex : 校验值的位置
	void SetCheckout(S16BIT cheType,S16BIT cheIndex);

	// 设置配置的文件数据源
	void SetFileResource(const _fileConfig & fileStr);

	// 得到配置的检验信息
	void GetCheckout(__out S16BIT & cheType,__out S16BIT & cheIndex);

	// 得到配置的文件源信息
	void GetFileResource(__out _fileConfig & RecourceFile);

	// 设置数据源的时间码
	void SetTimeCode(_TimeCode & timeC);

	// 得到时间码
	void GetTimeCode(_TimeCode & timeC);

	// 大小端颠倒
	void BigWord(U16BIT * pData,U16BIT wordLen);

	// 大小端标记变量
	bool m_bBigWord;

	// 设置大小端问题
	void SetBigWord(bool bBigWord);

	// 设置三取二结构
	void SetThreeStu(_three_struct & threeS);

	// 得到三取二结构
	void GetThreeStu(_three_struct & threeS);

	// 得到数据字长度
	S16BIT GetSubLen();

	// 设置数据字长度
	void SetSubLen(S16BIT saDataLen);

	// 时间码
	_TimeCode m_TimeCode;

	// 设置是否进入故障模式,或者进入正常模式
	// 故障模式,各种配置都不起作用,即copyData函数不拷贝任何东西
	void SetWorkMode(CHAR val);

public:

	// 设置板卡号及启动时间
	void SetDevNum(S16BIT devNum);
	// 设置板卡号及启动时间
	//void SetStartTime();
	void SetTimeObj(time_Inter * timeInter){
		m_TimeInter = timeInter;
	};

private:

	// 计数器
	int m_iCount;

	// 时间格式解析的几个函数
	time_Inter * m_TimeInter;

	// 三取二数据源
	ThreeCatch  m_ThreeObj;

	S16BIT m_devNum; // 设备编号,从板卡获取时间要用
	//GIST::BrTime m_startTime; // 从板卡读取的开始实验时间  //zhanghao close.
	
	// 设置GIST船上时
	//void TestStart_GIST();

public:

	// 校验和
	_CheckOutStu m_Checkout;

	// 小包的默认值
	S16BIT m_defWord;

	// 小包类
	SimlBag m_bagBag;

	// 文件名称
	string m_strFileName;

	// 有小包
	bool m_bSmilBag;

	// 文件默认值
	bool m_bDefFile; 

	// 子地址数据长度
	S16BIT m_saDataLen; 

private:

	// 小包启用标记
	bool m_bBagStart;

	// 是否故障模式
	bool m_bErrorMode;

	// 从本地小包缓存读数据
	void CopyDataByBag(U16BIT * pData,S16BIT len_Sub,S16BIT offset);

private:
	// 仿真文件类
	SimFile * m_pFileObj;

	// 小包的数据
	U16BIT m_BagData[MAX_LEN_COPYMSG];

	// 拷贝数据
	S16BIT m_CopyResource_msg[32];

	// 拷贝数据
	S16BIT m_CopyResource_ins[MAX_LEN_COPYMSG];

	// 消息拷贝
	bool m_IsCopy_msg;
	_CopyMsg_type  m_CopMsgType;
	S16BIT m_msgLen;

	// 指令数据源
	bool m_IsCopy_ins;
	bool m_IsCopy_ins_more ; // 指令是否大于32个字，即需要多次发送
	S16BIT m_insLen;
	S16BIT m_offset;	// 指令数据源数据源的偏移

	WriteDataBuffer my_write_data_buf;	///< 用于缓存write进来的数据
	size_t r_offset;
public:
	// 默认值的缓存
	U16BIT m_defDataArry[32];   //32个字的缓存
};



