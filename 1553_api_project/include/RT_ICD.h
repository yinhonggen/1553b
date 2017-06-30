#pragma once

#include "zynq_1553b_api.h"
#include "RT_DataSource.h"

//#include "../../../../../include/gist/icd/icdman.h"
//#include "../../../../../include/gist/icd/BrBasicParser.h"

class RT_ICD
{
public:
	RT_ICD(void);
	~RT_ICD(void);
};

// 实际得到时间的几个函数
class timeInter_obj:public virtual time_Inter
{
public:

	timeInter_obj();
	virtual ~timeInter_obj();
public:
	// 时间码转换,编码,将time编码到pData中
	//virtual int ConvertTime_Code(char * pData,int len,const GIST::BrTime & time);  zhanghao close.

	// 时间码转换,解码,从pData中解析出time
	//virtual	int ConvertTime_Encode(const char * pData,int len,GIST::BrTime & time);  zhanghao close.

private:
	//BrBasicParser* parser;

	//Block* block;

	// ICDMan对象
	//const ICD::ICDMan * m_IcdMan;

	// 接口指针
	//const Interface* m_interface;  

	U16BIT m_timeData[32];

public:
	// 设置ICDMan和系统服务
	//void SetIcdManAndServ(const ICD::ICDMan * icd,Agent::ISysServ * sysServ,const Interface * pInterface);

	// 设置接口指针
	//void SetTimeCode(_TimeCode & m_TimeCode);
};

