#include "zynq_1553b_api.h"
#include "DeviceInfo.h"
#include "IPCORE.h"
#include "common_qiu.h"

extern map<S16BIT, CDeviceInfo*> map_devInfo;

void ISR_Thread::do_job(){
	CDeviceInfo* dev_info = map_devInfo[DevNum];
	
	if(dev_info->IPCORE_mode == ACE_MODE_BC){
		syncEvent.wait(1000);
		//只用0002中断才需要调用户中断函数
		if(g_1553B_BCISR(DevNum)){
			dev_info->g_1553B_usrISR(DevNum, ACE_IMR2_BC_UIRQ0);			
		}
		//在g_1553B_usrISR里BCAdapter读出了所有它关心的消息，其他消息需要删除，防止占用空间过大。
		dev_info->BC_map_msgNo_blkQueue.clear();  
	}
	else if(dev_info->IPCORE_mode == ACE_MODE_RT || dev_info->IPCORE_mode == ACE_MODE_RTMT){
		syncEvent.wait(1000); /*yinhonggen添加时间1000*/
		g_1553B_RTISR(DevNum);
		dev_info->g_1553B_usrISR(DevNum, ACE_IMR1_RT_SUBADDR_EOM | ACE_IMR1_RT_MODE_CODE);
	}
	//else if(dev_info->IPCORE_mode == ACE_MODE_MT && MT_sync){   //close by yinhonggen
	 else if(dev_info->IPCORE_mode == ACE_MODE_MT){
	    
		//T_sync->wait(1000);
		syncEvent.wait(1000);
		g_1553B_MTISR(DevNum);
		//g_1553B_usrISR(DevNum, 0);  MTAdapter并未挂中断，是在read函数中调读数接口的。
      //  usleep(500000);
	 }

}

CDeviceInfo::CDeviceInfo(void)
{
}


CDeviceInfo::CDeviceInfo(S16BIT DeviceNum)
:DevNum(DeviceNum),
IPCORE_mode(0),
is_IPCORE_start(FALSE),
MT_msg_now(0),
isr_thread(DeviceNum)
{

}

CDeviceInfo::~CDeviceInfo(void)
{
	isr_thread.syncEvent.signal();
	if (isr_thread.MT_sync)
	{
		isr_thread.MT_sync->signal();
	}
	isr_thread.exit();

	if(IPCORE_mode == ACE_MODE_BC){
		BC_vec_CmdBlk.clear();
		BC_map_msgNo_cmdIndex.clear();
		BC_map_AsyncMsgNo_cmdIndex.clear();
		BC_map_dataNo_data.clear();

		BC_map_msgCode_msgNo.clear();
		BC_map_minFraID_msgCodes.clear();
		BC_map_minFraID_minFraTime.clear();
		BC_map_minFraCode_minFraID.clear();

		BC_set_msgNo_blkAddr.clear();
		BC_map_msgNo_ifCallBlkAddr.clear();
		BC_map_dataNo_dataAddr.clear();

		BC_map_msgNo_blkQueue.clear();
		BC_map_IntBlkAddr_msgAddr.clear();

		BC_map_AsyncMsgNo_dataQueue.clear();
	}
	else if(IPCORE_mode == ACE_MODE_RT || IPCORE_mode == ACE_MODE_RTMT){
		RT_map_dataID_dataAddr.clear();
		RT_map_dataID_dspBlkAddr.clear();
		while(!RT_queue_MSG.empty()){
			RT_queue_MSG.pop();
		}
	}
	else if(IPCORE_mode == ACE_MODE_MT){
		while(!MT_queue_MSG.empty()){
			MT_queue_MSG.pop();
		}
	}
}
