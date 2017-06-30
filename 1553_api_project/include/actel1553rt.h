/**
 * actel1553rt.h
 * RT结构和功能函数定义
 * \date 2013-6-5
 * \author xiaoqing.lu
 */
#ifndef _1553CORERT_H_
#define _1553CORERT_H_

#include "actel1553io.h"
/*Actel Descriptor Block Control Word*/
typedef union
{
	WORD DespBlkCtlWord;
	struct
	{
		unsigned	INDX		         : 8;/*Index*/
		unsigned	INTX			     : 1;/*Interrupt Equals Zero*/
		unsigned	IWA	    			 : 1;/*Interrupt When Accessed*/
		unsigned	IBRD		         : 1;/*Interrupt Broadcast Received*/
		unsigned	BAC					 : 1;/*Block Accessed*/
		unsigned	LA_B		         : 1;/*Last A or B Buffer*/
		unsigned	BufA_B		         : 1;/*A or B Buffer*/
		unsigned	BRD			         : 1;/*Broadcast*/
		unsigned	NII			         : 1;/*Notice II*/
	} DespBlkCtl;
} AT_DESP_BLK_CTL, *PAT_DESP_BLK_CTL, *LPAT_DESP_BLK_CTL;
/*Descriptor Block*/
typedef struct
{
	AT_DESP_BLK_CTL DespBlkCtl;
	WORD DataPtrA;
	WORD DataPtrB;
	WORD BcDataPtr;
} AT_DESCRIPTOR_BLK, *PAT_DESCRIPTOR_BLK, *LPAT_DESCRIPTOR_BLK;
/*Message Infomation Word*/
typedef union
{
	WORD MsgInfoWord;
	struct 
	{
		unsigned WC_MC		: 5;
		unsigned NA2		: 1;
		unsigned CHA_B		: 1;
		unsigned RT_RT		: 1;
		unsigned ME			: 1;
		unsigned NA1		: 2;
		unsigned ILL		: 1;
		unsigned TO			: 1;
		unsigned OVR		: 1;
		unsigned PRTY		: 1;
		unsigned MAN		: 1;
	}MsgInfo;
	
}AT_RT_MSG_INFO, *PAT_RT_MSG_INFO, *LPAT_RT_MSG_INFO;
/*使用1/4的RAM定义消息，使用3/4的RAM保存消息的数据字*/

#endif

