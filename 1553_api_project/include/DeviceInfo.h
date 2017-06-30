#pragma once

#include "head.h"
class ISR_Thread : public Thread{
public:
	ISR_Thread(){
	}
	ISR_Thread(S16BIT DevNum){
		this->DevNum = DevNum;
	}
	SyncEvent syncEvent;
	SyncEvent* MT_sync; //MT使用Adapter传入的sync，这样定时器和中断都可以signal它
	void do_job();
private:
	S16BIT DevNum;
};

///记录每个通道用到的变量
class CDeviceInfo
{
public:
	CDeviceInfo(void);
	CDeviceInfo(S16BIT DevNum);
	~CDeviceInfo(void);

	S16BIT DevNum;///< 通道号，从0开始，是相对于整个机器的通道号

	U32BIT IPCORE_wIntPtr;	///< 中断日志指针
	int is_IPCORE_start;	///< 是否开始工作
	WORD IPCORE_mode;		///< BC RT MT


	void (*g_1553B_usrISR)( S16BIT DevNum, U32BIT dwIrqStatus );
	ISR_Thread isr_thread;

	///BC
	///保存组帧前消息块和数据块的一些全局变量
	vector<AT_COMMAND_BLOCK> BC_vec_CmdBlk; ///< 保存消息命令块和延时命令块，如果是异步消息还要保存ret call commad
	map<WORD, WORD> BC_map_msgNo_cmdIndex; ///< <同步消息号，消息命令块index>
	map<WORD, WORD> BC_map_AsyncMsgNo_cmdIndex; ///< <异步消息号，消息命令块index>
	map<WORD, AT_DATA_BLK> BC_map_dataNo_data; ///< <数据块号，数据块> 数据块号的值存入MsgBlk->DataPtr中

	///用于组帧的一些全局变量,组帧完成之后就不再用了
	map<WORD, WORD> BC_map_msgCode_msgNo;///< <消息操作码，消息号>
	map<WORD, vector<WORD> > BC_map_minFraID_msgCodes;///< <小帧号，消息操作码>
	map<WORD, WORD> BC_map_minFraID_minFraTime; ///< <小帧号，小帧周期>
	map<WORD, WORD> BC_map_minFraCode_minFraID;///< <小帧操作码，小帧号>

	///保存组帧后RAM里消息块和数据块的地址一些全局变量
	set<pair<WORD,WORD> > BC_set_msgNo_blkAddr; ///< <消息号，消息块的RAM地址>全部的
	map<WORD, WORD> BC_map_msgNo_ifCallBlkAddr; ///< <异步消息号，异步消息调用块的RAM地址>
	map<WORD, WORD> BC_map_dataNo_dataAddr; ///< <数据块号，数据块的RAM地址>

	///保存执行过的消息的一些全局变量
	map<WORD, queue<pair<AT_COMMAND_BLOCK,AT_DATA_BLK> > > BC_map_msgNo_blkQueue; ///< <消息号，消息块的队列>BC消息的写入和读出是在一个线程中完成的，所以不用加锁
	map<WORD, vector<pair<WORD,WORD> > > BC_map_IntBlkAddr_msgAddr;///< <小帧末和异步消息末中断块地址，vec<消息号, 消息块RAM地址>>

	///保存异步消息的发送数据
	map<WORD, queue<AT_DATA_BLK> > BC_map_AsyncMsgNo_dataQueue; ///< <异步消息号，该异步消息的数据队列>

	///RT
	map<WORD, WORD> RT_map_dataID_dataAddr; ///< <数据块ID, 数据RAM地址> 
	map<WORD, WORD> RT_map_dataID_dspBlkAddr; ///< <数据块ID, 对应的描述块地址>
	queue<MSGSTRUCT> RT_queue_MSG; ///< 保存1553消息，写入和读出是在一个线程里的，所以无需加锁。

	///MT
	queue<MSGSTRUCT> MT_queue_MSG; ///< 保存1553消息
	MXLock MT_queue_MSG_lock; ///< MT_queue_MSG的消息写入和读出是在两个线程中，所以需要加锁
	short MT_msg_now; ///< MT采用中断加轮询的方式读数据，该变量记录待读消息的当前位置
};
