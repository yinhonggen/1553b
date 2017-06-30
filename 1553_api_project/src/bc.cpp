/*	
 * bc.cpp
 * 实现驱动BC功能的一些接口实现
 * \date 2013-6-5
 * \author xiaoqing.lu
 */
#include "IPCORE.h"
#include "head.h"
#include "common_qiu.h"
#include "stdio.h"
#include "callback.h"
extern map<S16BIT, CDeviceInfo*> map_devInfo;
extern void SetTimecodeForAsync(WORD msgNO, U16BIT* data);

bool g_1553B_BCISR(S16BIT DevNum)
{	
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return false;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	WORD wRegVal = AtReadReg( DevNum, 5 );
	if (wRegVal < AT_RAM_SIZE - 32)
	{
		return false;;
	}
	WORD wIntData[2];
	bool hasMsg = false;
	while( wRegVal != dev_info->IPCORE_wIntPtr ){
		AtReadBlock(DevNum, dev_info->IPCORE_wIntPtr, wIntData, 2 );
		if( wIntData[0] == 0x0002 )
		{
			WORD IntBlkAddr = wIntData[1];
			
			//小帧末和异步消息末中断，所以要读出该小帧内的所有消息
			if(dev_info->BC_map_IntBlkAddr_msgAddr.find(IntBlkAddr) 
					!= dev_info->BC_map_IntBlkAddr_msgAddr.end())
			{	
				vector<pair<WORD,WORD> > vec_msgNo_msgAddr = dev_info->BC_map_IntBlkAddr_msgAddr[IntBlkAddr];
				for(vector<pair<WORD,WORD> >::iterator it = vec_msgNo_msgAddr.begin();
				    it != vec_msgNo_msgAddr.end();
				    it++)
				{
				    //printf("IntBlkAddr:%d\n",IntBlkAddr);
					WORD msgNo = it->first;
					WORD msgAddr = it->second;
					AT_COMMAND_BLOCK blk;
					AtReadBlock( DevNum, msgAddr, (WORD*)&blk, 8 );
					//printf("------->len:%d<-------\n",blk.CmWORD1.Command.DataNum);
					WORD data[32];
					AtReadBlock( DevNum, blk.DataPtr, data, 32 );
					//printf("data:0x%x\n",data[0]);
					dev_info->BC_map_msgNo_blkQueue[msgNo].push(make_pair(blk, AT_DATA_BLK(data)));				
				}
			}
			hasMsg = true;
		}
		dev_info->IPCORE_wIntPtr += 2;
		if( dev_info->IPCORE_wIntPtr >= (AT_RAM_SIZE - 1) )
		{
			dev_info->IPCORE_wIntPtr = AT_RAM_SIZE - 32;
		}
	}
	
	//发送队列中的异步消息
	map<WORD, queue<AT_DATA_BLK> >::iterator it;
	for(it = dev_info->BC_map_AsyncMsgNo_dataQueue.begin();
		it != dev_info->BC_map_AsyncMsgNo_dataQueue.end();
		it++)
	{
		WORD addr = dev_info->BC_map_msgNo_ifCallBlkAddr[it->first];
		AT_MSG_CONTROL ctrlWrd;//跳转块控制字
		ctrlWrd.ControlWord = AtReadRam( DevNum, addr );
		
		queue<AT_DATA_BLK>* que = &(it->second);
		if(!que->empty() && (ctrlWrd.Control.Retry == 3)){
			//更新数据RAM
			WORD dataAddr = dev_info->BC_map_dataNo_dataAddr[it->first];
			AT_DATA_BLK dataBlk = que->front();
			//更新时间码
			SetTimecodeForAsync(it->first, dataBlk.data);

			AtWriteBlock( DevNum, dataAddr, dataBlk.data, dataBlk.dataLen);
			//调整消息的数据字数
			//取得异步消息的相对块地址
			WORD msgBlkAddr = AtReadRam( DevNum, addr+6 );
			//修改命令字1
			AT_MSG_COMMAND cmd1;
			cmd1.CommanWORD = AtReadRam( DevNum, msgBlkAddr+1 );
			cmd1.Command.DataNum = dataBlk.dataLen;
			AtWriteRam( DevNum, msgBlkAddr+1, cmd1.CommanWORD );
			//修改命令字2
			AT_MSG_CONTROL msgCtrlWrd;//控制字
			msgCtrlWrd.ControlWord = AtReadRam( DevNum, msgBlkAddr );
			if(msgCtrlWrd.Control.RT_RT){
				AT_MSG_COMMAND cmd2;
				cmd2.CommanWORD = AtReadRam( DevNum, msgBlkAddr+2 );
				cmd2.Command.DataNum = dataBlk.dataLen;
				AtWriteRam( DevNum, msgBlkAddr+2, cmd2.CommanWORD );	
			}
			//设置标志位为发出
			ctrlWrd.Control.Retry = 2;
			AtWriteRam( DevNum, addr, ctrlWrd.ControlWord );
			que->pop();
		}
	}

	return hasMsg;
}


/**
* 设置命令块CmdBlk里的控制字
* \param[out] CmdBlk 要设置控制字的命令块。
* \param[in]  dwMsgOptions 关联消息的属性。
* \param[in]  isRT_RT 0:不是RT到RT消息；1：是RT到RT消息。
* \return 操作是否成功。
*/
int AtBCMakeControl(AT_COMMAND_BLOCK &CmdBlk, 
					 U32BIT dwMsgOptions,
					 WORD isRT_RT)
{	
	//消息命令块
	memset( &CmdBlk, 0, sizeof(AT_COMMAND_BLOCK) );
	CmdBlk.CtrlWord.Control.OP_Code = OP_CODE_EXEC;//0111-Retry on Condition OP_CODE_ROC
	if(dwMsgOptions & ACE_BCCTRL_RETRY_ENA){
		CmdBlk.CtrlWord.Control.OP_Code = OP_CODE_ROC;
		CmdBlk.CtrlWord.Control.Retry = 2;
	}
	CmdBlk.CtrlWord.Control.Channel_AorB = 0;
	if(dwMsgOptions & ACE_BCCTRL_CHL_A){
		CmdBlk.CtrlWord.Control.Channel_AorB = 1;
	}
	CmdBlk.CtrlWord.Control.RT_RT = isRT_RT;
	CmdBlk.CtrlWord.Control.Condition_Code = 0x60;
	return TRUE;
}

/**
* 功能: 定义消息，将消息的命令块写入结构体
* 描述: 每条传输的消息自动添加Opcode为0001消息（间隔时间）
* 参数: 
* 	Bus-通道号
* 	MsgNo-消息序号
* 	DataNo-数据块号
* 	wMsgGapTime-间隔时间，单位：us
* 	MsgBlk-消息块
* 返回: 
* 注意: 
*/
int AtBCDefMsg( WORD Bus, 
				SHORT MsgNo, 
				SHORT DataNo, 
				WORD wMsgGapTime, 
				AT_COMMAND_BLOCK* MsgBlk )
{
	AT_COMMAND_BLOCK MsgBlkAdd;
	memset(&MsgBlkAdd, 0, sizeof(MsgBlkAdd));
	if( MsgBlk == NULL )
		return FALSE;

	if (map_devInfo.find(Bus) == map_devInfo.end()){
		return FALSE;
	}
	CDeviceInfo* dev_info = map_devInfo[Bus];

	//消息命令块
	MsgBlk->DataPtr = DataNo; //先将数据块号存入数据指针中，写RAM时再换成真正的数据块地址
	dev_info->BC_vec_CmdBlk.push_back(*MsgBlk);
	WORD count = dev_info->BC_vec_CmdBlk.size();
	dev_info->BC_map_msgNo_cmdIndex.insert(make_pair(MsgNo, count-1));
	//附加的时间间隔消息命令块
	MsgBlkAdd.CtrlWord.Control.OP_Code = OP_CODE_SKIP;//skip
	MsgBlkAdd.CtrlWord.Control.Retry = 0;
	//	MsgBlkAdd.CtrlWord.Control.Channel_AorB = 1;
	//	MsgBlkAdd.CtrlWord.Control.RT_RT = 0;
	//	MsgBlkAdd.TimerVal = 0x2D00*2;//16M时钟的计数(1600us)
	MsgBlkAdd.TimerVal = wMsgGapTime * 20;

	dev_info->BC_vec_CmdBlk.push_back(MsgBlkAdd);
	return TRUE;
}

/**
* 功能: 定义消息，将消息的命令块写入结构体
* 描述: 每条传输的消息自动添加Opcode为0001和1100的消息（间隔时间和调用返回）
* 参数: 
* 	Bus-通道号
* 	MsgNo-消息序号
* 	DataNo-数据块号
* 	wMsgGapTime-间隔时间
* 	MsgBlk-消息块
* 返回: 
* 注意: 
*/
int AtBCDefAsyncMsg( WORD Bus, 
					 SHORT MsgNo, 
					 SHORT DataNo, 
					 WORD wMsgGapTime, 
					 AT_COMMAND_BLOCK* MsgBlk )
{
	AT_COMMAND_BLOCK MsgBlkAdd;
	if( MsgBlk == NULL )
		return FALSE;

	if (map_devInfo.find(Bus) == map_devInfo.end()){
		return FALSE;
	}
	CDeviceInfo* dev_info = map_devInfo[Bus];

	//消息命令块
	MsgBlk->DataPtr = DataNo; //先将数据块号存入数据指针中，写RAM时再换成真正的数据块地址
	dev_info->BC_vec_CmdBlk.push_back(*MsgBlk);
	WORD count = dev_info->BC_vec_CmdBlk.size();
	dev_info->BC_map_AsyncMsgNo_cmdIndex.insert(make_pair(MsgNo, count-1));

	//附加的时间间隔消息命令块。Channel_AorB、RT_RT的值不影响效果luxq
	memset(&MsgBlkAdd, 0, sizeof(AT_COMMAND_BLOCK));
	MsgBlkAdd.CtrlWord.Control.OP_Code = OP_CODE_SKIP;//skip
	MsgBlkAdd.CtrlWord.Control.Retry = 0;
	//	MsgBlkAdd.TimerVal = 0x2D00*2;//16M时钟的计数(1600us)
	MsgBlkAdd.TimerVal = wMsgGapTime * 20;//16M时钟的计数(1600us)
	dev_info->BC_vec_CmdBlk.push_back(MsgBlkAdd);

	//附加的调用返回命令块。Channel_AorB、RT_RT的值不影响效果luxq
	memset(&MsgBlkAdd, 0, sizeof(AT_COMMAND_BLOCK));
	MsgBlkAdd.CtrlWord.Control.OP_Code = OP_CODE_GOTO;//0010 
	MsgBlkAdd.CtrlWord.Control.Retry = 0;
	dev_info->BC_vec_CmdBlk.push_back(MsgBlkAdd);

	return TRUE;
}

SHORT aceBCDataBlkCreate(SHORT DevNum,
SHORT nDataBlkID,
WORD wDataBlkSize,
WORD *pBuffer,
WORD wBufferSize)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	dev_info->BC_map_dataNo_data.insert(make_pair(nDataBlkID, AT_DATA_BLK(pBuffer)));
	return 0;
}

SHORT aceBCMsgCreateBCtoRT (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wSA,
WORD wWC,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRT;
	CmdBlk.CmWORD1.Command.SubAddr = wSA;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;//RT接收
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCMsgCreateRTtoBC (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wSA,
WORD wWC,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRT;
	CmdBlk.CmWORD1.Command.SubAddr = wSA;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 1;//RT发送
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCMsgCreateRTtoRT(SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRTRx,
WORD wSARx,
WORD wWC,
WORD wRTTx,
WORD wSATx,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 1);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRTRx;
	CmdBlk.CmWORD1.Command.SubAddr = wSARx;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;//RT接收
	//Command Word 2
	CmdBlk.CmWORD2.Command.RTAddr = wRTTx;
	CmdBlk.CmWORD2.Command.SubAddr = wSATx;
	CmdBlk.CmWORD2.Command.DataNum = wWC;
	CmdBlk.CmWORD2.Command.TRFlag = 1;//RT发送
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCMsgCreateMode(SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wTR,
WORD wModeCmd,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRT;
	CmdBlk.CmWORD1.Command.SubAddr = 0;
	CmdBlk.CmWORD1.Command.DataNum = wModeCmd;
	CmdBlk.CmWORD1.Command.TRFlag = wTR;
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCMsgCreateBcst(SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wSA,
WORD wWC,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word
	CmdBlk.CmWORD1.Command.RTAddr = 31;
	CmdBlk.CmWORD1.Command.SubAddr = wSA;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCMsgCreateBcstRTtoRT (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wSARx,
WORD wWC,
WORD wRTTx,
WORD wSATx,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 1);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = 31;
	CmdBlk.CmWORD1.Command.SubAddr = wSARx;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;//RTs接收
	//Command Word 2
	CmdBlk.CmWORD2.Command.RTAddr = wRTTx;
	CmdBlk.CmWORD2.Command.SubAddr = wSATx;
	CmdBlk.CmWORD2.Command.DataNum = wWC;
	CmdBlk.CmWORD2.Command.TRFlag = 1;//发送
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCMsgCreateBcstMode (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wTR,
WORD wModeCmd,
WORD wMsgGapTime,
U32BIT dwMsgOptions)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = 31;
	CmdBlk.CmWORD1.Command.SubAddr = 0;
	CmdBlk.CmWORD1.Command.DataNum = wModeCmd;
	CmdBlk.CmWORD1.Command.TRFlag = wTR;
	
	AtBCDefMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateBCtoRT (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wSA,
WORD wWC,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRT;
	CmdBlk.CmWORD1.Command.SubAddr = wSA;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;//RT接收
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateRTtoBC (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wSA,
WORD wWC,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRT;
	CmdBlk.CmWORD1.Command.SubAddr = wSA;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 1;//RT发送
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateRTtoRT (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRTRx,
WORD wSARx,
WORD wWC,
WORD wRTTx,
WORD wSATx,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 1);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRTRx;
	CmdBlk.CmWORD1.Command.SubAddr = wSARx;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;//RT接收
	//Command Word 2
	CmdBlk.CmWORD2.Command.RTAddr = wRTTx;
	CmdBlk.CmWORD2.Command.SubAddr = wSATx;
	CmdBlk.CmWORD2.Command.DataNum = wWC;
	CmdBlk.CmWORD2.Command.TRFlag = 1;//RT发送
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateMode(SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wTR,
WORD wModeCmd,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = wRT;
	CmdBlk.CmWORD1.Command.SubAddr = 0;
	CmdBlk.CmWORD1.Command.DataNum = wModeCmd;
	CmdBlk.CmWORD1.Command.TRFlag = wTR;
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateBcst (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wSA,
WORD wWC,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word
	CmdBlk.CmWORD1.Command.RTAddr = 31;
	CmdBlk.CmWORD1.Command.SubAddr = wSA;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateBcstRTtoRT (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wSARx,
WORD wWC,
WORD wRTTx,
WORD wSATx,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 1);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = 31;
	CmdBlk.CmWORD1.Command.SubAddr = wSARx;
	CmdBlk.CmWORD1.Command.DataNum = wWC;
	CmdBlk.CmWORD1.Command.TRFlag = 0;//RTs接收
	//Command Word 2
	CmdBlk.CmWORD2.Command.RTAddr = wRTTx;
	CmdBlk.CmWORD2.Command.SubAddr = wSATx;
	CmdBlk.CmWORD2.Command.DataNum = wWC;
	CmdBlk.CmWORD2.Command.TRFlag = 1;//发送
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT aceBCAsyncMsgCreateBcstMode (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wTR,
WORD wModeCmd,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD *pBuffer)
{
	AT_COMMAND_BLOCK CmdBlk;
	AtBCMakeControl(CmdBlk, dwMsgOptions, 0);
	
	//Command Word 1
	CmdBlk.CmWORD1.Command.RTAddr = 31;
	CmdBlk.CmWORD1.Command.SubAddr = 0;
	CmdBlk.CmWORD1.Command.DataNum = wModeCmd;
	CmdBlk.CmWORD1.Command.TRFlag = wTR;
	
	aceBCDataBlkCreate(DevNum, nDataBlkID, 32, pBuffer, 32);
	AtBCDefAsyncMsg( DevNum, nMsgBlkID, nDataBlkID, wMsgGapTime, &CmdBlk);
	return 0;
}

SHORT  aceBCMsgModify(  SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID1,
WORD wBCCtrlWrd1,
WORD wCmdWrd1_1,
WORD wCmdWrd1_2,
WORD wMsgGapTime1,
SHORT nDataBlkID2,
WORD wBCCtrlWrd2,
WORD wCmdWrd2_1,
WORD wCmdWrd2_2,
WORD wMsgGapTime2,
WORD wModFlags)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if( ACE_BC_MOD_CMDWRD1_1 == wModFlags ){
		//如果还没有开始BC，即未组帧，则修改本地内存里的消息
		if(!dev_info->is_IPCORE_start)
		{
			WORD index;
			if(dev_info->BC_map_msgNo_cmdIndex.find(nMsgBlkID) 
					!= dev_info->BC_map_msgNo_cmdIndex.end())
			{
				index = dev_info->BC_map_msgNo_cmdIndex[nMsgBlkID];
			}
			else if(dev_info->BC_map_AsyncMsgNo_cmdIndex.find(nMsgBlkID) 
					!= dev_info->BC_map_AsyncMsgNo_cmdIndex.end())
			{
				index = dev_info->BC_map_AsyncMsgNo_cmdIndex[nMsgBlkID];
			}
			else{
				return -1;
			}
			dev_info->BC_vec_CmdBlk[index].CmWORD1.CommanWORD = wCmdWrd1_1;
		}
		//如果还已经开始BC，即组帧后，则修改RAM里的消息
		else
		{
			for(set<pair<WORD, WORD> >::iterator it = dev_info->BC_set_msgNo_blkAddr.begin(); 
				it != dev_info->BC_set_msgNo_blkAddr.end(); 
				it++)
			{
				if(it->first == nMsgBlkID)
				{
					WORD addr = it->second;
					AtWriteRam( DevNum, addr + 1, wCmdWrd1_1 );
				}
			}
		}
	}//end of if( ACE_BC_MOD_CMDWRD1_1 == wModFlags )
	//修改控制字总是在开始BC后使用
	else if( ACE_BC_MOD_BCCTRL1 == wModFlags )
	{
		for(set<pair<WORD, WORD> >::iterator it = dev_info->BC_set_msgNo_blkAddr.begin(); 
			it != dev_info->BC_set_msgNo_blkAddr.end(); 
			it++)
		{
			if(it->first == nMsgBlkID)
			{
				WORD addr = it->second;
				AT_MSG_CONTROL ctrlWrd;
				ctrlWrd.ControlWord = AtReadRam( DevNum, addr );
				
				ctrlWrd.Control.Retry = 0;
				if(wBCCtrlWrd1 & ACE_BCCTRL_RETRY_ENA){
					ctrlWrd.Control.Retry = 2;
				}
				ctrlWrd.Control.Channel_AorB = 0;
				if(wBCCtrlWrd1 & ACE_BCCTRL_CHL_A){
					ctrlWrd.Control.Channel_AorB = 1;
				}
		
				AtWriteRam( DevNum, addr, ctrlWrd.ControlWord );
			}
		}
	}
	else
	{
		return -1;
	}
	return 0;
}
/**
 * 只处理wModFlags为ACE_BC_MOD_BCCTRL1的情况。
 */
SHORT aceBCMsgModifyRTtoRT (SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRTRx,
WORD wSARx,
WORD wWC,
WORD wRTTx,
WORD wSATx,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD wModFlags)
{
	if( wModFlags != ACE_BC_MOD_BCCTRL1 )
	{
		return -1;
	}

	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	for(set<pair<WORD, WORD> >::iterator it = dev_info->BC_set_msgNo_blkAddr.begin(); 
		it != dev_info->BC_set_msgNo_blkAddr.end(); 
		it++)
	{
		if(it->first == nMsgBlkID)
		{
			WORD addr = it->second;
			AT_MSG_CONTROL ctrlWrd;
			ctrlWrd.ControlWord = AtReadRam( DevNum, addr );
			
			ctrlWrd.Control.Retry = 0;
			if(dwMsgOptions & ACE_BCCTRL_RETRY_ENA)
			{
				ctrlWrd.Control.Retry = 2;
			}
			ctrlWrd.Control.Channel_AorB = 0;
			if(dwMsgOptions & ACE_BCCTRL_CHL_A)
			{
				ctrlWrd.Control.Channel_AorB = 1;
			}
		
			AtWriteRam( DevNum, addr, ctrlWrd.ControlWord );		
		}
	}
	return 0;
}

SHORT aceBCMsgModifyBcstMode(SHORT DevNum, 
		SHORT nMsgBlkID, 
		SHORT nDataBlkID, 
		USHORT wTR, 
		USHORT wModeCmd, 
		USHORT wMsgGapTime, 
		U32BIT dwMsgOptions, 
		USHORT wModFlags){
	if( wModFlags != ACE_BC_MOD_BCCTRL1 )
	{
		return -1;
	}

	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	for(set<pair<WORD, WORD> >::iterator it = dev_info->BC_set_msgNo_blkAddr.begin(); 
		it != dev_info->BC_set_msgNo_blkAddr.end(); 
		it++)
	{
		if(it->first == nMsgBlkID)
		{
			WORD addr = it->second;
			AT_MSG_CONTROL ctrlWrd;
			ctrlWrd.ControlWord = AtReadRam( DevNum, addr );
			
			ctrlWrd.Control.Retry = 0;
			if(dwMsgOptions & ACE_BCCTRL_RETRY_ENA)
			{
				ctrlWrd.Control.Retry = 2;
			}
			ctrlWrd.Control.Channel_AorB = 0;
			if(dwMsgOptions & ACE_BCCTRL_CHL_A)
			{
				ctrlWrd.Control.Channel_AorB = 1;
			}

			AtWriteRam( DevNum, addr, ctrlWrd.ControlWord );		
		}
	}
	return 0;	
}

SHORT aceBCMsgModifyMode(SHORT DevNum,
SHORT nMsgBlkID,
SHORT nDataBlkID,
WORD wRT,
WORD wTR,
WORD wModeCmd,
WORD wMsgGapTime,
U32BIT dwMsgOptions,
WORD wModFlags)
{
	if( wModFlags != ACE_BC_MOD_BCCTRL1 )
	{
		return -1;
	}

	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	for(set<pair<WORD, WORD> >::iterator it = dev_info->BC_set_msgNo_blkAddr.begin(); 
		it != dev_info->BC_set_msgNo_blkAddr.end(); 
		it++)
	{
		if(it->first == nMsgBlkID)
		{
			WORD addr = it->second;
			AT_MSG_CONTROL ctrlWrd;
			ctrlWrd.ControlWord = AtReadRam( DevNum, addr );
			
			ctrlWrd.Control.Retry = 0;
			if(dwMsgOptions & ACE_BCCTRL_RETRY_ENA)
			{
				ctrlWrd.Control.Retry = 2;
			}
			ctrlWrd.Control.Channel_AorB = 0;
			if(dwMsgOptions & ACE_BCCTRL_CHL_A)
			{
				ctrlWrd.Control.Channel_AorB = 1;
			}

			AtWriteRam( DevNum, addr, ctrlWrd.ControlWord );		
		}
	}
	return 0;
}

SHORT aceBCDataBlkWrite(SHORT DevNum,
SHORT nDataBlkID,
WORD *pBuffer,
WORD wBufferSize,
WORD wOffset)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if(dev_info->BC_map_dataNo_dataAddr.find(nDataBlkID) 
		== dev_info->BC_map_dataNo_dataAddr.end())
	{
		dev_info->BC_map_dataNo_data[nDataBlkID] = AT_DATA_BLK(pBuffer);
		return -1;
	}
	
	WORD addr = dev_info->BC_map_dataNo_dataAddr[nDataBlkID];
	AtWriteBlock( DevNum, addr, pBuffer, wBufferSize);
	return 0;
}

/**
 * 只处理消息和小帧
 */ 
SHORT aceBCOpCodeCreate ( S16BIT DevNum, S16BIT nOpCodeID, U16BIT wOpCodeType, U16BIT wCondition, U32BIT dwParameter1,
		U32BIT dwParameter2, U32BIT dwReserved)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if(wOpCodeType == ACE_OPCODE_XEQ){//消息
		dev_info->BC_map_msgCode_msgNo.insert(make_pair(nOpCodeID, dwParameter1));
	}
	else if(wOpCodeType == ACE_OPCODE_CAL){  //小帧
		dev_info->BC_map_minFraCode_minFraID.insert(make_pair(nOpCodeID, dwParameter1));
	}
	return 0;
}

SHORT aceBCFrameCreate(SHORT DevNum,
SHORT nFrameID,
WORD wFrameType,
SHORT aOpCodeIDs[],
WORD wOpCodeCount,
WORD wMnrFrmTime,
WORD wFlags)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if(wFrameType == ACE_FRAME_MINOR){
		vector<WORD> vec;
		for(int i=0; i<wOpCodeCount; i++){
			vec.push_back(aOpCodeIDs[i]);
		}
		dev_info->BC_map_minFraID_msgCodes.insert(make_pair(nFrameID, vec));
		dev_info->BC_map_minFraID_minFraTime.insert(make_pair(nFrameID, wMnrFrmTime));
	}
	else if(wFrameType == ACE_FRAME_MAJOR){
		WORD minFraCount = wOpCodeCount;
		
		/*
		 * 检查RAM是否够用
		 * 
		 * 各小帧:调用块(B)+中断块（A）
		 * 返回第一个命令块的goto块(2)
		 *
		 *
		 * 每一个异步消息goto（2）块    //goto到每一个异步消息的消息块
		 * 返回块（C）     //返回各小帧调用块
		 *
		 * 每一个异步消息的:消息块(4)、延时块(1)、中断块(A)、goto块(2)    //goto到中断块
		 *
		 * 每一个小帧
		 *  小帧延时块(E)
		 *  每一个同步消息的:消息块(4)和延时块(1)
		 *  异步消息goto块（2） 
         * ...
		 * 数据块
		 * 中断列表
		 */
		WORD countSyncMsg = 0;
		for(int i=0; i<minFraCount; i++){
			WORD minFraCode = aOpCodeIDs[i];
			WORD minFraID = dev_info->BC_map_minFraCode_minFraID[minFraCode];
			countSyncMsg += dev_info->BC_map_minFraID_msgCodes[minFraID].size();
		}
		WORD countAsycMsg = dev_info->BC_map_AsyncMsgNo_cmdIndex.size();
		WORD len = (2 + 4 * minFraCount + 2 * countSyncMsg + 5 * countAsycMsg) * 8
		+ (countSyncMsg + countAsycMsg) * 32 + 32;
		if(len > AT_RAM_SIZE){
			printf("[ERROR]: too many frame or message!!");
			return -1;
		}
		
		//如果RAM够用，开始写RAM
		AT_COMMAND_BLOCK blk;
		memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
		WORD dataAddr = AT_RAM_SIZE - 32 - 32;  //当前可写消息位置,最后32个字被中断表占了

		//返回第一个命令块的goto块
		//所有call minorN执行完后要回到第一个call minorN重新执行
		memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
		blk.CtrlWord.Control.OP_Code = OP_CODE_GOTO;
		blk.BranchAddr = 0;
		AtWriteBlock( DevNum, 2*minFraCount*8, (WORD*)&blk, SIZEOFCMDBLK); 

		WORD AsyMsgCount = dev_info->BC_map_AsyncMsgNo_cmdIndex.size();
		//当前异步消息块地址
		WORD AsyMsgGotoAddress = (2*minFraCount + 1) * 8;
		WORD AsyMsgAddress = (2*minFraCount + 1 + AsyMsgCount + 1) * 8;
		map<WORD, WORD>::iterator it = dev_info->BC_map_AsyncMsgNo_cmdIndex.begin();
		//每一个异步消息goto（0010）块 
		//每一个异步消息的消息块、延时块、中断块、goto块()
		for(; it!=dev_info->BC_map_AsyncMsgNo_cmdIndex.end(); it++){
			//写入 if call AsyncMsg 块  goto
			blk.CtrlWord.Control.OP_Code = OP_CODE_GOTO;//0010
			blk.CtrlWord.Control.Retry = 3;
			blk.BranchAddr = AsyMsgAddress;
			AtWriteBlock( DevNum, AsyMsgGotoAddress, (WORD*)&blk, SIZEOFCMDBLK); 
			dev_info->BC_map_msgNo_ifCallBlkAddr.insert(make_pair (it->first, AsyMsgGotoAddress));
			AsyMsgGotoAddress += 8;

			WORD AsyMsgIndex = it->second;

			//写入消息块
			blk = dev_info->BC_vec_CmdBlk[AsyMsgIndex];
			WORD dataNo = blk.DataPtr;
			blk.DataPtr = dataAddr;
			AtWriteBlock( DevNum, AsyMsgAddress, (WORD*)&blk, SIZEOFCMDBLK); 
			dev_info->BC_set_msgNo_blkAddr.insert(make_pair(it->first, AsyMsgAddress));
			//vec_msgAddr.push_back(make_pair(it->first, minAddr));
			AsyMsgAddress += 8;

			//写入消息延时块
			blk = dev_info->BC_vec_CmdBlk[AsyMsgIndex+1];
			AtWriteBlock( DevNum, AsyMsgAddress, (WORD*)&blk, SIZEOFCMDBLK); 
			AsyMsgAddress += 8;

			//写入中断块,每个异步消息后面都加入一个中断块
			memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
			blk.CtrlWord.Control.OP_Code = OP_CODE_INTC;
			AtWriteBlock( DevNum, AsyMsgAddress, (WORD*)&blk, SIZEOFCMDBLK); 
			vector<pair<WORD,WORD> > async_vec_msgAddr;
			async_vec_msgAddr.push_back(make_pair(it->first, AsyMsgAddress-16));
			dev_info->BC_map_IntBlkAddr_msgAddr.insert(make_pair(AsyMsgAddress, async_vec_msgAddr));	
			AsyMsgAddress += 8;		

			//goto块
			blk = dev_info->BC_vec_CmdBlk[AsyMsgIndex+2];
			blk.BranchAddr = AsyMsgGotoAddress;
			AtWriteBlock( DevNum, AsyMsgAddress, (WORD*)&blk, SIZEOFCMDBLK); 
			AsyMsgAddress += 8;

			//写入该消息数据块
			AtWriteBlock( DevNum, dataAddr, dev_info->BC_map_dataNo_data[dataNo].data, 32); 
			dev_info->BC_map_dataNo_dataAddr.insert(make_pair(dataNo, dataAddr));
			dataAddr -= 32;

			//初始化BC_map_AsyncMsgNo_dataQueue
			dev_info->BC_map_AsyncMsgNo_dataQueue.insert(make_pair(it->first, queue<AT_DATA_BLK>()));
		}
		//返回块（1100）C
		memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
		blk.CtrlWord.Control.OP_Code = OP_CODE_RTCALL;
		AtWriteBlock( DevNum, AsyMsgGotoAddress, (WORD*)&blk, SIZEOFCMDBLK); 
		
		//各小帧调用块
		//每一个小帧
		//  小帧延时块
		//	每一个同步消息的消息块和延时块
		//	中断块（1010）A
		//	异步消息goto块（0010）
		WORD minAddr = AsyMsgAddress;  //当前可写小帧里的块地址，前面放了call minor命令块和 goto 0 命令块 
		for(int i=0; i<minFraCount; i++){
			WORD minFraCode = aOpCodeIDs[i];
			WORD minFraID = dev_info->BC_map_minFraCode_minFraID[minFraCode];
			vector<pair<WORD,WORD> > vec_msgAddr;

			//写入call minorN
			blk.CtrlWord.Control.OP_Code = OP_CODE_CALL;//1011
			blk.BranchAddr = minAddr;
			AtWriteBlock( DevNum, 2*i*8, (WORD*)&blk, SIZEOFCMDBLK); 

			//写入minorN
			//小帧延时块
			memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
			blk.CtrlWord.Control.OP_Code = OP_CODE_LMFT;//1110
			blk.TimerVal = (WORD)(dev_info->BC_map_minFraID_minFraTime[minFraID] * 15.625);
			AtWriteBlock( DevNum, minAddr, (WORD*)&blk, SIZEOFCMDBLK); 
			minAddr += 8;

			//写入minorN的消息
			WORD msgCount = dev_info->BC_map_minFraID_msgCodes[minFraID].size();
			for(int j=0; j<msgCount; j++){
				WORD msgCode = dev_info->BC_map_minFraID_msgCodes[minFraID][j];
				WORD msgNo = dev_info->BC_map_msgCode_msgNo[msgCode];
				WORD msgIndex = dev_info->BC_map_msgNo_cmdIndex[msgNo];
				//写入消息块
				blk = dev_info->BC_vec_CmdBlk[msgIndex];
				WORD dataNo = blk.DataPtr;
				if(dev_info->BC_map_dataNo_dataAddr.find(dataNo) == dev_info->BC_map_dataNo_dataAddr.end()){
					blk.DataPtr = dataAddr;
				}
				else{ //同一个消息使用同一个数据块地址
					blk.DataPtr = dev_info->BC_map_dataNo_dataAddr[dataNo];
				}
				AtWriteBlock( DevNum, minAddr, (WORD*)&blk, SIZEOFCMDBLK); 
				dev_info->BC_set_msgNo_blkAddr.insert(make_pair(msgNo, minAddr));
				vec_msgAddr.push_back(make_pair(msgNo, minAddr));
				minAddr += 8;

				//写入消息延时块
				blk = dev_info->BC_vec_CmdBlk[msgIndex+1];
				AtWriteBlock( DevNum, minAddr, (WORD*)&blk, SIZEOFCMDBLK); 
				minAddr += 8;

				//写入该消息数据块
				if(dev_info->BC_map_dataNo_dataAddr.find(dataNo) == dev_info->BC_map_dataNo_dataAddr.end()){
					AtWriteBlock( DevNum, dataAddr, dev_info->BC_map_dataNo_data[dataNo].data, 32); 
					dev_info->BC_map_dataNo_dataAddr.insert(make_pair(dataNo, dataAddr));
					dataAddr -= 32;
				}
			}

			//小帧末加入中断块
			memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
			blk.CtrlWord.Control.OP_Code = OP_CODE_INTC;
			AtWriteBlock( DevNum, (2*i+1)*8, (WORD*)&blk, SIZEOFCMDBLK); 
			dev_info->BC_map_IntBlkAddr_msgAddr.insert(make_pair((2*i+1)*8, vec_msgAddr));
//			minAddr += 8;

			//异步消息goto块
			memset( &blk, 0, sizeof(AT_COMMAND_BLOCK) );
			blk.CtrlWord.Control.OP_Code = OP_CODE_GOTO;
			blk.BranchAddr = (2*minFraCount + 1) * 8;
			AtWriteBlock( DevNum, minAddr, (WORD*)&blk, SIZEOFCMDBLK); 
			minAddr += 8;
		}
	}
	return 0;
}

/**
 * 对于读出的pMsg，只用到了其状态字和数据字
 */
S16BIT aceBCGetMsgFromIDDecoded(S16BIT DevNum, 
		S16BIT nMsgBlkID, 
		MSGSTRUCT *pMsg, 
		U16BIT bPurge)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	queue<pair<AT_COMMAND_BLOCK,AT_DATA_BLK> >* que = &dev_info->BC_map_msgNo_blkQueue[nMsgBlkID];
	if(que->empty()){
	   //printf("the que is empty\n");
		return 0;
	}
	pair<AT_COMMAND_BLOCK,AT_DATA_BLK> temp = que->front();
	AT_COMMAND_BLOCK cmdBlk = temp.first;
	AT_DATA_BLK dataBlk = temp.second;
	pMsg->wStsWrd1 = cmdBlk.StatWord1.StatusWord;
	pMsg->wStsWrd1Flg = 1;
	pMsg->wWordCount = cmdBlk.CmWORD1.Command.DataNum;
	if(!(cmdBlk.CmWORD1.Command.TRFlag == 1 && cmdBlk.StatWord1.StatusWord == 0)) { //RT send
		for(int i=0; i<32; i++){
			pMsg->aDataWrds[i] = dataBlk.data[i];
		}
	}
	if(bPurge){
		que->pop();
	}
	
	return 1;
}

S16BIT aceBCGetMsg(S16BIT DevNum, 
		S16BIT nMsgBlkID, 
		U16BIT bPurge)
{
   if(NULL == g_pfucCallback)
   	{
			return -1;
	}
	 
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	queue<pair<AT_COMMAND_BLOCK,AT_DATA_BLK> >* que = &dev_info->BC_map_msgNo_blkQueue[nMsgBlkID];
	if(que->empty()){
		return 0;
	}
	pair<AT_COMMAND_BLOCK,AT_DATA_BLK> temp = que->front();
	AT_COMMAND_BLOCK cmdBlk = temp.first;
	AT_DATA_BLK dataBlk = temp.second;
	if(bPurge){
		que->pop();
	}

	 if(NULL != g_pfucCallback)
    {
         stmsg_struct CallMsg;
		  U16BIT datalen = 0;
         int i =0;
		  if(cmdBlk.CmWORD1.CommanWORD != 0)
		  {
			    CallMsg.cmdwrd  = cmdBlk.CmWORD1.CommanWORD;
			    CallMsg.datalen   = cmdBlk.CmWORD1.Command.DataNum;
			    CallMsg.iaddrs     = cmdBlk.CmWORD1.Command.RTAddr;
			    CallMsg.subaddr  = cmdBlk.CmWORD1.Command.SubAddr;
			    CallMsg.trflag      = cmdBlk.CmWORD1.Command.TRFlag;
			    CallMsg.stswrd    = cmdBlk.StatWord1.StatusWord;
				 datalen = CallMsg.datalen;
				 if((CallMsg.subaddr != 31) && (CallMsg.subaddr != 0))
				 {
                   if(CallMsg.datalen == 0)
                   {
                        CallMsg.datalen = 32;
							datalen = 32;
					  }
				 }
				 else
				 {
						if((datalen == 0x10) || (datalen == 0x11) || (datalen == 0x12) || (datalen == 0x13) || (datalen == 0x14) ||(datalen == 0x15))
						{
							  datalen = 1;
						}
						else
						{
                         datalen = 0;
						}
				 }
				 
			    for(i = 0; i< datalen; i++)
			    {
	               CallMsg.datawrd[i] =  dataBlk.data[i];
			    }
				  g_pfucCallback(&CallMsg);
			}  
	 }
	 
	return 1;
}

/*
 * 发送异步消息,放入队列中，等待中断处理函数发送
 * Adapter代码中，MsgID和DataID总是相等
 */
SHORT aceBCSendAsyncMsgHP(SHORT DevNum,
WORD nMsgID,
WORD *pData,
WORD nDataLen,
WORD wTimeFactor)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if(dev_info->BC_map_msgNo_ifCallBlkAddr.find(nMsgID) 
		== dev_info->BC_map_msgNo_ifCallBlkAddr.end())
	{
		return -1;
	}
	queue<AT_DATA_BLK>* que = &dev_info->BC_map_AsyncMsgNo_dataQueue[nMsgID];
	
	WORD addr = dev_info->BC_map_msgNo_ifCallBlkAddr[nMsgID];
	AT_MSG_CONTROL ctrlWrd;//跳转块控制字
	ctrlWrd.ControlWord = AtReadRam( DevNum, addr );
	//队列中无等待发送的消息，且发送标志位为已发送
	if (que->empty() && ctrlWrd.Control.Retry == 3)
	{
		//更新数据RAM
		WORD dataAddr = dev_info->BC_map_dataNo_dataAddr[nMsgID];
		//更新时间码
		SetTimecodeForAsync(nMsgID, pData);

		AtWriteBlock( DevNum, dataAddr, pData, nDataLen);
		//调整消息的数据字数
		//取得异步消息的相对块地址
		WORD msgBlkAddr = AtReadRam( DevNum, addr+6 );
		//修改命令字1
		AT_MSG_COMMAND cmd1;
		cmd1.CommanWORD = AtReadRam( DevNum, msgBlkAddr+1 );
		cmd1.Command.DataNum = nDataLen;
		AtWriteRam( DevNum, msgBlkAddr+1, cmd1.CommanWORD );
		//修改命令字2
		AT_MSG_CONTROL msgCtrlWrd;//控制字
		msgCtrlWrd.ControlWord = AtReadRam( DevNum, msgBlkAddr );
		if(msgCtrlWrd.Control.RT_RT){
			AT_MSG_COMMAND cmd2;
			cmd2.CommanWORD = AtReadRam( DevNum, msgBlkAddr+2 );
			cmd2.Command.DataNum = nDataLen;
			AtWriteRam( DevNum, msgBlkAddr+2, cmd2.CommanWORD );	
		}
		//设置标志位为发出
		ctrlWrd.Control.Retry = 2;
		AtWriteRam( DevNum, addr, ctrlWrd.ControlWord );
	} 
	else
	{
		//保存要发送的数据，在中断线程中处理
		que->push(AT_DATA_BLK(pData, nDataLen));
	}
	return 0;
}

/**
 * 只能建一个mainframe，并且不能指定执行次数。总是按lMjrFrmCount等于-1处理。
 */
SHORT aceBCStart(SHORT DevNum,
SHORT nMjrFrmID,
S32BIT lMjrFrmCount)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];
	
	WORD wRegVal;
	wRegVal = AtReadReg( DevNum, 0 );
	AtWriteReg( DevNum, 0, wRegVal | 0x8000 );
	dev_info->is_IPCORE_start = TRUE;
	return 0;
}

SHORT aceBCStop (SHORT DevNum)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return -1;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	WORD wRegVal;
	wRegVal = AtReadReg( DevNum, 0 );
	AtWriteReg( DevNum, 0, wRegVal & 0x7FFF );	
	dev_info->is_IPCORE_start = FALSE;
	return 0;
}

/**
 * aceFree的时候统一删除消息和数据块。
 */
SHORT aceBCMsgDelete(SHORT DevNum,
SHORT nMsgBlkID)
{
	return 0;
}

SHORT aceBCDataBlkDelete(SHORT DevNum,
SHORT nDataBlkID)
{
	return 0;
}

/**
 * 该函数不实现。重试策略固定为乒乓模式，重试次数固定为2次。
 */
SHORT aceBCSetMsgRetry(SHORT DevNum,
WORD wNumOfRetries,
WORD wFirstRetryBus,
WORD wSecondRetryBus,
WORD wReserved)
{
	return 0;
}

S16BIT aceBCConfigure(S16BIT DevNum, U32BIT dwOptions)
{
	return 0;
}

SHORT aceBCEmptyAsyncList (SHORT DevNum)
{
	return 0;
}

/**
 * 不处理GPF。
 */
S16BIT aceBCSetGPFState(S16BIT DevNum, U16BIT wGPF, U16BIT wStateChange)
{
	return -1;
}

S16BIT aceBCMsgGapTimerEnable(S16BIT DevNum, U16BIT bEnable)
{
	return 0;
}

S16BIT aceBCResetAsyncPtr(S16BIT DevNum)
{
	return 0;
}

