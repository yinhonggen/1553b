/**
* common_bc.cpp
* 通用1553BC
*
*    \date 2012/9/1
*  \author luxiaoqing
*/

#include "zynq_1553b_api.h"

#ifdef WIN32
//#include "../dll1553BC/stdafx.h"
#endif
#include "common_bc.h"
#ifdef IPCORE
#include "IPCORE.h"
#endif
#ifdef ALL_MODE
#include "ISR.h"
#endif
#ifdef __vxworks
#include "sys/times.h" 
#include "logLib.h"
#endif
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef WIN32
#include <io.h>
#endif

#if  (!defined WIN32)||(defined __linux__)
#define logMsg(a,b,c,d,e,f,g) printf(a,b,c,d,e,f,g);
#endif

/// 静态初始化
S16BIT BC_Ops::s_op_id = 400;

#define MAX_TYPE   21
typedef std::string::size_type size_type;

/// 大帧id
const int BrAda1553B_BC_COMM::MAJOR_FRAME_ID = 32767;
BOOL BC_Adapter_changeEndian;
BrAda1553B_BC_COMM* p_AdaBC = NULL;

/// 此函数主要用于处理1553B的数据。其作用就是将制定缓冲中的数据按字进行颠倒，如果缓冲的长度不是2的整数倍，则不处理尾部的数据
inline void swap_by_word(char *buf, size_t size)
{
	if (!buf || size <= 0) return;
	size_t swap_size = size /2 * 2;
	for (size_t i = 0 ; i < swap_size - 1 ; i+=2)
	{
		*(buf + i) ^= *(buf + i + 1);
		*(buf + i + 1) ^= *(buf + i);
		*(buf + i) ^= *(buf + i + 1);
	}
}

void SetTimecodeForAsync(WORD msgNO, WORD* data){
	if (!p_AdaBC || !p_AdaBC->m_sys_service || !data){
		return;
	}

	if (p_AdaBC->m_mapID.find(msgNO) != p_AdaBC->m_mapID.end()){
		WORD index = p_AdaBC->m_mapID[msgNO];
		if (p_AdaBC->m_vec_msg[index].isTimeCode)
		{
			/* zhanghao close.
			BrTimeTriple now;
			p_AdaBC->m_sys_service->get_system_time(now);
			int64_t shiptime = now.tm_ship_time / 1000;

			data[0] = (shiptime & 0xFFFFFFFF) >> 16;
			data[1] =  shiptime & 0x0000FFFF;
			swap_by_word((char*)data, 4);
			*/
		}
	} 
}

void my_strupr(char * pData,int Len)
{
	int i =0 ;
	while(i<Len)
	{
		char cTemp = *(pData+i);
		if ('a' <= cTemp && cTemp <= 'z')
		{
			*(pData+i) = (cTemp-32);
		}

		i++;
	}
}

int hex_atoi(const char s[])  
{  
	int hexdigit, i, inhex, n;  
	i = 0;  

	if(NULL == s)
	{
        return 0;
	}
	
	if(s[i] == '0')   //如果字符串以0x或0X开头，则跳过这两个字符  
	{  
		++i;  
		if(s[i] == 'x' || s[i] == 'X')    
			++i;  
	}
	else
	{
		n = atoi(s);
		return n;
	}

	n = 0;  
	inhex = 1;            //如果字符是有效16进制字符，则置inhex为YES  
	for(; inhex == 1; ++i)  
	{  
		if(s[i] >= '0' && s[i] <= '9')  
			hexdigit = s[i] - '0';  
		else if(s[i] >= 'a' && s[i] <= 'f')  
			hexdigit = s[i] - 'a' + 10;  
		else if(s[i] >= 'A' && s[i] <= 'F')  
			hexdigit = s[i] - 'A' + 10;  
		else  
			inhex = 0;  
		if(inhex == 1)  
			n = n * 16 + hexdigit;  
	}  

	return n;  
}


/// CRC码表
WORD CRCTable[256] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
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
WORD CalCRC16_BC(BYTE *pbDataBuf, DWORD dwNumOfBytes, const WORD *pwCRCTable)
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

std::map<S16BIT, BrAda1553B_BC_COMM*> BrAda1553B_BC_COMM::s_adapt;

/*           
* 构造函数
*  
**/
BC_Msg::BC_Msg():GPF(ACE_CNDTST_ALWAYS),
				m_irqCount(0),
				m_missFreq(0),
				//m_parser(NULL),
				//m_block(NULL),
				m_buff(NULL),
				m_buffSize(0),
				m_packCount(0),
				m_pbuff(NULL),
				m_CRCBuff(NULL),
				m_CRCBuffSize(0),
				m_isBrokenMode(FALSE){

}

/*           
* 析构函数
*  
**/
BC_Msg::~BC_Msg(){
	/* zhanghao close.
	if (!m_parser)
	{
		delete m_parser;
		m_parser = NULL;
	}
	*/

	if (isFromFile && isCheckCRC && CRCTiming == 1 && m_CRCBuff)
	{
		delete [] m_CRCBuff;
		m_CRCBuff = NULL;
	}
	if (isFromFile && m_buff)
	{
		delete [] m_buff;
		m_buff = NULL;
	}
}

/*           
* 初始化时间码消息所需的信息,每次实验只会在开始时调用一次
*  
**/
/* zhanghao close.
void BC_Msg::InitTimecode(const ICDMan* icdman, const Interface* ref_if){
	if (!isTimeCode || !icdman || !ref_if){
		return;
	}

	char dest_addr[8];
	char dest_subaddr[8];

	sprintf(dest_addr, "%d", Addr[2]);
	sprintf(dest_subaddr, "%d", Addr[3]);

	if ( ref_if ) {
		std::vector<ICD::Routing *> routings = const_cast<ICDMan*>(icdman)->MatchRoutes(const_cast<Interface*>(ref_if),
			"TimeCode", "", dest_addr, dest_subaddr);
		if ( routings.size() >= 1 ){
			BlockAttr * blockAttr = const_cast<ICDMan*>(icdman)->GetBlock(routings[0]);
			m_block = const_cast<ICDMan*>(icdman)->GetBlock(blockAttr);
		}
	}
	if (m_block)
	{
		m_parser = new BrBasicParser(const_cast<ICDMan*>(icdman), m_block, (char*)data, 64);
	}
}
*/

/*           
* 设置时间码参数到data中
*  
**/
/* zhanghao close.
void BC_Msg::SetTimecode(Agent::ISysServ *svr){
	if (!isTimeCode || !m_block || !m_parser || !svr){
		return;
	}

	try
	{
		BrTimeTriple now;
		svr->get_system_time(now);
		BrVariant var_shiptime;
		var_shiptime.set((int64_t)now.tm_ship_time);
		const ICDMan* icdman = svr->get_icd();
		ICDMan::MemNode::iterator itMem = icdman->GetMembers(m_block);
		for( ; itMem != ICDMan::MemNode::end(); itMem++ ){
			Field *field = itMem->GetField();
			Parameter *para = field->GetParamPtr();
			if (para->GetParamType() == "BusRobot.ICD.Parameter.Type.TimeBase"){
				(*m_parser)[para] = var_shiptime;
			}
		}
	}
	catch (std::exception& e)
	{
		printf("BC_Msg::SetTimecode:[exception]%s\n", e.what());
	}

	//方式命令只能发一个字，根据项目要求，发低16位
	if (type == MSG_MODE_CODE || type == MSG_MODE_CODE_BROADCAST)
	{
		data[0] = data[1];
	}
}
*/

/*           
* 设置从文件读出的数据到data
*  
**/
void BC_Msg::SetFileData(){
	if (!isFromFile){
		return;
	}
	m_packCount++;
//	memset(data, defaultValue, 64);
/*	for(int i=0; i<32; i++){
		data[i] = defaultValue;
	}*/ //close by yinhonggen
	if (m_buffSize == 0 || length == 0){
		return;
	}
	int tempLen = length;
	int offset = 0;  //本条消息要发的文件缓存里字数，在文件尾只取剩余的字数

	//不校验
	if(!isCheckCRC){
		//小包的最后一条消息
		if ( packLen%tempLen != 0 && m_packCount % (packLen/tempLen+1) == 0 )
		{
			offset = packLen%tempLen;
		}
		//小包的非最后一条消息
		else{
			offset = tempLen;
		}
	}
	//消息校验
	else if (isCheckCRC && CRCTiming == 0){
		tempLen = tempLen - 1;
		if (tempLen == 0){
			return;
		}
		//小包的最后一条消息
		if ( packLen%tempLen != 0 && m_packCount % (packLen/tempLen+1) == 0 )
		{
			offset = packLen%tempLen;
		}
		//小包的非最后一条消息
		else{
			offset = tempLen;
		}
	}
	//包校验
	else if(isCheckCRC && CRCTiming == 1){
		//小包的最后一条消息还有写校验码位置的情况
		if ( packLen%tempLen != 0 ){
			//小包的最后一条消息
			if ( m_packCount % (packLen/tempLen+1) == 0 )
			{
				offset = packLen%tempLen;
			}
			//小包的非最后一条消息
			else{
				offset = tempLen;
			}
		}
		else{
			//只写小包校验码的消息，最后一条
			if ( m_packCount % (packLen/tempLen + 1) == 0)
			{
				offset = 0;
			}
			//小包数据消息，非最后一条消息
			else{
				offset = tempLen;
			}
		}
	}

	memcpy(data, m_pbuff, offset*2);
	m_pbuff += offset*2;

	//如果读到文件缓存尾了，回到头
	if (m_pbuff - m_buff == m_buffSize){
		m_pbuff = m_buff;
	}
}

/*           
* 在消息的最后一个字设置校验和到data
*  
**/
void BC_Msg::SetCRC(){
	if (!isCheckCRC || length == 0){
		return;
	}

	char *pData = reinterpret_cast<char*>(data);
	DWORD len = (length - 1) * 2;

	//如果是在包尾加校验
	if (isFromFile && CRCTiming == 1)
	{
		short temp = m_packCount % (packLen/length+1);
		//不是包未，退出
		if (m_packCount % (packLen/length+1) != 0)
		{
			memcpy(m_CRCBuff + (temp-1)*length*2, data, length*2);
			return;
		}else{
			memcpy(m_CRCBuff + (packLen/length)*length*2, data, length*2);
			pData = m_CRCBuff;
//			len = m_CRCBuffSize - 2;
			len = packLen * 2;
		}
	}

	//累加和
	if (CRCMode == 0)
	{
		S16BIT sum = 0;
		for (U32BIT i=0;i<(len/2);i++)
		{
			sum += *(reinterpret_cast<U16BIT*>(pData) + i);
		}
		data[length-1] = sum;
	}
	//CRC
	else if(CRCMode == 1)
	{
		U16BIT wCRC = CalCRC16_BC((BYTE*)pData,len,CRCTable);

		data[length-1] = wCRC;
	}
}

/*           
* 设置发出数据的内容
*  
**/
void BC_Msg::SetBCData(S16BIT numCard, Agent::ISysServ *svr){
if (isFromFile)
{
	SetFileData();
}
if (isTimeCode)
{
	//SetTimecode(svr); zhanghao close.
}
if (isCheckCRC)
{
	SetCRC();
}
if ( (isFromFile || isTimeCode || isCheckCRC) && sendTiming != SENDTIMING_DUP )
{
	//修改数据块内容
	aceBCDataBlkWrite( numCard, blkid, data, 32, 0 );
#if 0
	for(WORD cc = 0 ; cc<32 ;cc++)
		{
			printf("data[%d] = 0x%x  ",cc,data[cc]);
		}
#endif
}
}

/*           
* 载入仿真文件
*  
**/
void BC_Msg::loadSimFile(){
	if (!isFromFile){
		return;
	}
	if (isCheckCRC && CRCTiming == 1)
	{
		m_CRCBuffSize = 2 * (packLen + length - packLen%length);
		m_CRCBuff = new char[m_CRCBuffSize];
		if (!m_CRCBuff){
			std::cout << "new m_CRCBuff failed.\n";
			return;
		}
	}
	char filepath[160];
	sprintf(filepath, "agent.rc/BC_SimFile/%s", filename);
	int retry_num = 3;
	while ( --retry_num >= 0 ) {
		struct stat file_stat;
		if ( (stat(filepath, &file_stat) != 0 ) ) {
			std::cout << "BC_Msg::loadSimFile(): file '" << filename << "' not found." << std::endl;
			usleep(1);
			continue;
		}
		if ( file_stat.st_size <= 0 ) {
			std::cout << "BC_Msg::loadSimFile(): file '" << filename << "': no contents." << std::endl;
			break;
		}
#ifdef WIN32
		int fd = _open(filepath, O_RDONLY|O_BINARY, 0);
#else
		int fd = open(filepath, O_RDONLY, 0);
#endif
		if ( fd == -1 ) {
			std::cerr << "open file '" << filename << "' failed: " << errno << std::endl;
			usleep(1);
			continue;
		}
		int filesize = file_stat.st_size;
		std::cout << "file[" << filename << "] size is " << filesize << " bytes." << std::endl;      //file[RT1_RT1_DAT1.DAT] size is 217 byte;
		//整理文件缓存，将最后一个包填完整
		m_buffSize = filesize;
		int temp = m_buffSize%(2*packLen);        // m_buffsize = 217 ; packLen = 67 (0x0043) ; temp = 83
		if (temp != 0){
			m_buffSize = m_buffSize + 2*packLen - temp;  // m_buffsize = 217 +67*2 - 83 = 268 bytes
		}
		m_buff = new char[m_buffSize];
		if (!m_buff){
			std::cout << "new m_buff failed.\n";
			close(fd);
			return;
		}
		//填充默认值
/*		memset(m_buff, defaultValue, m_buffSize);
		for(U32BIT i=0; i<(m_buffSize/2); i++){
			memcpy(m_buff+2*i, &defaultValue, 2);      //m_buff = 0x5a5a      size = 268 bytes
		}*/   //close by yinhonggen
		size_t remains = filesize;
		char *p = m_buff;
		// 读出数据
		while ( remains > 0 ) {
			int nrd = ::read(fd, p, remains);
			if ( nrd < 0 )
				break;
			p += nrd;
			remains -= nrd;
		}
		if ( remains > 0 ) { // error, load failed!
			close(fd);
			delete []m_buff;
			m_buff = NULL;
			std::cerr << "BC_Msg::loadSimFile(): '" << filename << "' error: " << errno << std::endl;
			usleep(1);
			continue;
		}
		if(BC_Adapter_changeEndian){
			swap_by_word(m_buff, filesize);
		}
		//初始化m_pbuff指针。
		m_pbuff = m_buff;
		close(fd);
		break;
	}

}

/*           
 * 创建当前消息
 *  
**/
void BC_Msg::Create(S16BIT numCard) {
	if (length > 32){
		printf("[BC_Msg::Create](warning)invalid length. messID is %d.\n", id);
		length = 32;
	}

	//创建当前消息的数据区
	U16BIT wTemp[64];
	memset(wTemp, 0, 64);
	if (data && length) {
		memcpy(wTemp, data, length*2);
	}

	S16BIT nResult = 0;
	//异步消息不创建blk，而是将wTemp直接作为参数传给了BCCreateXX函数
	if(!isAsync){
		nResult = aceBCDataBlkCreate(numCard, blkid, 32, wTemp, 32);
		if (nResult) {
			logMsg("[BC_Msg::Create][ERROR] aceBCDataBlkCreate failed.\r\n", 0,
					0, 0, 0, 0, 0);
		}
	}

	
	U32BIT nowOpt = GetOpt();
	U16BIT tempTR = ACE_TX_CMD;
	short addr1 = Addr[0];
	short addr2 = Addr[1];
	if (!isAsync)
	{
		switch (type) {
		case MSG_BROADCAST:
			nResult = aceBCMsgCreateBcst(numCard, id, blkid, Addr[3], length,
				gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateBcst failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_BC_TO_RT:
			nResult = aceBCMsgCreateBCtoRT(numCard, id, blkid, Addr[2],
				Addr[3], length, gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateBCtoRT failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_RT_TO_BC:
			nResult = aceBCMsgCreateRTtoBC(numCard, id, blkid, Addr[0],
				Addr[1], length, gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateBCtoRT failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_RT_TO_RT:
			nResult = aceBCMsgCreateRTtoRT(numCard, id, blkid, Addr[2],
				Addr[3], length, Addr[0], Addr[1], gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateRTtoRT failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_MODE_CODE:
			/*0x11 -->同步    0x14--->选定的发送器关闭  0x15---->取消选定的发送器关闭*/
			if (length == 0x11 || length == 0x14 || length == 0x15){
				tempTR = ACE_RX_CMD;
				addr1 = Addr[2];
				addr2 = Addr[3];
			}
			nResult = aceBCMsgCreateMode(numCard, id, blkid, addr1,
				tempTR, length, gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateMode failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			//默认子地址为0，若指定为31的话，改为31。
			if (addr2 == 31){
				U16BIT cmdword;
				aceCmdWordCreate(&cmdword, addr1, tempTR, addr2, length);
				nResult = aceBCMsgModify( numCard, id, blkid,0, cmdword, 0, 0, 0, 0, 0, 0, 0, ACE_BC_MOD_CMDWRD1_1 );
				if( nResult ){
					logMsg( "[BC_Msg::Create][ERROR] aceBCMsgModify failed.%d\n", nResult, 0, 0, 0, 0, 0 );
				}
			}
			break;
		case MSG_RT_BROADCAST:  //luxq 校对地址Addr[0], Addr[1]
			nResult = aceBCMsgCreateBcstRTtoRT(numCard, id, blkid, Addr[3],
				length, Addr[0], Addr[1], gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateBcstRTtoRT failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_MODE_CODE_BROADCAST:  
			if (length == 0x11 || length == 0x14 || length == 0x15){
				tempTR = ACE_RX_CMD;
				addr2 = Addr[3];
			}
			nResult = aceBCMsgCreateBcstMode(numCard, id, blkid, tempTR,
				length, gap_time, nowOpt);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCMsgCreateBcstMode failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			//默认子地址为0，若指定为31的话，改为31。
			if (addr2 == 31){
				U16BIT cmdword;
				aceCmdWordCreate(&cmdword, 31, tempTR, addr2, length);
				nResult = aceBCMsgModify( numCard, id, blkid,0, cmdword, 0, 0, 0, 0, 0, 0, 0, ACE_BC_MOD_CMDWRD1_1 );
				if( nResult ){
					logMsg( "[BC_Msg::Create][ERROR] aceBCMsgModify failed.%d\n", nResult, 0, 0, 0, 0, 0 );
				}
			}
			break;
		default:
			break;
		}
	}else{
		switch (type) {
		case MSG_RT_TO_BC:
			nResult = aceBCAsyncMsgCreateRTtoBC(numCard, id, blkid, Addr[0],
				Addr[1], length, gap_time, nowOpt, wTemp);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateRTtoBC failed.%d\r\n", nResult, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_BC_TO_RT:
			nResult = aceBCAsyncMsgCreateBCtoRT(numCard, id, blkid, Addr[2],
				Addr[3], length, gap_time, nowOpt, wTemp);
			//logMsg("Create Asyn Msg %d-----\r\n", id, 0, 0, 0, 0, 0);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateBCtoRT failed.%d\r\n", nResult, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_BROADCAST:
			nResult = aceBCAsyncMsgCreateBcst(numCard, id, blkid, Addr[3], length,
				gap_time, nowOpt, wTemp);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateBcst failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_RT_TO_RT:
			nResult = aceBCAsyncMsgCreateRTtoRT(numCard, id, blkid, Addr[2],
				Addr[3], length, Addr[0], Addr[1], gap_time, nowOpt, wTemp);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateRTtoRT failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_MODE_CODE: 
			if (length == 0x11 || length == 0x14 || length == 0x15){
				tempTR = ACE_RX_CMD;
				addr1 = Addr[2];
				addr2 = Addr[3];
			}
			nResult = aceBCAsyncMsgCreateMode(numCard, id, blkid, addr1,
				tempTR, length, gap_time, nowOpt, wTemp);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateMode failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			//默认子地址为0，若指定为31的话，改为31。
			if (addr2 == 31){
				U16BIT cmdword;
				aceCmdWordCreate(&cmdword, addr1, tempTR, addr2, length);
				nResult = aceBCMsgModify( numCard, id, blkid,0, cmdword, 0, 0, 0, 0, 0, 0, 0, ACE_BC_MOD_CMDWRD1_1 );
				if( nResult ){
					logMsg( "[BC_Msg::Create][ERROR] aceBCMsgModify failed.%d\n", nResult, 0, 0, 0, 0, 0 );
				}
			}
			break;
		case MSG_RT_BROADCAST: 
			nResult = aceBCAsyncMsgCreateBcstRTtoRT(numCard, id, blkid, Addr[3],
				length, Addr[0], Addr[1], gap_time, nowOpt, wTemp);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateBcstRTtoRT failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			break;
		case MSG_MODE_CODE_BROADCAST: 
			if (length == 0x11 || length == 0x14 || length == 0x15){
				tempTR = ACE_RX_CMD;
				addr2 = Addr[3];
			} 
			nResult = aceBCAsyncMsgCreateBcstMode(numCard, id, blkid, tempTR,
				length, gap_time, nowOpt, wTemp);
			if (nResult) {
				logMsg("[BC_Msg::Create][ERROR] aceBCAsyncMsgCreateBcstMode failed.\r\n", 0, 0, 0, 0, 0, 0);
			}
			//默认子地址为0，若指定为31的话，改为31。
			if (addr2 == 31){
				U16BIT cmdword;
				aceCmdWordCreate(&cmdword, 31, tempTR, addr2, length);
				nResult = aceBCMsgModify( numCard, id, blkid,0, cmdword, 0, 0, 0, 0, 0, 0, 0, ACE_BC_MOD_CMDWRD1_1 );
				if( nResult ){
					logMsg( "[BC_Msg::Create][ERROR] aceBCMsgModify failed.%d\n", nResult, 0, 0, 0, 0, 0 );
				}
			}
			break;
		default:
			break;
		}
	}
	if (nResult)
	{
		BrAda1553B_BC_COMM::print_err_str(nResult);
	}
}

void BC_Msg::Modify(S16BIT numCard){
	U16BIT tempOpt = GetOpt();
	if (tempOpt == nowOpt)
	{
		return;
	}
	else{
		nowOpt = tempOpt;
	}
	
	S16BIT sresult;

	if (type == MSG_RT_TO_RT){
		//MSG_RT_TO_RT时，aceBCMsgModify用起来比较复杂。
		sresult = aceBCMsgModifyRTtoRT(numCard, id, blkid, Addr[2],
			Addr[3], length, Addr[0], Addr[1], gap_time, tempOpt, ACE_BC_MOD_BCCTRL1);
		if( sresult ){
			logMsg( "[BC_Msg::Modify][ERROR] aceBCMsgModifyRTtoRT failed.%d\n", sresult, 0, 0, 0, 0, 0 );
		}
	} 
	else if (type == MSG_MODE_CODE)
	{
		sresult = aceBCMsgModifyMode(numCard, id, blkid, 0,
			0, 0, 0, tempOpt, ACE_BC_MOD_BCCTRL1);
		if( sresult ){
			logMsg( "[BC_Msg::Modify][ERROR] aceBCMsgModifyMode failed.%d\n", sresult, 0, 0, 0, 0, 0 );
		}
	}
	else if (type == MSG_MODE_CODE_BROADCAST)
	{
		sresult = aceBCMsgModifyBcstMode(numCard, id, blkid, 0,
			0, 0, tempOpt, ACE_BC_MOD_BCCTRL1);
		if( sresult ){
			logMsg( "[BC_Msg::Modify][ERROR] aceBCMsgModifyBcstMode failed.%d\n", sresult, 0, 0, 0, 0, 0 );
		}
	}
	else{
		sresult = aceBCMsgModify( numCard, id, blkid,tempOpt, 0, 0, 0, 0, 0, 0, 0, 0, ACE_BC_MOD_BCCTRL1 );
		if( sresult ){
			logMsg( "[BC_Msg::Modify][ERROR] aceBCMsgModify failed.%d\n", sresult, 0, 0, 0, 0, 0 );
		}
	}
}

/*           
 * 获得消息的通道和重试选项
 *  
**/
U16BIT BC_Msg::GetOpt() {
	U16BIT optTemp;
	if (channel == CHANNEL_A){
		optTemp = ACE_BCCTRL_CHL_A;
	} 
	else if (channel == CHANNEL_B){
		optTemp = ACE_BCCTRL_CHL_B;
	} 
	//channel == CHANNEL_AB   luxq?
	else{  
		if ((m_irqCount % 2) == 0){
			optTemp = ACE_BCCTRL_CHL_A;
		}else{
			optTemp = ACE_BCCTRL_CHL_B;
		}
	}

	if (isRetry)
	{
		optTemp |= ACE_BCCTRL_RETRY_ENA;
	}
	return optTemp;
}

/*   
 * 添加操作                     
 *   devNum:[I]通道号
 * codeType:[I]操作码
 *     para:[I]对应不同的操作码,有不同意义的参数
 *  ACE_OPCODE_IRQ:para表示产生中断类型，跟ACE_IMR2_BC_UIRQ0对应
 *  ACE_OPCODE_FLG:para表示GPF码
 *  ACE_OPCODE_CAL:para表示小帧号
**/
void BC_Ops::AddOp(S16BIT devNum, S16BIT codeType, S16BIT para) {
	S16BIT opId = GetAOpID();
	S16BIT uRet = aceBCOpCodeCreate(devNum, opId, codeType, ACE_CNDTST_ALWAYS,
			para, 0, 0);
	if (uRet) {
		throw std::runtime_error("[BC_Ops::AddOp][ERROR] aceBCOpCodeCreate failed.");
	}
	m_lstOps.push_back(opId);
}

/*           
 * 添加消息
 *    devNum:[I]通道号
 *       msg:[I]消息
 * condition:[I]条件码
**/
void BC_Ops::AddMsg(S16BIT devNum, BC_Msg& msg, S16BIT condition) {
	S16BIT opId = GetAOpID();
	S16BIT uRet = aceBCOpCodeCreate(devNum, opId, ACE_OPCODE_XEQ, condition,
			msg.id, 0, 0);
	if (uRet) {
		throw std::runtime_error("[BC_Ops::AddMsg][ERROR] aceBCOpCodeCreate failed.");
	}
	m_lstOps.push_back(opId);
}

/**
 * 添加延时
 * \param devNum 设备代号
 * \param interval 延时长度，单位微妙
 * \param condition 条件代码
 * \return void
 */
void BC_Ops::AddDelay(S16BIT devNum, U16BIT interval, S16BIT condition) {
	S16BIT opId = GetAOpID();
	S16BIT uRet = aceBCOpCodeCreate(devNum, opId, ACE_OPCODE_DLY, condition,
			interval, 0, 0);
	if (uRet) {
		throw std::runtime_error("[BC_Ops::AddDelay][ERROR] aceBCOpCodeCreate failed.");
	}
	m_lstOps.push_back(opId);
}

S16BIT* BC_Ops::GetArrayAddr() {
	if (m_lstOps.size() > 0) {
		if ( !m_pOps) {
			int iCount = m_lstOps.size();
			m_pOps = new S16BIT[iCount];
			for (int i = 0; i < iCount; ++i) {
				m_pOps[i] = m_lstOps[i];
			}
		}
		return m_pOps;
	} else {
		return NULL;
	}
}

BrAda1553B_BC_COMM::BrAda1553B_BC_COMM() {
	m_sys_service = NULL;
	device_info_bc.cardNum = 0;
	device_info_bc.interval = 0;
	MINOR_FRAME_TIME = 320;
	m_minorFrameCount = 0;
	//m_interface = NULL;
	m_isBrokenMode = FALSE;
	BC_Adapter_changeEndian = FALSE;
}

/*
 * 处理根据其他消息数据判断是否发送的消息
 *
 */
void BrAda1553B_BC_COMM::ProcessVector(void)
{
	S16BIT DevNum = device_info_bc.cardNum;
	MSGSTRUCT Msg;
	
	typedef std::multimap<S16BIT, S16BIT>::iterator itr;
	std::pair<itr, itr> pos;
	
	for(set<S16BIT>::iterator it = m_set_followMsgID.begin(); it != m_set_followMsgID.end(); it++){
#ifndef IPCORE //IPCORE中，异步消息是延时发送的，所以不能在RecvAsyncMsg中被及时收回来
		//不处理异步消息，在RecvAsyncMsg中处理
		S16BIT index = m_mapID[*it];
		if (m_vec_msg[index].isAsync){
			continue;
		}
#endif
		while (1) {
			S16BIT res_vector = aceBCGetMsgFromIDDecoded(DevNum, *it,
					&Msg, TRUE);
			if(res_vector <= 0){break;}
				
			pos = m_multimap_followMsg.equal_range(*it);
			for(; pos.first != pos.second; pos.first++){
				BC_Msg* pBCmsg = &(m_vec_msg[pos.first->second]);
				//如果不检查服务请求位，或者检查服务请求位被设上
				if (!pBCmsg->isCheckSrvbit || (Msg.wStsWrd1Flg > 0 && (Msg.wStsWrd1 & 0x0100) > 0)) {
					U16BIT wVectorData = Msg.aDataWrds[pBCmsg->followDataindex-1];
					//如果数据字与上判断mask的值等于判断值，发送消息
					if((wVectorData & pBCmsg->followMask) == pBCmsg->followValue){
						//同步消息
						if(!pBCmsg->isAsync){ 
							if (!pBCmsg->m_isBrokenMode){
								pBCmsg->SetBCData(device_info_bc.cardNum, m_sys_service);
							}
							pBCmsg->Modify(device_info_bc.cardNum);
							aceBCSetGPFState(DevNum, pBCmsg->GPF, ACE_GPF_SET);
						}
						//异步消息
						else{  
#ifndef IPCORE
							pBCmsg->Create(device_info_bc.cardNum);
#endif
							if (!pBCmsg->m_isBrokenMode){
								pBCmsg->SetBCData(device_info_bc.cardNum, m_sys_service);
							}
#ifdef IPCORE
							aceBCSendAsyncMsgHP(device_info_bc.cardNum, pBCmsg->id, pBCmsg->data, pBCmsg->length, MINOR_FRAME_TIME); 
#else
							aceBCSendAsyncMsgHP(device_info_bc.cardNum, pBCmsg->id, MINOR_FRAME_TIME); 
#endif
							if (m_set_followMsgID.find(pBCmsg->id) != m_set_followMsgID.end()){
								RecvAsyncMsg(pBCmsg->id);
							} 
#ifndef IPCORE
							aceBCMsgDelete(device_info_bc.cardNum, pBCmsg->id);
							aceBCDataBlkDelete(device_info_bc.cardNum, pBCmsg->blkid);
#endif
						}
					}
				}
			}
		} //end of while(1)
	} //end of for
}

/*
 * 中断函数
 *
 */
void _DECL BrAda1553B_BC_COMM::MyISR( S16BIT DevNum, U32BIT dwIrqStatus )
{
	if(!(dwIrqStatus & ACE_IMR2_BC_UIRQ0)){
		return;
	}
	
	BrAda1553B_BC_COMM* pThis = s_adapt[DevNum];

	/*end of message
	if (dwIrqStatus == 1)
	{
		it->m_irqCount++;
		if (it->isCanMiss)
		{
			if (it->m_missFreq != 0 && (m_irqCount % m_missFreq == 0))
			{
				aceBCSetGPFState(DevNum, it->GPF, ACE_GPF_CLEAR);
			}else{
				aceBCSetGPFState(DevNum, it->GPF, ACE_GPF_SET);
			}
		}
		if (it->channel == CHANNEL_AB)
		{
			it->Modify(DevNum);
		}
		if (it->isTimeCode){
			it->SetTimecode(DevNum, pThis->m_sys_service);
		}
	}
*/
	//pThis->ProcessVector(); 
	// 处理write进来的数据
	//pThis->deal_data();
	pThis->m_itr_count++;

	S16BIT frameID = (S16BIT)(pThis->m_itr_count % pThis->m_minorFrameCount);
	if (frameID == 0){
		frameID = pThis->m_minorFrameCount;
	}
	//设置周期消息的停发，同时调整通道
	for(vector<BC_Msg>::iterator it = pThis->m_vec_msg.begin(); it != pThis->m_vec_msg.end(); it++) {
		//如果该消息在不当前小帧中，退出
		if (pThis->m_set_msgPlace.find(make_pair(frameID, it->id)) == pThis->m_set_msgPlace.end()){
			continue;
		}
		
		aceBCGetMsg(DevNum,  it->id, TRUE);
		it->m_irqCount++;
		pThis->deal_data(it->id);
		
		if(it->sendTiming == SENDTIMING_ALWAYS){
			//按设置频率漏发消息
			if( (it->m_missFreq != 0) && 
				(it->m_irqCount  % it->m_missFreq == 0)){
				aceBCSetGPFState(DevNum, it->GPF, ACE_GPF_CLEAR);
				continue;
			}
			it->Modify(DevNum);
//qiu     
#if 0 
			if (!it->m_isBrokenMode){
				it->SetBCData(DevNum, pThis->m_sys_service);
			}
#endif
         /*add by yinhonggen date:2017-02-22*/
          if(it->isFromFile)
          	{
				 it->SetBCData(DevNum, pThis->m_sys_service);
			}
			aceBCSetGPFState(DevNum, it->GPF, ACE_GPF_SET);
		}
	}
}

/*
 * 设置IP地址和卡号
 *
 */
void BrAda1553B_BC_COMM::set_address(const string &host_addr,
		const std::string &if_addr) {
	device_info_bc.hostAddr = host_addr;
	device_info_bc.cardNum = atoi(if_addr.c_str());
	S16BIT card = device_info_bc.cardNum;
	BrAda1553B_BC_COMM::s_adapt[card] = this;
}

/*
* 设置接口对象
*
*/
/* zhanghao close.
void BrAda1553B_BC_COMM::set_interface(const Interface *if_obj){
	m_interface = if_obj;
}
*/

/*
* 设置配置字符串
*
*/
U16BIT BrAda1553B_BC_COMM::set_config(const std::string &cfg) {
	string cfg_copy = cfg;
	string::size_type pos = 0;
	while((pos = cfg_copy.find(';', pos)) != std::string::npos){
		pos++;
		cfg_copy.insert(pos, "\n\r", 2);
	}	

	get_config_value(cfg_copy, "ConfFile", m_fileName);
	m_fileName = "agent.rc/" + m_fileName;
	get_config_value(cfg_copy, "ChangeEndian", BC_Adapter_changeEndian);

	m_minorFrameCount = 0;
    /*读取和配置重新组合配置文件*/
    readConfFile(m_fileName);	
	/*解析配置文件*/	
	 parseConfFile();
}

/*
 * 开始测试
 *
 */
U16BIT BrAda1553B_BC_COMM::source_init() 
{
	//这里应该创建所有东西，然后让bc开始工作
	if (work_stat == WS_RUN) {
		return RET_1553B_ERR;
	}
	//m_sys_service = svr;
	m_write_data_buf.clear();
	m_read_data_buf.clear();
	m_itr_count = 0;
	m_isBrokenMode = FALSE;

	// 各类id的初始值一定要重置，否则一味的增长，start多次之后，会和那些定死的id值发生冲突
	BC_Ops::init();

	if (m_minorFrameCount == 0)
	{
		printf("[BrAda1553B_BC_COMM::Start][WARNING] m_minorFrameCount is 0.\n");
		return RET_1553B_ERR;
	}

	S16BIT DevNum = device_info_bc.cardNum;
	S16BIT nResult = 0x0000;

 	nResult = aceInitialize(DevNum, ACE_ACCESS_CARD, ACE_MODE_BC, 0, 0, 0);
	if (nResult) 
	{
		printf("[BrAda1553B_BC_COMM::Start][ERROR] aceInitialize failed.\n");
		return RET_1553B_ERR;
	}

	// 设置异步消息的优先级
	nResult = aceBCConfigure(DevNum, ACE_BC_ASYNC_BOTH);
	if (nResult) 
	{
		printf("[BrAda1553B_BC_COMM::Start][ERROR] aceBCConfigure failed.\n");
		return RET_1553B_ERR;
	}

	for (vector<BC_Msg>::iterator it = m_vec_msg.begin(); it != m_vec_msg.end(); it++) 
	{
		//载入仿真文件
		if (it->isFromFile)
		{
			it->loadSimFile();
		}

		//创建所有同步消息，这些消息的数据量不会超过32个数据字
		if(!it->isAsync){  
			it->Create(DevNum);
			it->SetBCData(device_info_bc.cardNum, m_sys_service);
		}
#ifdef IPCORE
		//IPCORE中，异步消息也得提前创建
		if(it->isAsync){  
			it->Create(DevNum);
			//it->SetBCData(device_info_bc.cardNum, m_sys_service);
		}
#endif
	}

	//创建MINOR帧
	BC_Ops *minorops = new BC_Ops[m_minorFrameCount], majorops;
	MakeMinorOps(DevNum, minorops);
	//添加Minor1frame
	for (int i = 0; i < m_minorFrameCount; i++)
   {
#ifndef IPCORE
		//在每个小帧的末尾加上一个中断操作
		minorops[i].AddOp(DevNum, ACE_OPCODE_IRQ,1);
#endif
		nResult = aceBCFrameCreate(DevNum, i+1, ACE_FRAME_MINOR,
				minorops[i].GetArrayAddr(), minorops[i].GetSize(),
				MINOR_FRAME_TIME, 0);
		if (nResult) 
		{
			printf("[BrAda1553B_BC_COMM::Start][ERROR] aceBCFrameCreate failed.\n");
			return RET_1553B_ERR;
		}
	}
	delete [] minorops;


	if (m_itrMsgCount == 0){
		m_itrMsgCount = m_minorFrameCount;
	}

	//创建MAJOR帧
	MakeMajorOps(DevNum, majorops);
	nResult = aceBCFrameCreate(DevNum, MAJOR_FRAME_ID, ACE_FRAME_MAJOR,
			majorops.GetArrayAddr(), majorops.GetSize(), 0, 0);
	if (nResult) 
	{
		printf("[BrAda1553B_BC_COMM::Start][ERROR] aceBCFrameCreate failed.\n");
		return RET_1553B_ERR;
	}
 
	//这里清空所有的异步消息，以后创建的所有消息都时低优先级的异步消息
	aceBCEmptyAsyncList(DevNum);
	aceBCResetAsyncPtr(DevNum);

	return RET_1553B_OK;
}

U16BIT BrAda1553B_BC_COMM::start()
{
   S16BIT DevNum =  device_info_bc.cardNum;
	S16BIT nResult = 0x0000;
	
	nResult = aceSetIrqConditions(DevNum, TRUE, ACE_IMR2_BC_UIRQ0, MyISR);
	if (nResult) 
	{
		printf("[BrAda1553B_BC_COMM::start][ERROR] aceSetIrqConditions failed.\n");
		return RET_1553B_ERR;
	}
    //开始
	aceBCStart(DevNum, MAJOR_FRAME_ID, -1);
	work_stat = WS_RUN;
	
	p_AdaBC = this;
	std::cout << "1553B BC started!" << std::endl;

	return RET_1553B_OK;
}
/*
 * 停止测试
 *
 */
U16BIT BrAda1553B_BC_COMM::stop() {
	work_stat = WS_STOP;
	p_AdaBC = NULL;
	//停止
	aceBCStop(device_info_bc.cardNum);
	aceFree(device_info_bc.cardNum);
	m_write_data_buf.clear();
	m_read_data_buf.clear();
	m_vec_msg.clear();
	m_set_followMsgID.clear();
	m_multimap_followMsg.clear();
	m_itr_count = 0;
#ifdef ALL_MODE
	ddc1553b_ISR::m_pBC = NULL;
#endif
	std::cout << "1553B BC stoped!" << std::endl;

	return RET_1553B_OK;
}

/*
 * 将消息组成小帧
 *
 */
void BrAda1553B_BC_COMM::MakeMinorOps(S16BIT DevNum, BC_Ops* ops) {
	//遍历m_map_msgPlace
	for (int frameIndex=0; frameIndex<m_minorFrameCount; frameIndex++)
	{
		vector<S16BIT> vec = m_map_msgPlace[frameIndex+1];
		//遍历每个小帧对应的vector
		for (U32BIT j=0; j<vec.size(); j++)
		{
			S16BIT messID = vec[j];
			S16BIT messIndex = m_mapID[messID];
			BC_Msg* pMsg = &(m_vec_msg[messIndex]);
			ops[frameIndex].AddMsg(DevNum, *pMsg, pMsg->GPF);

			/// 对于根据其他消息数据判断发送的同步消息，在其后加入清除GPF操作
			if (pMsg->sendTiming == SENDTIMING_FOLLOW_SYNC){
				/// 加入清除GPF的操作
				S16BIT tempGPF = 1 << (8 + pMsg->GPF);
				ops[frameIndex].AddOp(DevNum, ACE_OPCODE_FLG, tempGPF);
				/// 让AB通道交替的GPF消息从B通道开始
				if (pMsg->channel == CHANNEL_AB)
				{
					pMsg->m_irqCount++;
				}
			}
		}
	}
}

/**
 * 将小帧组成大帧
 *
 */
void BrAda1553B_BC_COMM::MakeMajorOps(S16BIT DevNum, BC_Ops& ops) {
	for (int i = 0; i < m_minorFrameCount; i++)
		ops.AddOp(DevNum, ACE_OPCODE_CAL, i+1);
}

void BrAda1553B_BC_COMM::ioctl(U16BIT msg_id, msg_is_save flag, const char *save_path)
{
		if (m_mapID.find(msg_id) != m_mapID.end())
		{
            if(flag == ENUM_MSG_SAVE)
            {
					  m_setSaveFlag.insert(msg_id);
					  if(NULL == fp)
					  {
							 fp = fopen(save_path, "w+");
					  }
			  }
			  else
			  {
					if (m_setSaveFlag.find(msg_id) != m_setSaveFlag.end())
					{
						  m_setSaveFlag.erase(msg_id);
					}
			  }

			  if(m_setSaveFlag.empty())
			  {
					if(fp)
					{
						 fclose(fp);
						 fp = NULL;
					}
			  }
		}
		return;
}

U16BIT BrAda1553B_BC_COMM::write(ctl_data_wrd_info *datahead)
{
       my_Lock.lock();
       enum_operate_type type = datahead->type;
       U16BIT messID = datahead->msg_id;
		U16BIT pos = datahead->pos;
		size_t  len = datahead->msg_lenth;
		size_t msg_len;
		size_t dui_len ;
		size_t packlen ;
		size_t size = 0;
		S16BIT index = 0;
		BC_Msg* pmsg  = NULL;
		stHeadInfo h;
		U16BIT data[32];
		U16BIT temp = 0;

       memset((void *)data, 0, sizeof(data));
		switch(type)
		{
             case ENUM_DEL_MSG:
						if (m_mapID.find(messID) == m_mapID.end())
						{
							 printf("[BrAda1553B_BC_COMM::write][warning](ENUM_DEL_MSG)invalid messID:%d.\n", messID);
							 my_Lock.unlock();
							 return RET_1553B_ERR;
						}
					   h.type = ENUM_DEL_MSG;
						h.len = 1; 	
						m_write_data_buf.push(messID, h, (const char *)data); 
						break;
				case ENUM_MODIFY_MSG:
						//检查消息ID
						if (m_mapID.find(messID) == m_mapID.end())
						{
							printf("[BrAda1553B_BC_COMM::write][warning](ENUM_DEL_MSG)invalid messID:%d.\n", messID);
							my_Lock.unlock();
							return RET_1553B_ERR;
						}
						index = m_mapID[messID];
						pmsg = &m_vec_msg[index];
						memcpy(data, pmsg->data, sizeof(data));
						if(pos >= 32 || (NULL == datahead->data))
						{
                        printf("[BrAda1553B_BC_COMM::write][warning](ENUM_DEL_MSG)invalid pos:%d.\n", pos);
							my_Lock.unlock();
							return RET_1553B_ERR;
						}

						msg_len = pmsg->length;
						if(((pos * sizeof(U16BIT)) + len) > (msg_len * sizeof(U16BIT)))
						{
                          len =  (msg_len  - pos) * sizeof(U16BIT);
						}

						memcpy((char *)(data + pos), datahead->data, len);
						h.type = ENUM_MODIFY_MSG;
						h.len = msg_len * sizeof(U16BIT);
						m_write_data_buf.push(messID, h, (const char *)data); 
						break;	
				case ENUM_ADD_MSG:
						 //检查消息ID
						 if (m_mapID.find(messID) == m_mapID.end())
						 {
						      printf("[BrAda1553B_BC_COMM::write][warning](ENUM_ADD_MSG)invalid messID:%d.\n", messID);
								my_Lock.unlock();
							   return RET_1553B_ERR;
						 }

					     if(NULL == datahead->data)
					     {
							   printf("[BrAda1553B_BC_COMM::write][warning](ENUM_ADD_MSG)the datahead->data is NULL\n");
								my_Lock.unlock();
							   return RET_1553B_ERR;
						  }
							 
						  index = m_mapID[messID];
						  pmsg = &m_vec_msg[index];
						  packlen = sizeof(U16BIT) * pmsg->length;
						  h.type = ENUM_ADD_MSG;
						  dui_len = 0;
						  //printf("write len:%d\n",len);
					      while( dui_len < len )
						  {
							  size = ((len - dui_len) > packlen ) ? packlen : (len - dui_len);
							  h.len = size;
							  m_write_data_buf.push(messID, h, (const char *)(datahead->data + dui_len)); 
							  dui_len += size;
							  //printf("dui_len:%d  size:%d\n",dui_len, size);
						   }
					   break; 
				default:
					   break;
		}
		my_Lock.unlock();
		return RET_1553B_OK;
}

/**
 * 处理write进来的数据。
 */
void BrAda1553B_BC_COMM::deal_data( U16BIT messID) 
{
   	 	if (m_mapID.find(messID) != m_mapID.end())
   	 	{
   	 	     S16BIT index = m_mapID[messID];
			  size_t data_size_1553;
			  bool is_odd_size;
			  char *data1553 = NULL;
			  BC_Msg* pmsg = NULL;
			 if (m_write_data_buf.size(messID) > 0)
			 {
			       my_Lock.lock();
					stHeadInfo *head = m_write_data_buf.get_front_header(messID);
					size_t len = head->len;
					const char *data = m_write_data_buf.get_front_data(messID);	
					enum_operate_type type = head->type;
					switch(type)
					{
						case ENUM_DEL_MSG:
							memset((void *)m_vec_msg[index].data, 0, sizeof(unsigned short)*32);
							aceBCDataBlkWrite( device_info_bc.cardNum, m_vec_msg[index].blkid, m_vec_msg[index].data, 32, 0 );
							break;
						case ENUM_MODIFY_MSG:
							if (len > 2*m_vec_msg[index].length)
							{
								 len = 2*m_vec_msg[index].length;
							}
							memcpy(m_vec_msg[index].data, data, len);
							//printf("deal_data ENUM_MODIFY_MSG len:%d\n",len);
							aceBCDataBlkWrite( device_info_bc.cardNum, m_vec_msg[index].blkid, m_vec_msg[index].data, 32, 0 );
							break;
						case ENUM_ADD_MSG:
							pmsg = &m_vec_msg[index];
			              data_size_1553 = len;
			        		is_odd_size = len % 2 > 0;
			              if (is_odd_size)
						   {
								data_size_1553 = len + 1;
							}
				         data1553 = const_cast<char*>(data);
						  if (is_odd_size) 
						  {
							  data1553 = new char[data_size_1553];
							  memset(data1553, 0, data_size_1553);
							  memcpy(data1553, data, len);
						   }
							//printf("deal with len:%d  data_size_1553:%d\n",len, data_size_1553);
						   memcpy((void*)pmsg->data, (unsigned char *)data1553, data_size_1553);
				          if (is_odd_size)
						   {
								delete[] data1553;
						   }
						
						   if(!pmsg->isAsync)
						   {
						      //printf("isAsync:%d\n",pmsg->isAsync);
								aceBCDataBlkWrite( device_info_bc.cardNum, pmsg->blkid, pmsg->data, 32, 0 );	
						   }
						   else
						   {
		                    //创建并发出异步消息
		                           //printf("ENUM_MODIFY_MSG length:%d  blkid:%d\n",pmsg->length, pmsg->blkid);
										pmsg->Create(device_info_bc.cardNum);
										//IPCORE中的异步消息在真的设置标志位时再填消息，防止时间码不准
										if (!pmsg->m_isBrokenMode)
										{
											pmsg->SetBCData(device_info_bc.cardNum, m_sys_service);
										}
                                  aceBCDataBlkCreate(device_info_bc.cardNum, pmsg->id, 32, pmsg->data, 32);
										S16BIT result = aceBCSendAsyncMsgHP(device_info_bc.cardNum, pmsg->id, pmsg->data, pmsg->length, MINOR_FRAME_TIME);
										if (result < 0) 
										{
											 printf("aceBCSendAsyncMsgHP error\n");
											 my_Lock.unlock();
											 return;
										}
										if (m_set_followMsgID.find(pmsg->id) != m_set_followMsgID.end())
									   {
											 RecvAsyncMsg(pmsg->id);
										} 
						     }
						  break;
						default:
							break;	
					}
					m_write_data_buf.pop(messID);
					my_Lock.unlock(); 
			 }
             
		}
}			
	
void BrAda1553B_BC_COMM::CombSetConfig(const string & cfg)
{
	size_type pos1 = std::string::npos;
	size_type pos2 = std::string::npos;
	std::string cfg_key, cfg_value;

	string cfg_copy = cfg;

	// 去掉空格和换行 
	while ((pos1 = cfg_copy.find(' ')) != std::string::npos || (pos1
		= cfg_copy.find('\n')) != std::string::npos || (pos1
		= cfg_copy.find('\r')) != std::string::npos || ( pos1 = cfg_copy.find('\t') ) != std::string::npos ) 
	{
		cfg_copy.erase(pos1, 1);
	}

	size_type start_pos = 0;
	while ((
		(pos1 = cfg_copy.find('=', start_pos)) != std::string::npos
		) && (
		(pos2= cfg_copy.find(';', pos1)) != std::string::npos
		)) 
	{

		// 将等号前面的字符串存入cfg_key，等号和分号之间的字符串存入cfg_value
		string strKey = cfg_copy.substr(start_pos, pos1-start_pos);
		string strValue =cfg_copy.substr(pos1+1, pos2-pos1-1); 

		// 大小写转换
		int len = strKey.length() +1;
		auto_ptr<char> pChar((char *)malloc(len));


		memset(pChar.get(),0,len);
		memcpy(pChar.get(),strKey.c_str(),len);
		my_strupr((char *)pChar.get(),len);

		strKey = string((char *)pChar.get());
    
		m_item_value[strKey] = strValue;
		//m_item_value.insert(std::make_pair(strKey, strValue));
		start_pos = pos2+1;
	}
}

/**
 * 打印错误信息。
 */
void BrAda1553B_BC_COMM::print_err_str(S16BIT result) {
#ifndef IPCORE
	// 错误字符串
	char error_str[128];
	aceErrorStr(result, error_str, 128);
#endif
//	printf("result_no:%d;err_info:%s\r\n", result, error_str);
}

bool  BrAda1553B_BC_COMM::get_item(const string &item, string &value)
{
		// 大小写转换
		int len = item.length() +1;
		auto_ptr<char> pChar((char *)malloc(len));
		memset(pChar.get(),0,len);
		memcpy(pChar.get(),item.c_str(),len);
		my_strupr((char *)pChar.get(),len);

		map<string, string>::iterator it = m_item_value.find(pChar.get());
		if (it != m_item_value.end())
		{
		value = it->second;
		return true;
		}		
		return  false;
}

bool  BrAda1553B_BC_COMM::readConfFile(string fileName)
{
	char buff[1024];
	FILE * pFile;
	string bc_StrConfig;  //存储配置文件里的内容
	
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	pFile = fopen(fileName.c_str(),"r");
#ifdef WIN32
#pragma warning( pop )
#endif
	if (NULL == pFile )
	{
		std::cout << "BrAda1553B_BC_COMM::readConfFile: file '" << fileName << "' fopen faile!" << std::endl;
		return false;
	}
	
	std::cout<<"Load  "<< fileName << "  ok!\n"<<std::endl;
	int iRead = fread(buff,sizeof(char),1024,pFile);
	while(iRead > 0)
	{	
		//printf("%s",buff);
		string strLine(buff, iRead);
		bc_StrConfig += strLine;
		iRead = fread(buff,sizeof(char),1024,pFile);
	}
	//printf("%s\n",bc_StrConfig.c_str());
	fclose(pFile);

	 try {
			// 数据提供给解析模块
			CombSetConfig(bc_StrConfig);
			CombBcMsgData();
		}
		catch ( ... )
		{
			 return false;
		}

   return true;
}

void BrAda1553B_BC_COMM::Sub_Data(string str,vector<string> & vecStr,char cc)
{
	str.erase(0,1);
	str.erase(str.length()-1,1);
	int pos = 0;
	int iFind = str.find(cc);
	while(iFind > 0)
	{
		string sub = str.substr(pos,iFind - pos);

		vecStr.push_back(sub);

		pos = iFind + 1;

		iFind = str.find(cc,pos);
	}

	if ((int)str.length() > pos )
	{
		string sub = str.substr(pos,str.length() - pos);

		vecStr.push_back(sub);
	}
}

void BrAda1553B_BC_COMM::ParserData(string &str)
{
	vector<string> DataVector;
	vector<string> vecStr;
	string strLine;
	string strval;
	
	//cout<< str.c_str()<< endl;
	strval.clear();
	get_item(str,strval);
	Sub_Data(strval,DataVector,'|');
	
	for(size_t j = 0; j < DataVector.size(); j++)
	{
		strLine.clear();
		strLine = DataVector.at(j);
		//cout<<strLine.c_str()<<endl;
		vecStr.clear();
		Sub_Data(strLine, vecStr, ',');
		//data_value.insert(std::make_pair(j, vecStr));
		data_value[j] = vecStr;
	}
}

void BrAda1553B_BC_COMM::CombBcMsgData(void)
{
     string str[MAX_TYPE];
	  string strval;
	  vector<string> vecStr;
	  int i = 0;

	  //str[0] = "Msg_name";
	  str[0] = "Msg_id";
	  str[1] = "Block_id";
	  str[2] = "Msg_type";
	  str[3] = "Is_async";
	  str[4] = "Gap_time";
	  str[5] = "Msg_length";
	  str[6] = "Channel_select";
	  str[7] = "Is_retry";
	  str[8] = "Source_addr";
	  str[9] = "Source_sub_addr";
	  str[10] = "Dest_addr";
	  str[11] = "Dest_sub_addr";
	  str[12] = "Send_timing";
	  str[13] = "Cyc_time";
	  //str[15] = "Follow_msg_id";
	  //str[16] = "Follow_value";
	  //str[17] = "Follow_mask";
	  //str[18] = "Fram_index";
	  //str[19] = "Follow_data_index";
	  //str[20] = "Is_check_srvbit";
	  //str[21] = "Is_can_miss";
	 // str[22] = "Is_time_code";
	  str[14] = "Is_check_crc";
	  str[15] = "Crc_mode";
	  str[16] = "Crc_timing";
	  str[17] = "Is_from_file";
	  str[18] = "Pack_len";	
	 // str[28] = "Default_value";
	  str[19] = "Filename";	
	  str[20] = "Data";

      
	  for(i = 0; i < MAX_TYPE - 1; i++)
	  {
		     strval.clear();
			  get_item(str[i],strval);
			  vecStr.clear();
			  Sub_Data(strval, vecStr,',');
			  member_value[i] = vecStr;
	  }
	  
	 ParserData(str[20]); 		  	
}

void BrAda1553B_BC_COMM::GetBcMsgData(int i, char  *data)
{
      int index = 0;
	   vector<string> DataStrVec;
		string str[MAX_TYPE -1];	
		BC_Msg_info msg;

	   memset(&msg, 0, sizeof(BC_Msg_info));
		
      for(index = 0; index <  MAX_TYPE - 1; index++)
	  {
			 map<int,vector<string> >::iterator it  = member_value.find(index);
			 if (it != member_value.end())
			 {
					DataStrVec.clear();
					DataStrVec = it->second;
					str[index].clear();
					if(i < DataStrVec.size())
					{
						 str[index] = DataStrVec.at(i);
					}
					else
					{
                      str[index] = "0";
					}
			  }
	   }

       if(i < data_value.size())
       {
              map<int,vector<string> >::iterator it = data_value.find(i);
				if (it != data_value.end())
				{
						DataStrVec.clear();
						DataStrVec = it->second;
				}
				for(index = 0; index < DataStrVec.size(); index++)
				{
						string strLine;
						strLine.clear();
						strLine = DataStrVec.at(index);
						msg.data[index] =static_cast<unsigned short>(hex_atoi(strLine.c_str()));
				}	
		}
		
		//strcpy(msg.name, "haha");
		msg.id     =  static_cast<short>(hex_atoi(str[0].c_str()));
		msg.blkid = static_cast<short>(hex_atoi(str[1].c_str()));
		msg.type  = static_cast<enumMsgType>(hex_atoi(str[2].c_str()));
		msg.isAsync = static_cast<BOOL>(hex_atoi(str[3].c_str()));
		msg.gap_time = static_cast<unsigned int>(hex_atoi(str[4].c_str()));
		msg.length   = static_cast<short>(hex_atoi(str[5].c_str()));
		msg.channel = static_cast<enumChannel>(hex_atoi(str[6].c_str()));
       msg.isRetry  = static_cast<BOOL>(hex_atoi(str[7].c_str()));
		msg.Addr[0] = static_cast<short>(hex_atoi(str[8].c_str()));
		msg.Addr[1] = static_cast<short>(hex_atoi(str[9].c_str()));
		msg.Addr[2] = static_cast<short>(hex_atoi(str[10].c_str()));
		msg.Addr[3] = static_cast<short>(hex_atoi(str[11].c_str()));
		msg.sendTiming = static_cast<enumSendTiming>(hex_atoi(str[12].c_str()));
		msg.time = static_cast<unsigned int>(hex_atoi(str[13].c_str()));
		//msg.followMsgID = static_cast<short>(hex_atoi(str[15].c_str()));
		//msg.followValue = static_cast<unsigned short>(hex_atoi(str[16].c_str()));
		//msg.followMask = static_cast<unsigned short>(hex_atoi(str[17].c_str()));
		//msg.frameIndex = static_cast<short>(hex_atoi(str[18].c_str()));
		//msg.followDataindex = static_cast<short>(hex_atoi(str[19].c_str()));
		//msg.isCheckSrvbit = static_cast<BOOL>(hex_atoi(str[20].c_str()));
		//msg.isCanMiss = static_cast<BOOL>(hex_atoi(str[21].c_str()));
		//msg.isTimeCode = static_cast<BOOL>(hex_atoi(str[22].c_str()));
		msg.isCheckCRC = static_cast<BOOL>(hex_atoi(str[14].c_str()));
		msg.CRCMode = static_cast<short>(hex_atoi(str[15].c_str()));
		msg.CRCTiming = static_cast<short>(hex_atoi(str[16].c_str()));
		msg.isFromFile = static_cast<BOOL>(hex_atoi(str[17].c_str()));
		msg.packLen = static_cast<short>(hex_atoi(str[18].c_str()));
		//msg.defaultValue = static_cast<unsigned short>(hex_atoi(str[28].c_str()));
		strcpy(msg.filename, str[19].c_str());

		if(NULL != data)
		{
 			 memcpy(data, &msg, sizeof(BC_Msg_info));
		}
}

/**
* 解析配置文件，也就是BC配置文件BC_conf.bat的内容。
CHECK_STRING + 消息结构体长度 + 小帧个数 + 小帧延时 + 消息个数 + 消息n + 消息位置个数 + 消息位置n
*/
void BrAda1553B_BC_COMM::parseConfFile(void)
{
    string strval;
	m_itrMsgCount = 0;

	#if 0//消息结构体长度
	strval.clear();
	get_item("MessageLen", strval);
	int messLen = static_cast<int>(hex_atoi(strval.c_str()));
#ifdef __vxworks
	Br_Util::swap_bytes(messLen);
#endif
	if (messLen != sizeof(BC_Msg_info)){
		throw std::logic_error("[BrAda1553B_BC_COMM::write_data][ERROR]ConfFile's version is too low.");
	}
#endif
	//取得小帧个数
	strval.clear();
	get_item("MinorFrameCount", strval);
	m_minorFrameCount = static_cast<int>(hex_atoi(strval.c_str()));
#ifdef __vxworks
	Br_Util::swap_bytes(m_minorFrameCount);
#endif
	for (int i=0; i<m_minorFrameCount; i++){
		m_map_msgPlace.insert(make_pair(i+1,vector<S16BIT>()));
	}
	//取得小帧延时
	strval.clear();
	get_item("MinorFrameTime", strval);
	MINOR_FRAME_TIME = static_cast<int>(hex_atoi(strval.c_str()));
#ifdef __vxworks
	Br_Util::swap_bytes(MINOR_FRAME_TIME);
#endif
	MAJOR_FRAME_TIME = m_minorFrameCount * MINOR_FRAME_TIME;

	map<unsigned int, short> mapGPF_time;  //<time, GPF>
	vector<pair<FollowInfo, short> > vecGPF_follow;  //<followInfo, GPF>
	m_GPF = 2;  //从2开始
	//消息个数
   	strval.clear();
	get_item("MsgCount", strval);
	int msg_count = static_cast<int>(hex_atoi(strval.c_str()));
#ifdef __vxworks
	Br_Util::swap_bytes(msg_count);
#endif
	for(int i=0; i<msg_count; i++){
		BC_Msg msg;
		char msg_buf[sizeof(BC_Msg_info)];
		memset(msg_buf, 0, sizeof(BC_Msg_info));
		GetBcMsgData(i, msg_buf);
		memcpy(&msg, msg_buf, sizeof(BC_Msg_info));
		
    /*printf("name:%s\n  id:%d\n  blkid:%d\n type:%d\n  isAsync:%d\n  gap_time:%d\n length:%d\n channel:%d\n isRetry:%d\n addr[0]:%d\n  addr[1]:%d\n addr[2]:%d\n   addr[3]:%d\n sendTiming:%d\n  time:%d\n followMsgID:%d\n  followValue:%d\n  followMask:%d\n  frameIndex:%d\n  followDataindex:%d\n  isCheckSrvbit:%d\n  isCanMiss%d\n  isTimeCode:%d\n  isCheckCRC:%d\n CRCMode:%d\n CRCTiming:%d\n isFromFile:%d\n packLen:%d\n defaultValue:%d\n filename:%s\n",
			msg.name,msg.id,msg.blkid,msg.type,msg.isAsync,msg.gap_time,msg.length,
			msg.channel,msg.isRetry,msg.Addr[0],msg.Addr[1],msg.Addr[2],msg.Addr[3],
			msg.sendTiming,msg.time,msg.followMsgID,msg.followValue,msg.followMask,
			msg.frameIndex,msg.followDataindex,msg.isCheckSrvbit,msg.isCanMiss,
			msg.isTimeCode,msg.isCheckCRC,msg.CRCMode,msg.CRCTiming,msg.isFromFile,
			msg.packLen,msg.defaultValue,msg.filename);*/
#ifdef __vxworks
		Br_Util::swap_bytes(msg.id);
		Br_Util::swap_bytes(msg.blkid);
		Br_Util::swap_bytes(msg.type);
		Br_Util::swap_bytes(msg.gap_time);
		Br_Util::swap_bytes(msg.length);
		Br_Util::swap_bytes(msg.channel);
		Br_Util::swap_bytes(msg.Addr[0]);
		Br_Util::swap_bytes(msg.Addr[1]);
		Br_Util::swap_bytes(msg.Addr[2]);
		Br_Util::swap_bytes(msg.Addr[3]);
		Br_Util::swap_bytes(msg.sendTiming);
		Br_Util::swap_bytes(msg.time);
		Br_Util::swap_bytes(msg.followMsgID);
		Br_Util::swap_bytes(msg.followValue);
		Br_Util::swap_bytes(msg.followMask);
		Br_Util::swap_bytes(msg.frameIndex);
		Br_Util::swap_bytes(msg.followDataindex);
		Br_Util::swap_bytes(msg.CRCMode);
		Br_Util::swap_bytes(msg.CRCTiming);
		Br_Util::swap_bytes(msg.packLen);
		Br_Util::swap_bytes(msg.defaultValue);
#endif
		if(BC_Adapter_changeEndian){
			swap_by_word((char*)msg.data, 64);
		}
		//设置isAsync
		if (msg.sendTiming == SENDTIMING_FOLLOW_ASYNC || msg.sendTiming == SENDTIMING_DUP){
			msg.isAsync = TRUE;
		}else{
			msg.isAsync = FALSE;
		}
		//对于根据别的消息结果判断是否发送的消息，设置m_set_followMsgID、m_multimap_followMsg
		//同步但是小帧号不合法的这种消息不对其分配GPF
		if(msg.sendTiming == SENDTIMING_FOLLOW_SYNC || msg.sendTiming == SENDTIMING_FOLLOW_ASYNC){ 
			m_set_followMsgID.insert(msg.followMsgID);
			m_multimap_followMsg.insert(make_pair(msg.followMsgID, i));
			//如果是同步的,给消息分配GPF，并设置m_mapGPF
			if (!msg.isAsync)
			{
				//同样判断信息的同步消息共用一个GPF
				FollowInfo fi(msg.followMsgID, msg.followValue, msg.followMask, msg.followDataindex, msg.isCheckSrvbit);
				vector<pair<FollowInfo, short> >::iterator it = vecGPF_follow.end();
				for(it = vecGPF_follow.begin(); it != vecGPF_follow.end(); it++){
					if(it->first == fi){
						break;
					}
				}
				if (it != vecGPF_follow.end()){
					msg.GPF = it->second;
				}else{
					if (m_GPF > 6){
						printf("[BrAda1553B_BC_COMM::write_data][WARNING] m_GPF > 6. msgId is %d.\n", msg.id);
					}
					msg.GPF = m_GPF;
					vecGPF_follow.push_back(make_pair(fi, msg.GPF));
					m_GPF++;
				}
				m_mapGPF[msg.GPF] = msg.id;
			}
		}

		//对于可以停发漏发的固定消息，单独分配GPF
		if (msg.sendTiming == SENDTIMING_ALWAYS && msg.isCanMiss){
			if (m_GPF > 6){
				printf("[BrAda1553B_BC_COMM::write_data][WARNING] m_GPF > 6. msgId is %d.\n", msg.id);
			}
			msg.GPF = m_GPF;
			m_GPF++;
		}

		if(msg.isFromFile && msg.packLen == 0){
			msg.isFromFile = false;
		}
		
		m_vec_msg.push_back(msg);
		m_mapID.insert(make_pair(msg.id, i));
	}

	//消息位置个数
	strval.clear();
	get_item("MsgPlaceCount", strval);
	int msgplace_count = static_cast<int>(hex_atoi(strval.c_str()));
#ifdef __vxworks
	Br_Util::swap_bytes(msgplace_count);
#endif
   vector<string> tempIdVec[2];
   strval.clear();
   get_item("tempFrameID", strval);
   tempIdVec[0].clear();
   Sub_Data(strval, tempIdVec[0], ',');

   strval.clear();
   get_item("tempMsgID", strval);
   tempIdVec[1].clear();
   Sub_Data(strval, tempIdVec[1], ','); 

   for(int i=0; i<msgplace_count; i++)
   {
       strval.clear();
       strval = tempIdVec[0].at(i);
		S16BIT tempFrameID = static_cast<S16BIT>(hex_atoi(strval.c_str()));
#ifdef __vxworks
		Br_Util::swap_bytes(tempFrameID);
#endif
 		strval.clear();
       strval = tempIdVec[1].at(i);
		S16BIT tempMsgID = static_cast<S16BIT>(hex_atoi(strval.c_str()));
#ifdef __vxworks
		Br_Util::swap_bytes(tempMsgID);
#endif
		m_set_msgPlace.insert(make_pair(tempFrameID, tempMsgID));
		m_map_msgPlace[tempFrameID].push_back(tempMsgID);
		m_set_msgID_inFrame.insert(tempMsgID);
    }
}

void BrAda1553B_BC_COMM::RecvAsyncMsg(short msgID){
	S16BIT DevNum = device_info_bc.cardNum;
	MSGSTRUCT Msg;
	typedef std::multimap<S16BIT, S16BIT>::iterator itr;
	std::pair<itr, itr> pos;

	while (1) {
		S16BIT res_vector = aceBCGetMsgFromIDDecoded(DevNum, msgID,
			&Msg, TRUE);
		if(res_vector <= 0){break;}

		pos = m_multimap_followMsg.equal_range(msgID);
		for(; pos.first != pos.second; pos.first++){
			BC_Msg* pBCmsg = &(m_vec_msg[pos.first->second]);
			//如果不检查服务请求位，或者检查服务请求位被设上
			if (!pBCmsg->isCheckSrvbit || (Msg.wStsWrd1Flg > 0 && (Msg.wStsWrd1 & 0x0100) > 0)) {
				U16BIT wVectorData = Msg.aDataWrds[pBCmsg->followDataindex-1];
				//如果数据字与上判断mask的值等于判断值，发送消息
				if((wVectorData & pBCmsg->followMask) == pBCmsg->followValue){
					//同步消息
					if(!pBCmsg->isAsync){ 
						if (!pBCmsg->m_isBrokenMode){
							pBCmsg->SetBCData(device_info_bc.cardNum, m_sys_service);
						}
						pBCmsg->Modify(device_info_bc.cardNum);
						aceBCSetGPFState(DevNum, pBCmsg->GPF, ACE_GPF_SET);
					}
					//异步消息
					else{  
#ifndef IPCORE
						pBCmsg->Create(device_info_bc.cardNum);
#endif
						if (!pBCmsg->m_isBrokenMode){
							pBCmsg->SetBCData(device_info_bc.cardNum, m_sys_service);
						}
#ifdef IPCORE
							aceBCSendAsyncMsgHP(device_info_bc.cardNum, pBCmsg->id, pBCmsg->data, pBCmsg->length, MINOR_FRAME_TIME); 
#else
							aceBCSendAsyncMsgHP(device_info_bc.cardNum, pBCmsg->id, MINOR_FRAME_TIME); 
#endif
						if (m_set_followMsgID.find(pBCmsg->id) != m_set_followMsgID.end()){
							RecvAsyncMsg(pBCmsg->id);
						} 
#ifndef IPCORE
						aceBCMsgDelete(device_info_bc.cardNum, pBCmsg->id);
						aceBCDataBlkDelete(device_info_bc.cardNum, pBCmsg->blkid);
#endif
					}
				}
			}
		}
	} //end of while(1)
}
