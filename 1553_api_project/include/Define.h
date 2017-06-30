#pragma once

#include "zynq_1553b_api.h"

#ifdef WIN32_DDC
#include "../../1553B_BC/Include/stdemace.h"
#pragma warning( disable : 4819 )
#elif defined(IPCORE)
//#include "../../../../FrameWork/vxWorks/1553B_IPCore_driver/IPCORE.h"
#include "IPCORE.h"
#else
//#include "../../1553B_BC/Include_vx/stdemace.h"
#endif
#include "types.h"


#ifndef  WIN32
#define __out
#define __in
#endif
#include <string>
#include <vector>
#include <map>
#include <list>
using namespace std;

// 拷贝消息的最大缓存
const int MAX_LEN_COPYMSG  = 10 * 1024;

// 子地址数据周期变化结构                                                                     
typedef struct _CycleIndex
{
	long cycle;
	int cyleIndex;
	S16BIT updataLen; //　更新的长度
	_CycleIndex()
	{
		cyleIndex = 0;
		cycle=0;
		updataLen=0;
	}
}_CycleIndex;

// 时间码类型
typedef enum enum_TimeCode{
	TIME_CODE_error= 0,
	TIME_CODE_send = 1,
	TIME_CODE_check = 2,
	TIME_CODE_And = 3 ,  // 又收又发
}enum_TimeCode;

// 时间码结构
typedef struct _TimeCode
{
	enum_TimeCode  codeType; // 时间码类型

	string rout_addr;		 // 路由源地址
	string rout_subAddr;     // 路由源子地址
	string rout_des_addr;	 // 路由目的地址
	string rout_des_subAddr; // 路由目的子地址

	_TimeCode()
	{
		codeType = TIME_CODE_error;
	}
}_TimeCode;

// 校验方式
typedef enum {
	_ERROR = 0,
	CRC =1,	 // CRC校验
	ADD = 2, // 累加和
} enum_Check;		// 校验方式

// 校验结构
typedef struct _CheckOutStu
{
	enum_Check type;		// 校验的方式
	S16BIT wordIndex;   // 校验和的位置,在包中的总位置
}_CheckOutStu;

// 消息拷贝的三种形式
typedef enum _CopyMsg_type
{
	_copyMsg_normal ,  // 普通消息拷贝
	_copyMsg_three,	   // 拷贝内容三取二计算的
	_copyMsg_More      // 拷贝多条的
}_CopyMsg_type;

// 消息数据源中的三取二算法

// 三取二比对结果
// 最多是32个字
class ThreeResult
{
public:
	ThreeResult(){
		memset((void*)dataArr,0,sizeof(U16BIT)*32);
	};
	// 用字符串构造
	ThreeResult(const string & str);

	~ThreeResult(){
	};
	ThreeResult(const ThreeResult & other){
		memcpy((void *)dataArr,(void*)other.dataArr,sizeof(U16BIT)*32);
	};
	ThreeResult & operator = (const ThreeResult & other){
		memcpy((void *)dataArr,(void*)other.dataArr,sizeof(U16BIT)*32);
		return (*this);
	};

	// 返回字符串
	string getLineStr();

public:
	U16BIT dataArr[32];
};
// 三取二的类型
typedef enum _three_Type
{
	_three_error = 0,
	_three_or = 1,
	_three_INS = 2,
	_three_Array = 3,
}_three_Type;

// 按位与
typedef struct _three_Type_or 
{
	U16BIT def;
	map<U16BIT,int> mapVal;
}_three_Type_or ;

// 比对
typedef struct _three_Type_Array 
{
	bool che;
	map<U16BIT,ThreeResult> listVal; // 一个值对应另一个值
}_three_Type_Array;


/**********************************************************************
文件数据源中的小包

一个数据源文件可能需要分成若干小包，每次取一个小包发送，包的长度可能大于32

小包还需要指出每次发送的长度。
***********************************************************************/
class SimlBag 
{
public:
	SimlBag(){
		m_index = 0;
		m_SubLen_Count = 0;
		m_offset = 0;
		m_bEnd =false;
	}
private:
	int m_len_2; //小包的长度
	vector<S16BIT> m_subLen; // 拆分后每次发送的长度
	size_t m_index; // 记录变量
	// 已取走的长度
	int m_SubLen_Count;
	// 标记是否结束
	bool m_bEnd;
	// 偏移
	int m_offset;
public:
	S32BIT GetLen() const {
		return m_len_2;
	}
	void SetLen(S32BIT len)	{
		m_len_2 = len;
	}

	// 加入一个子长度
	void SetSubLen(S16BIT len) {
		m_subLen.push_back(len);
	}

	// 清理
	void clear(){
		m_subLen.clear();
	}

	// 用字符串初始化
	bool  SplitBag(string str);

	// 检查一个长度是否合法
	static bool CheckError(string & errorInfo,int bagLen);

	// 得到一个子长度 
	S16BIT GetSubLen(){
		if(m_index >= m_subLen.size())
		{
			m_index = 0;
			m_SubLen_Count = 0;
			m_offset =0;
			m_bEnd =false;
		}

		S16BIT subLen =m_subLen.at(m_index);
		m_index ++;
		//m_SubLen_Count += subLen;
		m_offset = m_SubLen_Count-subLen;
		m_bEnd =  (m_index >= m_subLen.size() )?true:false;

		return subLen;
	}

	bool IsEnd() const{
		return m_bEnd;
	}

	// 得到偏移
	S16BIT GetOffsetLen() const{
		return m_offset;
	}
public:
	void GetSublen(vector<S16BIT> & vecSublen) const{
		vecSublen = m_subLen;
	}
	void SetSubLen(const vector<S16BIT> & subLenVec){
		m_subLen = subLenVec;
	}
};


// 
// 描述一个收到的消息的结构,轻量级的对象
// 可以作为map的key值，方便查找。                                                                      
class RT_MSG
{
public:
	RT_MSG(){	
		addr = 0;
		subAddr =0;
		readFlag =0;
		modeCode =1;
		return ;
	};
	~RT_MSG()	{	
		return ;
	};
	RT_MSG(const RT_MSG & other){
		addr = other.addr;
		subAddr = other.subAddr;
		readFlag = other.readFlag;
		modeCode = other.modeCode;
	};
	RT_MSG operator =(const RT_MSG & other){
		addr = other.addr;
		subAddr = other.subAddr;
		readFlag = other.readFlag;
		modeCode = other.modeCode;
		return *this;
	};
public:
	RT_MSG(const MSGSTRUCT & msg,const S16BIT & subAddr_p,const S16BIT & readFlag,const S16BIT & msg_modecode);

	// 解析: MSG5-SA0-MODE1	// 含义: 子地址_类型_方式码
	RT_MSG(const string & strLine);

	// 构造
	// subAddr:
	// msg_type:
	// msg_modecode:
	RT_MSG(const S16BIT & subAddr,const S16BIT & readFlag,const S16BIT & msg_modecode);

	// 返回: MSG5-SA0-MODE1	// 含义: 子地址_类型_方式码
	string getLine() const;

	// 得到类似如下的消息描述
	// RT6-SA15-13 -> BC
	string GetDesLine();

	//比较
	bool CompareMsgAndCopy(const S16BIT & subAddr_p,const S16BIT & msg_type,const S16BIT & msg_modecode);

	// 比较操作符号重载
	bool operator == (const RT_MSG & other);

	bool operator < (const RT_MSG & other) const;
public:
	S16BIT addr; // 地址，0和31是广播
	// 子地址
	S16BIT subAddr;
	// 读写位
	S16BIT readFlag; // 0为读，1为写
	// 数据字长度/方式命令码
	S16BIT modeCode;
};
