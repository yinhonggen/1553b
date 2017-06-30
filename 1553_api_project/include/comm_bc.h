/**
* comm_bc.h
* BC的配置工具和适配器的接口文件
*
*    \date 2003-8-24
*  \author xiaoqing.lu
*/

#ifndef _COMM_BC_H_
#define _COMM_BC_H_

/**
* 消息类型
*/
typedef enum _enumMsgType
{
	MSG_BC_TO_RT = 0,			///< BC->RT消息
	MSG_RT_TO_BC = 1,			///< RT->BC消息
	MSG_RT_TO_RT = 2,			///< RT->RT消息
	MSG_MODE_CODE = 3,			///< 方式命令消息
	MSG_BROADCAST = 4,			///< BC->RT广播消息
	MSG_RT_BROADCAST = 5,		///< RT->RT广播消息
	MSG_MODE_CODE_BROADCAST = 6,///< 方式命令广播消息
	MSG_OPCODE_IRQ = 7,			///< 中断操作
}
enumMsgType;

/**
* 消息发送通道
*/
typedef enum _enumChannel
{
	CHANNEL_A = 0,	///< A通道
	CHANNEL_B = 1,	///< B通道
	CHANNEL_AB = 2,	///< AB通道交替
}
enumChannel;

/**
* 消息发送时机
*/
typedef enum _enumSendTiming
{
	SENDTIMING_ALWAYS = 0,			///< 固定发送
	SENDTIMING_FOLLOW_SYNC = 1,		///< 根据别的消息内容判断是否发送_同步
	SENDTIMING_FOLLOW_ASYNC = 2,	///< 根据别的消息内容判断是否发送_异步
	SENDTIMING_DUP = 3,				///< 数据注入
}
enumSendTiming;

/**
* 消息属性类
*/
class BC_Msg_info
{
public:
	char name[66];
	short id;					///< 消息id
	short blkid;				///< 数据块id
	enumMsgType type;			///< 消息类型
	BOOL isAsync;				///< 是否异步发送
	unsigned int gap_time;		///< 延时
	short length;				///< 消息长度，方式命令中复用为方式码
	unsigned short data[32];	///< 数据
	enumChannel channel;		///< 通道
	BOOL isRetry;				///< 是否重试
	short Addr[4];				///< 源地址、源子地址、目的地址、目的子地址
	enumSendTiming sendTiming;	///< 发送时机
	unsigned int time;			///< 周期性发送消息的周期
	short followMsgID;			///< 根据哪个消息判断是否要发
	unsigned short followValue;	///< 判断值
	unsigned short followMask;	///< 判断MASK
	short frameIndex;			///< 该消息属于哪个小帧
 	short followDataindex;		///< 判断该消息的第几个数据字
	BOOL isCheckSrvbit;			///< 是否判断消息的服务请求位
	BOOL isCanMiss;				///< 是否能漏发停发
	BOOL isTimeCode;			///< 是否是时间码
	BOOL isCheckCRC;			///< 是否添加校验
	short CRCMode;				///< 校验方式：累加和或CRC
	short CRCTiming;			///< 校验时机：消息末尾或包末尾
	BOOL isFromFile;			///< 是否有仿真文件
	short packLen;				///< 包长度,单位是字
//	unsigned char defaultValue;		///< 拆包后尾包的默认填充值
//	unsigned char reverse;			///< 保留位
	unsigned short defaultValue;	///< 拆包后尾包的默认填充值
	char filename[128];			///< 仿真文件名
};

/// 配置文件必须以该字符串打头，否则视为无效文件
#define CHECK_STRING "BCConfcheck"
#endif
