/*	
 * mt.cpp
 * 实现驱动MT功能的一些接口实现
 * \date 2013-6-5
 * \author xiaoqing.lu
 */
#include "zynq_1553b_api.h"
#include "IPCORE.h"
#include "head.h"
#include "common_qiu.h"

extern map<S16BIT, CDeviceInfo*> map_devInfo;
extern void g_1553B_myISR(UINT16 cardNum, UINT16 chnNum);
FILE *mt_msg_save;

void mt_save_the_message(MSGSTRUCT * pMsg)
{
      // 地址
	U16BIT wRT = 0;
	// 收发状态1
	U16BIT wTR1 = 0;
	// 子地址
	U16BIT subAddr = 0;
	// 数据字或矢量码
	U16BIT wWC = 0; 
	U16BIT wCmdWrd = 0;
	U16BIT wStsWrdFlg = 0;
	U16BIT wStsWrd;
	size_t   msg_len;
	int i = 0, cnt = 0;
    char tmp[50];
    char buf[1024];
    time_t timep;
    static string msg_type[] = {
          "BC-TO-RT", "RT-TO-BC", "RT-TO-RT", "MODE-NO-DATA", "MODE-DATA-RX",
			"MODE-DATA-TX", "BRDCST", "BRDCST-RT-TO-RT", "BRDCST-MODE-NO-DATA",
			"BRDCST-MODE-DATA"
		};

	if(NULL == mt_msg_save)
	{
	    printf("mt_msg_save is NULL\n");
		 return;
	}
	
	if(pMsg->wCmdWrd1Flg)
	{
	     wCmdWrd = pMsg->wCmdWrd1;
		 aceCmdWordParse(wCmdWrd, &wRT, &wTR1, &subAddr, &wWC);
	}
	else if(pMsg->wCmdWrd2Flg)
	{
	    wCmdWrd = pMsg->wCmdWrd2;
		 aceCmdWordParse(wCmdWrd, &wRT, &wTR1, &subAddr, &wWC);
	}

   time (&timep);
   memset(buf,  0,   sizeof(buf)); 
   memset(tmp, 0 , sizeof(tmp));

   sprintf(tmp, "%s",asctime(gmtime(&timep)));
	U16BIT str_len = strlen(tmp) -1;
   //tmp[(strlen(tmp) -1)] = '\0';
	memset((tmp + str_len), 0, (sizeof(tmp) - str_len));
   sprintf(buf, "%s                        msg type:%s\n",tmp,msg_type[pMsg->wType].c_str());

	if(pMsg->wStsWrd1Flg)
	{
		 wStsWrdFlg = 1;
		 wStsWrd = pMsg->wStsWrd1;
	}
	else if(pMsg->wStsWrd2Flg)
	{
		 wStsWrdFlg = 1;
		 wStsWrd = pMsg->wStsWrd2;
	}

	if(wStsWrdFlg)
	{
		 sprintf((buf + strlen(buf)), "CMD:0x%x       %02d-%c-%02d-%02d        state:0x%x\n",wCmdWrd, wRT, wTR1?'T':'R', subAddr, wWC, wStsWrd);
	}
	else
	{
        sprintf((buf + strlen(buf)), "CMD:0x%x                                      %02d-%c-%02d-%02d\n",wCmdWrd, wRT, wTR1?'T':'R', subAddr, wWC);
	}
  

    msg_len = pMsg->wWordCount;
    sprintf((buf + strlen(buf)), "%s","Data:");	
 	 for(i = 0; i < msg_len; i++)
 	 {
			sprintf((buf + strlen(buf)), "%04x    ",pMsg->aDataWrds[i]);
			cnt ++;
			if(!(cnt % 8))
			{
            	sprintf((buf + strlen(buf)), "\n     "); 
			}
 	 }
 
    sprintf((buf + strlen(buf)), "\n\n"); 
    fwrite(buf, 1, sizeof(buf), mt_msg_save);    	   		
}

void g_1553B_MTISR(S16BIT DevNum)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	WORD baseRamAddr = (DevNum%2 == 0) ? AT_BC_BASE_RAM_ADDR_BUS0 : AT_BC_BASE_RAM_ADDR_BUS1;
	//取得所有已经读到消息的下一个位置
	WORD msg_already =  MT_BLOCK_COUNT - 1 - AtReadReg( DevNum, 13 );
	while( dev_info->MT_msg_now != msg_already){

		AT_MT_MSG_COMMAND msgCmd;
		//读出当前位置的消息块信息
		AtReadBlock( DevNum, (dev_info->MT_msg_now * 8), (WORD*)&msgCmd, 8);
		
		//判断是否是新消息。如果不是，退出程序。否则，继续。
//		if(msgCmd.MsgInfo.MsgInfoWord == 0){
//			break;
//		}
		
		//清除消息块中的内容
//		AtRamSet( DevNum, dev_info->MT_msg_now*8, 0, 8);
		//清除数据指针的值
		AtWriteRam(DevNum, dev_info->MT_msg_now*8+3, 0);

		MSGSTRUCT msg;
		memset(&msg, 0, sizeof(MSGSTRUCT));
		msg.wTimeTag = msgCmd.TimeTag;
		//根据数据指针的值判断数据长度的合法性
		if(msgCmd.DataPtr){
			msg.wWordCount = msgCmd.MsgCmd1.Command.DataNum;
			if(msg.wWordCount == 0){
				msg.wWordCount = 32;
			}
			AtReadBlock( DevNum, msgCmd.DataPtr - baseRamAddr, msg.aDataWrds, 32 );
		}
		else{
			msg.wWordCount = 0;
		}
		msg.wCmdWrd1 = msgCmd.MsgCmd1.CommanWORD;
		msg.wCmdWrd2 = msgCmd.MsgCmd2.CommanWORD;
		msg.wStsWrd1 = msgCmd.Status1.StatusWord;
		msg.wStsWrd2 = msgCmd.Status2.StatusWord;
		//如果是B通道
		if(!msgCmd.MsgInfo.MsgInfo.CHA_B){
			msg.wBlkSts |= 0x2000;
		}
		msg.wCmdWrd1Flg = 1;
		if(msgCmd.Status1.Status.RTAddr == msgCmd.MsgCmd1.Command.RTAddr ||
			(msgCmd.MsgInfo.MsgInfo.RT_RT == 1 && msgCmd.Status1.Status.RTAddr == msgCmd.MsgCmd2.Command.RTAddr)){
			msg.wStsWrd1Flg = 1;
		}
		msg.wCmdWrd2Flg = 0;
		msg.wStsWrd2Flg = 0;
//			if(msgCmd.MsgInfo.MsgInfo.ME){  luxq
//				msg.wType = ACE_MSG_INVALID;
//				msg.wCmdWrd1Flg = 0;
//				msg.wStsWrd1Flg = 0;
//			}
		
		WORD subAddr1 = msgCmd.MsgCmd1.Command.SubAddr;
		WORD modeCode = msgCmd.MsgCmd1.Command.DataNum;
		WORD recvSend = msgCmd.MsgCmd1.Command.TRFlag;
		//广播消息
		if(msgCmd.MsgInfo.MsgInfo.BRD){
			if(msgCmd.MsgInfo.MsgInfo.RT_RT){
				msg.wType = ACE_MSG_BRDCSTRTTORT;
				msg.wCmdWrd2Flg = 1;
			}
			else{
				//方式命令
				if( (subAddr1 == 0) || (subAddr1 == 31) ){
					//没搞明白 BC->RT的方式命令不是都带数据吗 luxq
					if(modeCode == 0x11 || modeCode == 0x14 || modeCode == 0x15 ){
						msg.wType = ACE_MSG_BRDCSTMODEDATA;
						msg.wWordCount = 1;
					}
					else{
						msg.wType = ACE_MSG_BRDCSTMODENODATA;
						msg.wWordCount = 0;
					}
				}
				else{
					msg.wType = ACE_MSG_BRDCST;
				}
			}	
		}
		//非广播消息
		else{
			if(msgCmd.MsgInfo.MsgInfo.RT_RT){
				msg.wType = ACE_MSG_RTTORT;
				msg.wCmdWrd2Flg = 1;
				if(msgCmd.Status2.Status.RTAddr == msgCmd.MsgCmd1.Command.RTAddr){
					msg.wStsWrd2Flg = 1;
				}
			}
			else{
				//方式命令
				if( (subAddr1 == 0) || (subAddr1 == 31) ){
					if(modeCode == 0x11 || modeCode == 0x14 || modeCode == 0x15 ){
						msg.wType = ACE_MSG_MODEDATARX;
						msg.wWordCount = 1;
					}
					else if(modeCode == 0x10 || modeCode == 0x12 || modeCode == 0x13 ){
						msg.wType = ACE_MSG_MODEDATATX;
						msg.wWordCount = 1;
					}
					else{
						msg.wType = ACE_MSG_MODENODATA;
						msg.wWordCount = 0;
					}
				}
				else{
					if(recvSend == 1){
						msg.wType = ACE_MSG_RTTOBC;
					}
					else{
						msg.wType = ACE_MSG_BCTORT;
					}
				}
			}	
		}
		dev_info->MT_queue_MSG_lock.lock();
		dev_info->MT_queue_MSG.push(msg);
		dev_info->MT_queue_MSG_lock.unlock();

		dev_info->MT_msg_now++;
		if (dev_info->MT_msg_now == MT_BLOCK_COUNT)
		{
			dev_info->MT_msg_now = 0;
		}
		mt_save_the_message(&msg);
	}
}

S16BIT aceMTGetInfo(S16BIT DevNum, MTINFO *pInfo)
{

	return 0;
}

S16BIT aceMTInstallHBuf(S16BIT DevNum, U32BIT dwHBufSize)
{

	return 0;
}

S16BIT aceMTConfigure
(
    S16BIT DevNum,
    U16BIT wMTStkType,
    U16BIT wCmdStkSize,
    U16BIT wDataStkSize,
    U32BIT dwOptions
)
{

	return 0;
}

S16BIT aceMTStart(S16BIT DevNum)
{
	WORD wRegVal;
	wRegVal = AtReadReg(DevNum, 0);
	AtWriteReg( DevNum, 0, wRegVal | 0x8000 );
   
	char name[32]= {0};
	sprintf(name, "ISR_Thread%d", DevNum);
	map_devInfo[DevNum]->isr_thread.start(name);
	//挂1553B中断
	IrqConnect(DevNum/2, DevNum%2, g_1553B_myISR);
	IrqEnable(DevNum/2, DevNum%2, TRUE);

	return 0;
}

S16BIT aceMTStop(S16BIT DevNum)
{
	WORD wRegVal;
	wRegVal = AtReadReg( DevNum, 0 );
	AtWriteReg( DevNum, 0, wRegVal & 0x7FFF );	
	return 0;
}

S16BIT aceMTGetHBufMsgDecoded(S16BIT DevNum,
                                            MSGSTRUCT *pMsg,
                                            U32BIT *pdwMsgCount,
                                            U32BIT *pdwMsgLostStk,
                                            U32BIT *pdwMsgLostHBuf,
                                            U16BIT wMsgLoc)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return ACE_ERR_INVALID_DEVNUM;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if(dev_info->MT_queue_MSG.empty()){
		return 0;
	}

	dev_info->MT_queue_MSG_lock.lock();
	MSGSTRUCT msg = dev_info->MT_queue_MSG.front();
	memcpy(pMsg, &msg, sizeof(MSGSTRUCT) );
	
	if(wMsgLoc == ACE_MT_MSGLOC_NEXT_PURGE){
		dev_info->MT_queue_MSG.pop();
	}
	dev_info->MT_queue_MSG_lock.unlock();
	return 0;
}

S16BIT aceMTGetStkMsgDecoded(S16BIT DevNum,
                                           MSGSTRUCT *pMsg,
                                           U16BIT wMsgLoc,
                                           U16BIT wStkLoc)
{
	if (map_devInfo.find(DevNum) == map_devInfo.end()){
		return ACE_ERR_INVALID_DEVNUM;
	}
	CDeviceInfo* dev_info = map_devInfo[DevNum];

	if(dev_info->MT_queue_MSG.empty()){
		return 0;
	}

	dev_info->MT_queue_MSG_lock.lock();
	MSGSTRUCT msg = dev_info->MT_queue_MSG.front();
	memcpy(pMsg, &msg, sizeof(MSGSTRUCT) );
	
	if(wMsgLoc == ACE_MT_MSGLOC_NEXT_PURGE){
		dev_info->MT_queue_MSG.pop();
	}
	dev_info->MT_queue_MSG_lock.unlock();
	return 1;
}

void mt_set_file(FILE *fp)
{
      mt_msg_save = fp;
	  if(mt_msg_save == NULL)
	  {
			printf("mt_msg_save is NULL\n");
	  }
	   return;
}

