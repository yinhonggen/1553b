// commu_RT.cpp : 定义 DLL 应用程序的导出函数。
//

#include "RT_DataSource.h"

#include "utility.h"
using namespace RT;

// CRC码表
static WORD CRCTable[256] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
0xff9f, 0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
0xd94c,	0xc96d,	0xf90e,	0xe92f, 0x99c8,	0x89e9,	0xb98a,	0xa9ab,
0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};


#define	CRC16_INIT	0x0000

#ifndef BYTE 
#define BYTE unsigned char 
#endif
WORD CalCRC16(BYTE *pbDataBuf, DWORD dwNumOfBytes, const WORD *pwCRCTable)
{
	BYTE	bData = 0;
	WORD	wCRC = CRC16_INIT;	// CRC校验码

	while ( 0 != (dwNumOfBytes--) )
	{
		bData = (BYTE)(wCRC >> 8);				// 以8位二进制数的形式暂存wCRC的高8位
		wCRC <<= 8;								// 左移8位，相当于wCRC的低8位乘以2的8次方
		wCRC = wCRC ^ pwCRCTable[bData ^ (*pbDataBuf)];	// 高8位和当前字节相加后再查表求wCRC，再加上以前的wCRC
		pbDataBuf++;
	}	
	return	wCRC;
}
// CRC校验
S16BIT CheckoutCRC(U16BIT * pData,S16BIT len)
{
	return CalCRC16((BYTE*)pData,len * sizeof(S16BIT),CRCTable);
}

// 累加和校验
S16BIT CheckoutADD(U16BIT * pDAta,S16BIT len)
{
	S16BIT sum = 0;
	for (int i=0;i<len;i++)
	{
		sum += *(pDAta + i);
	}
	return sum;
}

void DataSource::SetFileResource(const _fileConfig & fileStr)
{
	m_strFileName = fileStr.fileName;
	// 添加小包的配置
	if (fileStr.bAffect)
	{	
		// 小包有效
		m_bBagStart = true;
		m_bSmilBag = fileStr.bAffect;
		m_defWord = fileStr.defWord;
		m_bagBag = fileStr.bagBag;
	}
	else
	{
		m_bSmilBag = false;
	}
}
// 得到配置的检验信息
void DataSource::GetCheckout(__out S16BIT & cheType,__out S16BIT & cheIndex)
{
	cheType = m_Checkout.type;
	cheIndex = m_Checkout.wordIndex;
}

// 得到配置的文件源信息
void DataSource::GetFileResource(__out _fileConfig & RecourceFile)
{
	RecourceFile.fileName = m_strFileName;
	if (m_bSmilBag)
	{
		RecourceFile.bAffect = true;
		RecourceFile.defWord = m_defWord;
		RecourceFile.bagBag = m_bagBag;
	}
	else
	{
		RecourceFile.bAffect = false;
	}
}

DataSource::DataSource(S16BIT addr,S16BIT subAddr,S16BIT cardNum)
{
	m_pFileObj = new SimFile();
	this->OpenSimFile(addr,subAddr,cardNum);

	init();
	return ;
}

DataSource::DataSource()
{
	m_pFileObj = new SimFile();

	init();
	return;
}

void DataSource::init()
{
	memset((void *)m_CopyResource_msg,0,sizeof(S16BIT) * 32);
	memset((void *)m_CopyResource_ins,0,MAX_LEN_COPYMSG);
	memset((void *)m_BagData,0,MAX_LEN_COPYMSG);
	memset((void *)m_defDataArry,0,sizeof(U16BIT) * 32);

	m_iCount = 0;

	EnSimSource();
	// 默认没有
	m_Checkout.type = _ERROR;
	m_TimeCode.codeType = TIME_CODE_error;
	m_bBagStart = false;
	m_bSmilBag = false;

	m_bDefFile = false;

	m_insLen = 0;

	m_TimeInter = NULL;

	m_bErrorMode = false;
	m_saDataLen =32;

	m_bBigWord = false;

	m_devNum = 0;
}


DataSource::~DataSource()
{
	clear();
	return ;
}

void DataSource::clear()
{
	if (m_pFileObj != NULL)
	{
		delete m_pFileObj;
		m_pFileObj = NULL;
	}

	if (m_TimeInter != NULL)
	{
		delete m_TimeInter;
		m_TimeInter = NULL;
	}
	return;
}

void DataSource::SetCheckout(S16BIT cheType,S16BIT cheIndex)
{
	// 默认没有
	if (cheIndex > 0)
	{
		m_Checkout.wordIndex = cheIndex;
		if (1 == cheType )
		{
			m_Checkout.type = CRC;
		}
		else
		{
			m_Checkout.type = ADD;
		}
	}
	else
	{
		m_Checkout.type = _ERROR;
	}
	return ;
}




S16BIT DataSource::GetSubLen()
{
	return m_saDataLen;
}


void DataSource::SetSubLen(S16BIT saDataLen)
{
	m_saDataLen = saDataLen;
}


// 设置是否进入故障模式,或者进入正常模式
// 故障模式,各种配置都不起作用,即copyData函数不拷贝任何东西
void DataSource::SetWorkMode(CHAR val)
{
	if (1 == val)
	{
		m_bErrorMode= true;
	}

	if(0 == val)
	{
		m_bErrorMode = false;
	}
}

/* zhanghao close.
void DataSource::ShipTimeCheck(char * pData,int dataLen)
{
	if (TIME_CODE_check == m_TimeCode.codeType || TIME_CODE_And == m_TimeCode.codeType)
	{
		// 本地校时
		GIST::BrTime brTime;

		if (NULL != m_TimeInter )
		{
			// 转为时间格式
			if (m_TimeInter->ConvertTime_Encode(pData,dataLen,brTime))
			{
				// 同步本地时间
				m_SysServ->on_time_change(brTime);
			}
		}
	}
}
*/

// 设置三取二的数据源
void DataSource::SetThreeSource(U16BIT * pData,S16BIT dataLen)
{
	EnSimSource();

	m_IsCopy_msg = true;
	m_CopMsgType = _copyMsg_three;
	m_msgLen = dataLen;

	if(!m_ThreeObj.SetThreeSource(pData,dataLen))
	{
		// 三取二不成功copy默认值。
		m_ThreeObj.SetDefVal(m_defDataArry,32);
	}
}

// 设置数据源的时间码
void DataSource::SetTimeCode(_TimeCode & timeC)
{
	m_TimeCode.codeType = timeC.codeType;
	m_TimeCode.rout_addr = timeC.rout_addr;
	m_TimeCode.rout_subAddr = timeC.rout_subAddr;
	m_TimeCode.rout_des_addr = timeC.rout_des_addr;
	m_TimeCode.rout_des_subAddr= timeC.rout_des_subAddr;
	return ;
}

// 得到时间码
void DataSource::GetTimeCode(_TimeCode & timeC)
{
	timeC.codeType = m_TimeCode.codeType;
	timeC.rout_addr = m_TimeCode.rout_addr;
	timeC.rout_subAddr = m_TimeCode.rout_subAddr ;
	timeC.rout_des_addr = m_TimeCode.rout_des_addr;
	timeC.rout_des_subAddr = m_TimeCode.rout_des_subAddr;
	return ;
}

// 设置大小端问题
void DataSource::SetBigWord(bool bBigWord)
{
	// 为1时大端，否则使用默认
	m_bBigWord = bBigWord;
}


// 设置三取二结构
void DataSource::SetThreeStu(_three_struct & threeS)
{
	m_ThreeObj.m_threeStu.eType = threeS.eType;
	m_ThreeObj.m_threeStu.msg = threeS.msg;
	m_ThreeObj.m_threeStu.retArry = threeS.retArry;
	m_ThreeObj.m_threeStu.orStu = threeS.orStu;
	m_ThreeObj.m_threeStu.num = threeS.num;
	return;
}

// 得到三取二结构
void DataSource::GetThreeStu(_three_struct & threeS)
{
	threeS.eType =m_ThreeObj.m_threeStu.eType;
	threeS.msg = m_ThreeObj.m_threeStu.msg;
	threeS.retArry = m_ThreeObj.m_threeStu.retArry;
	threeS.orStu = m_ThreeObj.m_threeStu.orStu;
	threeS.num = m_ThreeObj.m_threeStu.num;
	return;
}

void DataSource::OpenSimFile(S16BIT addr,S16BIT subAddr,S16BIT cardNum)
{
	if (NULL == m_pFileObj )
	{
		printf("\t[commu_RT]: 数据源class出错！\n");
		return ;
	}

	if (m_strFileName.length() > 0)
	{
		string fileName = "agent.rc/RT_simFile/" + m_strFileName;
              

		if (m_pFileObj->open(fileName) == false)
		{
#ifdef _DEBUG
			printf("[commu_RT]: RT error: open file(%s) fail!! \n",fileName.c_str());
#endif	
		}
		else
		{
			m_pFileObj->LoadFileData();
                printf("Open %s OK!\n",fileName.c_str());
		}
		return ;
	}
}
// 设置板卡号及启动时间
void DataSource::SetDevNum(S16BIT devNum)
{
	m_devNum = devNum;
}

/* zhanghao close.
// 设置板卡号及启动时间
void DataSource::SetStartTime()
{
#ifdef WIN32
	TestStart_GIST();
	return;
#endif
	GIST::BrTime shipTime;
	U64BIT llTTValue = 0;
	S16BIT nResult = aceGetTimeTagValueEx (m_devNum, &llTTValue);

	if(ACE_ERR_SUCCESS == nResult)
	{
		shipTime = llTTValue * 20;// 调试发现板卡的时间精度是2微妙，而我们的时间精度是100纳秒，所以转换。
	}
	else if(ACE_ERR_NOT_SUPPORTED == nResult)
	{
		GIST::BrTimeTriple time_var;				
		m_SysServ->get_system_time(time_var);		// 得到时间	
		shipTime = time_var.tm_ship_time;
		printf("aceGetTimeTagValueEx,ACE_ERR_NOT_SUPPORTED.\n");
	}
	else if(ACE_ERR_INVALID_DEVNUM == nResult)
	{
		GIST::BrTimeTriple time_var;				
		m_SysServ->get_system_time(time_var);		// 得到时间	
		shipTime = time_var.tm_ship_time;
		printf("aceGetTimeTagValueEx,ACE_ERR_INVALID_DEVNUM,%d.\n",m_devNum);
	}
	else
	{
		GIST::BrTimeTriple time_var;				
		m_SysServ->get_system_time(time_var);		// 得到时间	
		shipTime = time_var.tm_ship_time;
	}

	m_startTime = shipTime;
}


// 设置板卡号及启动时间
void DataSource::TestStart_GIST()
{
	if (NULL == m_SysServ)
	{
		return ;
	}
	GIST::BrTimeTriple time_var;				
	m_SysServ->get_system_time(time_var);		// 得到时间	
	m_startTime = time_var.tm_ship_time;
}


// 加入时间码,这里不针对大于32个字的情况，默认在缓存的首两个字
void DataSource::GetTimeData_GIST(U16BIT timeData[])
{
	S16BIT offset = 0;// 假设偏移为0
	if (NULL == m_SysServ)
	{
		return ;
	}
	GIST::BrTimeTriple time_var;				
	m_SysServ->get_system_time(time_var);		// 得到时间	
	GIST::BrTime shipTime = time_var.tm_ship_time;
		
	if( TIME_CODE_send == m_TimeCode.codeType || TIME_CODE_And == m_TimeCode.codeType)
	{	
		// 时间码
		if (NULL != m_TimeInter )
		{
			m_TimeInter->ConvertTime_Code((char *)(timeData+offset),2 * sizeof(U16BIT),shipTime);		
		}
		return ;
	}	
}
*/

// 加入时间码,这里不针对大于32个字的情况，默认在缓存的首两个字
void DataSource::GetTimeData(U16BIT timeData[])
{
	/* zhanghao close.
#ifdef WIN32
	GetTimeData_GIST(timeData);
	return ;
#endif
	S16BIT offset = 0;// 假设偏移为0
	if (NULL == m_SysServ)
	{
		return ;
	}
	// 	ACE中获取时间的函数 aceGetTimeTagValueEx
	//	retrieves the current value from the card’s 48-bit time tag register.

	// To get the time tag value.
	// 从板卡获取时间
	// 发送时间
	GIST::BrTime shipTime;
	S16BIT nResult = 0;
	U64BIT llTTValue = 0;
	nResult = aceGetTimeTagValueEx (m_devNum, &llTTValue);

	if(ACE_ERR_SUCCESS == nResult)
	{
		shipTime = llTTValue * 20;// 调试发现板卡的时间精度是2微妙，而我们的时间精度是100纳秒，所以转换。

		shipTime = shipTime - m_startTime;	// 得到船上时
	}
	else if(ACE_ERR_NOT_SUPPORTED == nResult)
	{
		GIST::BrTimeTriple time_var;				
		m_SysServ->get_system_time(time_var);		// 得到时间	
		shipTime = time_var.tm_ship_time;
		//printf("aceGetTimeTagValueEx,ACE_ERR_NOT_SUPPORTED.\n");
	}
	else if(ACE_ERR_INVALID_DEVNUM == nResult)
	{
		GIST::BrTimeTriple time_var;				
		m_SysServ->get_system_time(time_var);		// 得到时间	
		shipTime = time_var.tm_ship_time;
		//printf("aceGetTimeTagValueEx,ACE_ERR_INVALID_DEVNUM,%d.\n",m_devNum);
	}
	else
	{
		GIST::BrTimeTriple time_var;				
		m_SysServ->get_system_time(time_var);		// 得到时间	
		shipTime = time_var.tm_ship_time;
	}

	if( TIME_CODE_send == m_TimeCode.codeType || TIME_CODE_And == m_TimeCode.codeType)
	{	
		// 时间码
		if (NULL != m_TimeInter )
		{
			m_TimeInter->ConvertTime_Code((char *)(timeData+offset),2 * sizeof(U16BIT),shipTime);		
		}
		return ;
	}
	*/
}

// 加入校验值,这里不针对大于32个字的情况
void DataSource::GetCheckValue(U16BIT paramData[],S16BIT len)
{
	if (len > 32 )
	{
		return;
	}
	int index = (int)m_Checkout.type;
	FunCheckout chakeFun = g_FunArr[index];
	if (chakeFun != NULL ) 
	{
		S16BIT cheValue = chakeFun(paramData,len);
		if (m_Checkout.wordIndex <= 32)
		{
			*(paramData + m_Checkout.wordIndex-1) = cheValue;
		}
		else
		{
			printf("\t[commu_RT]:校验位置超出区间:%d\n",m_Checkout.wordIndex);
		}
	}
}



// 拷贝消息数据源
void DataSource::CopyData_msg(U16BIT * pData,U16BIT copyLen)
{
	copyLen = m_msgLen ;
	int readLen = sizeof(S16BIT) * copyLen;
	switch (m_CopMsgType)
	{
	case _copyMsg_normal:
		{
			memcpy((void *)pData,(void *)m_CopyResource_msg,readLen);
		}
		break;
	case _copyMsg_three: // 三取二
		{
			copyLen = m_ThreeObj.CopyData(pData);
		}
		break;
	case _copyMsg_More:
		break;
	default:
		break;
	}

	// 时间
	GetTimeData(pData);
	// 加入校验
	GetCheckValue(pData,copyLen);
}

// 拷贝指令数据源
void DataSource::CopyData_ins(U16BIT * pData,U16BIT copyLen)
{
	if (!m_IsCopy_ins_more)
	{
		copyLen = m_insLen; 
		int readLen = sizeof(S16BIT) * copyLen;
		memcpy((void *)pData,(void *)(m_CopyResource_ins),readLen);
		m_offset = 0;

		if(!m_bErrorMode) // 非故障模式
		{
			// 时间
			GetTimeData(pData);
			// 加入校验
			GetCheckValue(pData,copyLen);
		}
	}
	else
	{
		int len_max = m_offset + copyLen*sizeof(S16BIT);
		if ( len_max > MAX_LEN_COPYMSG )
		{
			printf("\t[commu_RT error]:  CopyData MAX_LEN_COPYMSG error!  \n");
			return ;
		}
		int readLen = sizeof(S16BIT) * copyLen;
		memcpy((void *)pData,(void *)(m_CopyResource_ins + m_offset),readLen);
		m_offset += copyLen;

		// 长指令发送完成
		if (m_offset >= m_insLen)
		{
			m_offset =0;
		}
	}	
}


// 拷贝文件数据源
void DataSource::CopyData_file(U16BIT * pData,U16BIT copyLen)
{
	if (!m_bSmilBag)// 文件数据源.(这里没有小包)
	{
		int readLen = sizeof(S16BIT) * copyLen;
		int readLen_file = m_pFileObj->fread_buff((char *)pData,readLen);
		if (readLen_file <= 0)
		{
			m_pFileObj->fseek_buff(0);
			m_pFileObj->fread_buff((char *)pData,readLen);	
		}

		// 时间码
		GetTimeData(pData);

		// 校验判断
		GetCheckValue(pData,copyLen);
	}
	else// 文件数据源,有小包 
	{			
		if (m_bBagStart) // 开始的初始化
		{
			// 更新本地小包缓存 
			int readLen = sizeof(S16BIT) * m_bagBag.GetLen();
			int readLen_file = m_pFileObj->fread_buff((char *)m_BagData,readLen);
			if (readLen_file <= 0)
			{
				m_pFileObj->fseek_buff(0);
				//int sub_readLen = m_pFileObj->fread_buff((char *)m_BagData,readLen);	
			}
			m_bBagStart = false;
		}

		int len_Sub = m_bagBag.GetSubLen();
		if (len_Sub > 0)
		{
			// 赋值
			CopyDataByBag(pData,len_Sub,m_bagBag.GetOffsetLen());

			// 校验判断
			GetCheckValue(pData,copyLen);
		}

		if (m_bagBag.IsEnd())
		{
			// 更新本地小包缓存 
			int readLen = sizeof(S16BIT) * m_bagBag.GetLen();
			int readLen_file = m_pFileObj->fread_buff((char *)m_BagData,readLen);
			if (readLen_file <= 0)
			{
				m_pFileObj->fseek_buff(0);
			}
		}
	}
}

// 大小端颠倒
void DataSource::BigWord(U16BIT * pData,U16BIT wordLen)
{
	if (wordLen>32)
	{
		wordLen = 32;
	}
	for (int i=0;i<wordLen;i++)
	{
		char * pTempData= (char *)(pData+i);
		char a= (*pTempData);
		char b= *(pTempData+1);
		*pTempData = b;
		*(pTempData+1) = a;
	}
}
/*******************************************
* 拷贝数据，文件或者固定值
* pData,
* copyLen_s16bit
********************************************/
void DataSource::CopyData(U16BIT * pData,U16BIT copyLen)
{

   //printf("m_bErrorMode:%d  m_IsCopy_ins:%d  m_IsCopy_msg:%d\n",m_bErrorMode,m_IsCopy_ins,m_IsCopy_msg);
	if (m_bErrorMode && m_IsCopy_ins)
	{ 
		// 故障模式,可以执行指令数据源,但不需要校验和时间码
		// 指令数据源
		if (m_IsCopy_ins)
		{
			CopyData_ins(pData,copyLen);

			// 是否大小端颠倒
			if (m_bBigWord)
			{
				BigWord(pData,32);
			}
			return;
		}
		return;
	}

	if ( copyLen > MAX_LEN_COPYMSG || copyLen >32)
	{
		return;
	}

	// 消息拷贝数据源
	if(m_IsCopy_msg)
	{
		CopyData_msg(pData,copyLen);
		// 是否大小端颠倒
		if (m_bBigWord)
		{
			BigWord(pData,32);
		}

		return;
	}
	// 指令数据源
	if (m_IsCopy_ins)
	{
		CopyData_ins(pData,copyLen);
		// 是否大小端颠倒
		if (m_bBigWord)
		{
			BigWord(pData,32);
		}

		return;
	}


	// 文件数据源
	if (m_pFileObj->IsOpen())
	{
		CopyData_file(pData,copyLen);
		// 是否大小端颠倒
		if (m_bBigWord)
		{
			BigWord(pData,32);
		}
		return ;
	}

	// 其他配置都没有时，copy默认值。
	memcpy((void *)pData,(void *)m_defDataArry,sizeof(U16BIT)*32);

	// 时间码
	GetTimeData(pData);

	// 校验判断
	GetCheckValue(pData,copyLen);

	// 是否大小端颠倒
	if (m_bBigWord)
	{
		BigWord(pData,32);
	}

	return ;
}

void DataSource::CopyData(S16BIT subAddr, U16BIT * pData,U16BIT copyLen)
{
		if(my_write_data_buf.empty(subAddr))
		{
            return;
		}
		
		size_t *save_len = my_write_data_buf.get_front_header(subAddr);
		if(NULL == save_len)
		{
				printf("the save len is NULL\n");
				return;
		}
		U16BIT *m_Copy_data = my_write_data_buf.get_front_data(subAddr);
		if(NULL == m_Copy_data)
		{
			  printf("the m_Copy_data is NULL\n");
			  return;
		}
		
		if((r_offset + copyLen) >*save_len/2)
		{
		      
			  int readLen = sizeof(S16BIT) * (*save_len/2 - r_offset);
			  memcpy((void *)pData, (m_Copy_data + r_offset), readLen);
			  my_write_data_buf.pop(subAddr);
			  m_Copy_data = my_write_data_buf.get_front_data(subAddr);
			  if(NULL == m_Copy_data)
		     {
			     printf("the m_Copy_data is NULL\n");
			     return;
		     }
			  size_t *save_len = my_write_data_buf.get_front_header(subAddr);
			  if(NULL == save_len)
			  {
						printf("the save len is NULL\n");
						return;
			   }	
				
			  size_t tmp_len = readLen/sizeof(S16BIT);	
			  if(NULL != m_Copy_data)
			  {
			       if(*save_len/2 <= (copyLen - tmp_len))
			       {
						 memcpy((void *)(pData + tmp_len) , m_Copy_data, *save_len);
						 my_write_data_buf.pop(subAddr);
						 r_offset = 0;
					}
					else
					{
					    r_offset = copyLen - tmp_len;
					    memcpy((void *)(pData + tmp_len) , m_Copy_data, r_offset*sizeof(S16BIT));
					}
			  }
			  else
			  {
					memset((void *)(pData + tmp_len), 0, (copyLen - tmp_len)*sizeof(S16BIT));
					r_offset = 0;
			  }  
		}
		else if((r_offset + copyLen) < *save_len/2)
		{
           int readLen = sizeof(S16BIT) * copyLen;
           memcpy((void *)pData, (m_Copy_data + r_offset), readLen);
			 r_offset += copyLen;
		}
		else if((r_offset + copyLen) == *save_len/2)
		{
           int readLen = sizeof(S16BIT) * copyLen;
           memcpy((void *)pData , (m_Copy_data + r_offset), readLen);
			 r_offset = 0;
			 my_write_data_buf.pop(subAddr);
		}

		return;
}

bool DataSource::WriteBuffIsEmpty(S16BIT subAddr)
{
		return my_write_data_buf.empty(subAddr);
}

void DataSource::CopyDataByBag(U16BIT * pData,S16BIT len_Sub,S16BIT offset)
{
	if (pData == NULL || len_Sub <= 0)
	{
		return ;
	}
	if(len_Sub > 0 && len_Sub <= 32)
	{
		// 赋默认值 
		for (int iSet = 0 ;iSet< 32 ;iSet++)
		{
			*(pData+iSet) = m_defWord; 
		}
		// 拷贝数据
		memcpy((void *)pData,(void *)(m_BagData+ offset),len_Sub * sizeof(S16BIT));
	}

/*	FunCheckout chakeFun = g_FunArr[m_Checkout.type];
	if (chakeFun != NULL && m_bagBag.IsEnd()) // 有校验
	{
		// 计算是否到校验
		S16BIT cheValue = chakeFun(m_BagData,m_bagBag.GetLen());
		int iIndex = m_Checkout.wordIndex%32;
		iIndex =(0 ==iIndex?32:iIndex);
		if (iIndex >= 0)
		{
			*(pData + (iIndex-1)) = cheValue;
		}
	}
*/
}

void DataSource::SetSource(const MSGSTRUCT & msg)
{
	EnSimSource();
	m_IsCopy_msg = true;
	m_CopMsgType = _copyMsg_normal;

	int iCount = (int)msg.wWordCount;
	memcpy((void *)m_CopyResource_msg,(void *)msg.aDataWrds,sizeof(S16BIT) * (iCount>32?32:iCount));
	m_msgLen = iCount;
	return ;
}

void DataSource::SetSource(const char * pData,int len)
{
	EnSimSource();
	m_IsCopy_ins = true;
	memset((void *)m_CopyResource_ins,0,MAX_LEN_COPYMSG);

	if (len > MAX_LEN_COPYMSG)
	{
		memcpy((void *)m_CopyResource_ins,(void *)pData,MAX_LEN_COPYMSG);
	}
	else if( (len /sizeof(U16BIT)) > 32)
	{
		m_IsCopy_ins_more = true;
		memcpy((void *)m_CopyResource_ins,(void *)pData, len);
	}
	else
	{
		m_IsCopy_ins_more = false;
		memcpy((void *)m_CopyResource_ins,(void *)pData, len);
	}

	m_insLen =  len%sizeof(U16BIT) == 0 ? len/sizeof(U16BIT) : (len/sizeof(U16BIT) +1);
	m_offset = 0;
	return ;
}

void DataSource::SetSource(S16BIT subAddr, const char * pData,int len)
{
	 my_write_data_buf.push(subAddr, len, pData);
	return ;
}

void DataSource::EnSimSource()
{
	m_IsCopy_ins_more = false;
	m_IsCopy_msg = false;
	m_IsCopy_ins = false;
	m_bSmilBag = false;
}
