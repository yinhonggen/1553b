/**
 * actel1553mt.h
 * MT结构和功能函数定义
 * \date 2013-6-5
 * \author xiaoqing.lu
 */
#ifndef _1553COREMT_H_
#define _1553COREMT_H_

#include "actel1553io.h"
/*Monitor Message Infomation Word*/
typedef union
{
	WORD MsgInfoWord;
	struct
	{
		unsigned short MAN		: 1;
		unsigned short PRTY		: 1;
		unsigned short OVR		: 1;
		unsigned short TO		: 1;
		unsigned short NA		: 1;
		unsigned short BRD		: 1;
		unsigned short MCwD		: 1;
		unsigned short ME		: 1;
		unsigned short RT_RT	: 1;
		unsigned short CHA_B	: 1;
		unsigned short Retry	: 2;
		unsigned short Opcode	: 4;
	}MsgInfo;
}AT_MT_MSG_INFO, *PAT_MT_MSG_INFO, *LPAT_MT_MSG_INFO;
/*Monitor Blocks*/
typedef struct
{
	AT_MT_MSG_INFO MsgInfo;
	AT_MSG_COMMAND MsgCmd1;
	AT_MSG_COMMAND MsgCmd2;
	WORD DataPtr;
	AT_MSG_STATUS Status1;
	AT_MSG_STATUS Status2;
	WORD TimeTag;
	WORD NA;
}AT_MT_MSG_COMMAND, *PAT_MT_MSG_COMMAND;

#endif

