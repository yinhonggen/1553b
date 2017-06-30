/*	
 * IPCORE.cpp
 * 实现驱动通用功能的一些函数实现
 * \date 2013-6-5
 * \author xiaoqing.lu
 */
#include "zynq_1553b_api.h"
#include "IPCORE.h"
#include "head.h"
#include "common_qiu.h"
#ifdef __vxworks
#include <vxBusLib.h>
#endif

typedef void 		(*VOIDFUNCPTR) (...);

//#ifdef DUMP_RAM
int IrqConnect(UINT16 cardNum, UINT16 chnNum, void(*funcIsr)( UINT16 cardNum,  UINT16 chnNum)){return 1;};
int IrqEnable(UINT16 cardNum, UINT16 chnNum, BOOL enable){return 1;};
int GetCardCount(){return 1;};
int open( UINT16 cardNum ){return 1;};
int close( ){return 1;};
//#endif


map<S16BIT, CDeviceInfo*> map_devInfo;


/**
 * 一些BC、RT、MT都要用到的接口
 */
void g_1553B_myISR(UINT16 cardNum, UINT16 chnNum)
{
	S16BIT DevNum = cardNum * 2 + chnNum;
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return;
	}

	CDeviceInfo* dev_info = map_devInfo[DevNum];

	dev_info->isr_thread.syncEvent.signal();
	if(dev_info->IPCORE_mode == ACE_MODE_MT && dev_info->isr_thread.MT_sync){
		dev_info->isr_thread.MT_sync->signal();
	}

}

S16BIT aceSetIrqConditions(S16BIT DevNum, 
		U16BIT bEnable, 
		U32BIT dwIrqMask, 
		void(*funcExternalIsr)(S16BIT DevNum, U32BIT dwIrqStatus))
{
	map_devInfo[DevNum]->g_1553B_usrISR = funcExternalIsr;
	
	char name[32]= {0};
	sprintf(name, "ISR_Thread%d", DevNum);
	map_devInfo[DevNum]->isr_thread.start(name);

	//挂1553B中断
	IrqConnect(DevNum/2, DevNum%2, g_1553B_myISR);
	IrqEnable(DevNum/2, DevNum%2, TRUE);

	return 0;
}

S16BIT aceCmdWordCreate(U16BIT *pCmdWrd,
					  U16BIT wRT,
					  U16BIT wTR,
					  U16BIT wSA,
					  U16BIT wWC)
{
	AT_MSG_COMMAND* pcmd = reinterpret_cast<AT_MSG_COMMAND*>(pCmdWrd);
	pcmd->Command.DataNum = wWC;
	pcmd->Command.SubAddr = wSA;
	pcmd->Command.TRFlag = wTR;
	pcmd->Command.RTAddr = wRT;
	return 0;
}

S16BIT aceCmdWordParse(U16BIT wCmdWrd,
						U16BIT *pRT,   //RT地址
						U16BIT *pTR,  //发送与接收
						U16BIT *pSA,  //子地址
						U16BIT *pWC)  //数据长度
{
	AT_MSG_COMMAND* pcmd = reinterpret_cast<AT_MSG_COMMAND*>(&wCmdWrd);
	*pWC = pcmd->Command.DataNum;
	*pSA = pcmd->Command.SubAddr;
	*pTR = pcmd->Command.TRFlag;
	*pRT = pcmd->Command.RTAddr;
	return 0;
}

S16BIT aceInitialize(S16BIT DevNum, 
		U16BIT wAccess, 
		U16BIT wMode, 
		U32BIT dwMemWrdSize, 
		U32BIT dwRegAddr, 
		U32BIT dwMemAddr,
		SyncEvent* MTsync)
{
	int cardCount = GetCardCount();
	if(DevNum < 0 || DevNum > (cardCount * 2 - 1)){
		printf("[aceInitialize]aceInitialize failed. invalid DevNum%d.\n", DevNum);
		return ACE_ERR_INVALID_DEVNUM;
	}	
	if (map_devInfo.find(DevNum) != map_devInfo.end()){
		printf("[aceInitialize]aceInitialize failed. channel%d is used.\n", DevNum);
		return ACE_ERR_INVALID_DEVNUM;
	}
	int ret = open(DevNum/2);
	if (ret == 0)
	{
		printf("[aceInitialize]aceInitialize failed. open card error. channel is %d.\n", DevNum);
		return -1;
	}

	AtWriteReg( DevNum, 0, 0x6000 );
	while( AtReadReg( DevNum, 0 ) & 0x2000 )
	{
		printf( "Reset\r\n");	
		sleep(1000); 
	}

    int wIndex;
	WORD wRegVal;
    /*Check Ram*/
	for ( wIndex = 0; wIndex < AT_RAM_SIZE; wIndex++ )
	{
		AtWriteRam( DevNum, wIndex, wIndex );	
	}
	for ( wIndex = 0; wIndex < AT_RAM_SIZE; wIndex++ )
	{
		WORD kk = AtReadRam( DevNum, wIndex );
		if ( kk != wIndex )
		{
			WORD kk1 = AtReadRam( DevNum, wIndex );
			printf("Ram %d Check Error.\n", wIndex);
			return -1;
		}		
	}
     /*Initialize Ram*/
	for ( wIndex = 0; wIndex < AT_RAM_SIZE; wIndex++ )//AT_SHARE_RAM_SIZE
	{
		AtWriteRam( DevNum, wIndex, 0x0 );
	}

	for ( wIndex = 0; wIndex < AT_RAM_SIZE; wIndex++ )
	{
		if ( (wRegVal = AtReadRam( DevNum, wIndex )) != 0x0 )
		{
			printf("Ram %d Init Error.\n", wIndex);
			return -1;
		}		
	}

    	printf("Ram init OK.\n");

	CDeviceInfo* devInfo = new CDeviceInfo(DevNum);
	map_devInfo[DevNum] = devInfo;

	devInfo->IPCORE_mode = wMode; 
	if(wMode == ACE_MODE_BC){
 		//AtWriteReg( DevNum, 0x100, 0x80b1 );
		//Initialization
		wRegVal = AtReadReg( DevNum, 0 );
		AtWriteReg( DevNum, 0, wRegVal | 0x0216 );//Message Timeout, Dynamic Bus Control Acceptance，Broadcast Enable

		wRegVal = AtReadReg( DevNum, 1 );
		wRegVal |= 5 << 11;
		AtWriteReg( DevNum, 1, wRegVal & 0xFC7F );//MSEL[1:0]设置为BC(00)，Bit7选择1553B标准
		//Register 3
		AtWriteReg( DevNum, 3, 0xFFFF );//Masking Interrupt
		//Register 8 - 命令块指针
		AtWriteReg( DevNum, 8, 0 );//Should be Zero.
		//Register 5 - Interrupt Pointer
		AtWriteReg( DevNum, 5, AT_RAM_SIZE - 32 );//Interrupt Log List
		//Register 32 - Enabled Asynchronous Message
		wRegVal = AtReadReg( DevNum, 32 );
		AtWriteReg( DevNum, 32, wRegVal | (1 << 5) );
		
	}
	else if(wMode == ACE_MODE_RT || wMode == ACE_MODE_RTMT){
		//AtWriteReg( DevNum, 0x100, 0x80b5 );
		//Register 0
		AtWriteReg( DevNum, 0, 0x1816 );//0x181A Bus A & B Enable, PPEN enable, Circular-Mode0
		//Register 1
		wRegVal = AtReadReg(DevNum, 1);
		//设置RT模式的同时需要设置RT的地址，如果先设置RT模式，再进行RT地址则会报错。luxq
		AtWriteReg( DevNum, 1, 0x817F);//MSEL[1:0]设置为RT(01)
		//Register 3
		AtWriteReg( DevNum, 3, 0xFFFF);//Masking Interrupt
		//Register 8 - 描述块指针
		AtWriteReg( DevNum, 8, 0 );
		//Register 9 状态字寄存器
		//AtWriteReg( DevNum, 9, 0x4000 );
		AtWriteReg( DevNum, 5, AT_RAM_SIZE - 32 );//Interrupt Log List
		//初始化RT合法化寄存器。方式命令按照协议设置。非法：1
		for(WORD i=16; i<32; i++){
			AtWriteReg( DevNum, i,0 );
		}
		//open by yinhonggen
		AtWriteReg( DevNum, 24, 0xFFFF );//Receive Mode Code 15 to 0
		AtWriteReg( DevNum, 25, 0xFFCD );//Receive Mode Code 31 to 16
		AtWriteReg( DevNum, 26, 0xFE00 );//Transmit Mode Code 15 to 0
		AtWriteReg( DevNum, 27, 0xFFF2 );//Transmit Mode Code 31 to 16 
	}
	else if(wMode == ACE_MODE_MT){
		//AtWriteReg( DevNum, 0x100, 0x80b9 );
		//Register 0
		AtWriteReg( DevNum, 0, 0x0212 );
		//Register 1
		AtWriteReg( DevNum, 1, 0x0a00 );
		//Register 3
		AtWriteReg( DevNum, 3, 0xF7FF );//Masking Interrupt,不要message error 中断
		//Register 11 - Monitor Block Pointer
		AtWriteReg( DevNum, 11, 0 );
		//Register 12 - Monitor Data Pointer
		AtWriteReg( DevNum, 12, MT_BUFF_SIZE );
		//Register 13 -Monitor Block Count
		AtWriteReg( DevNum, 13, (MT_BLOCK_COUNT - 1) ); //缓存MT_BLOCK_COUNT条消息
		AtWriteReg( DevNum, 14, 0xffff );
		AtWriteReg( DevNum, 15, 0xffff );
		//Register 5
		AtWriteReg( DevNum, 5, AT_RAM_SIZE - 32 );
	}		
//	spiWriteReg(0x0300, 0xffff);
	devInfo->IPCORE_wIntPtr = AT_RAM_SIZE - 32;
	devInfo->is_IPCORE_start = FALSE;
	
	devInfo->MT_msg_now = 0;
	devInfo->isr_thread.MT_sync = MTsync;

     printf("aceInitialize end! \n");
    //showReg();
	return 0;
}

/**
 * 释放消息和数据所占的空间
 */
S16BIT aceFree(S16BIT DevNum)
{   
	int cardCount = GetCardCount();
	if(DevNum < 0 || DevNum > (cardCount * 2 - 1)){
		return ACE_ERR_INVALID_DEVNUM;
	}	
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return ACE_ERR_INVALID_DEVNUM;
	}
	
#ifdef DUMP_RAM //luxq
//	dumpRAM();
#endif

	IrqEnable(DevNum/2, DevNum%2, FALSE);
    
	CDeviceInfo* devInfo = map_devInfo[DevNum];
	if (devInfo)
	{
		devInfo->isr_thread.stop();
		devInfo->isr_thread.syncEvent.signal();
		if (devInfo->IPCORE_mode == ACE_MODE_MT && devInfo->isr_thread.MT_sync)
		{
			devInfo->isr_thread.MT_sync->signal();
		}
		devInfo->isr_thread.sync();
		delete devInfo;
		devInfo = NULL;
	}
	map_devInfo.erase(DevNum);

	if (map_devInfo.empty())
	{
		//关闭所有板卡
		close();
	}
	return 0;
}