/*	
 * msgop.h
 * 从DDC驱动中抄来的宏定义和结构定义
 * \date 2013-6-5
 * \author xiaoqing.lu
 */

#ifndef __MSGOP_H__
#define __MSGOP_H__
#include "types.h"

/* the T/R* bit can be defined as follows (wTR parameters) */
#define ACE_RX_CMD	0x0000
#define ACE_TX_CMD	0x0001

/* 1553 message type constants - the values assigned to these constants
 * are from a the below bitmap.
 *
 * Bcst			= RT = ACE_BROADCAST
 * Mode			= SA = ACE_MODE_CODE1 or ACE_MODE_CODE2
 * C2/Data		= RT to RT transfer or Data associated with mode code
 * Tx/~Rx		= bit 10 of Cmd Word 1
 */

#define ACE_BROADCAST		0x001F	/* Broadcast RT address (31) */ 
#define ACE_MODE_CODE1		0x0000	/* Mode Code Subaddress (0)  */
#define ACE_MODE_CODE2		0x001F	/* Mode Code Subaddress (31) */

										/* Bcst   Mode  C2/Data  Tx/~Rx	*/
#define ACE_MSG_BCTORT				0 	/*   0		0		0		0	*/
#define ACE_MSG_RTTOBC				1 	/*   0		0		0		1	*/
#define ACE_MSG_RTTORT				2 	/*   0		0		1		0	*/
#define ACE_MSG_MODENODATA			3 	/*   0		1		0		1	*/
#define ACE_MSG_MODEDATARX			4 	/*   0		1		1		0	*/
#define ACE_MSG_MODEDATATX			5 	/*   0		1		1		1	*/
#define ACE_MSG_BRDCST				6 	/*   1		0		0		0   */
#define ACE_MSG_BRDCSTRTTORT		7 	/*   1		0		1		0	*/
#define ACE_MSG_BRDCSTMODENODATA	8 	/*   1		1		0		1   */
#define ACE_MSG_BRDCSTMODEDATA		9	/*   1		1		1		0   */
#define ACE_MSG_INVALID				10	/*	 1		1		1		1   */

/* Data Stream Types */
#define ACE_STREAM_BCRT				0
#define ACE_STREAM_RTRT				1

/* RAW message sizes */
#define ACE_MSGSIZE_RT	 36
#define ACE_MSGSIZE_MT	 40
#define ACE_MSGSIZE_BC	 42
#define ACE_MSGSIZE_RTMT 42
#define ACE_MSGSIZE_MRT  40

#define ACE_BCCTRL_SELFTST		            0x0040
#define ACE_BCCTRL_CHL_A		            0x0080
#define ACE_BCCTRL_CHL_B		            0x0000
#define ACE_BCCTRL_RETRY_ENA	            0x0100
#define ACE_BC_MOD_CMDWRD1_1      	     0x0004
#define ACE_BC_MOD_BCCTRL1       	     0x0002
#define ACE_OPCODE_XEQ			            0x0001	/* Execute Message */
#define ACE_OPCODE_CAL			            0x0003	/* Call subroutine (frame) */
#define ACE_FRAME_MAJOR		            0x0000
#define ACE_FRAME_OTHER		            0x0001
#define ACE_FRAME_MINOR		            0x0002
#define ACE_IMR2_BC_UIRQ0			     0x00040000
#define ACE_CNDTST_ALWAYS			     0x000F	/* always run opcode */
#define ACE_IMR1_RT_SUBADDR_EOM 	     0x00000010
#define ACE_IMR1_RT_MODE_CODE	     0x00000002
#define ACE_OPCODE_DLY		0x0008	/* Delay in microseconds */
#define ACE_GPF_SET			0x0001
#define ACE_GPF_CLEAR		0x0002
#define ACE_RETRY_TWICE	2
#define ACE_RETRY_SAME		0
#define ACE_RETRY_ALT		1
#define ACE_BC_ASYNC_BOTH  0x00000003
#define ACE_OPCODE_IRQ		0x0006	/* Irq request (user ISR2 bits set) */
#define ACE_OPCODE_FLG		0x000C	/* Set,clear,toggle 8 GP bits */
#define ACE_ACCESS_CARD	0		/* ACE card running on W32 OS */
#define ACE_MODE_BC		0x0001	/* 1553 bus controller mode */
#define ACE_MODE_RT		0x0002	/* 1553 remote terminal mode */
#define ACE_MODE_MT		0x0003	/* 1553 monitor terminal mode */
#define ACE_MODE_RTMT		0x0004	/* 1553 RT with monitoring */
#define ACE_MODE_MTI		0x0006  /* Flexcore MT Improved mode */
#define ACE_MODE_RTMTI		0x0007  /* Flexcore RTMT Improved mode */

#define ACE_RT_MSGTYPE_RX	0x0001	/* Rx msgs to SA will use data blk */
#define ACE_RT_MSGTYPE_TX	0x0002	/* Tx msgs to SA will use data blk */
#define ACE_RT_MSGLOC_NEXT_PURGE		0	/* next unread msg, clr stk */
#define ACE_RT_MSGLOC_NEXT_NPURGE	1	/* next unread msg, leave stk */
#define ACE_RT_MSGLOC_LATEST_PURGE	2	/* latest msg, clear stk */
#define ACE_RT_MSGLOC_LATEST_NPURGE	3	/* latest msg, leave stk alone */
#define ACE_RT_STSBIT_DBCA				0x0800
#define ACE_RT_STSBIT_BUSY				0x0400
#define ACE_RT_STSBIT_SREQ				0x0200
#define ACE_RT_STSBIT_SSFLAG			0x0100
#define ACE_RT_STSBIT_RTFLAG			0x0080
#define ACE_RT_DBLK_DOUBLE		    33	/* doubled buffered msg dblk 64 wds */
#define ACE_RT_DBLK_EOM_IRQ		    0x0002 /* Irqs gen on end of msg */
#define ACE_IMR1_EOM 				0x00000001
#define ACE_RT_MCTYPE_RX_NO_DATA		0x0000 /* undefined mc's */
#define ACE_RT_MCTYPE_RX_DATA			0x0001 /* rx mc's w/ data */
#define ACE_RT_MCTYPE_TX_NO_DATA		0x0002 /* tx mc's w/o data */
#define ACE_RT_MCTYPE_TX_DATA			0x0003 /* tx mc's w/ data */
#define ACE_RT_MCIRQ_TRNS_VECTOR		0x0001 /* TX_DATA */
#define ACE_RT_MCIRQ_SYNCHRONIZE		0x0002 /*(BCST_)(TX|RX)(_NO)_DATA*/
#define ACE_RT_MCDATA_TX_TRNS_VECTOR		0x0010
#define ACE_MT_MSGLOC_NEXT_PURGE	0	/* next unread msg, clr stk */
#define ACE_ERR_SUCCESS             0     /* No error occurred */
#define ACE_ERR_NOT_SUPPORTED          -68   /* Function not supported */
#define ACE_ERR_INVALID_DEVNUM         -50   /* Bad Card number */
#define ACE_MT_SINGLESTK	0		/* Use a single stack */
#define ACE_MT_CMDSTK_4K	0x1000	/* Cmd Stacks are 4K words */
#define ACE_MT_CMDSTK_16K	0x1800	/* Cmd Stacks are 16K words */
#define ACE_MT_DATASTK_32K	0x0100	/* data Stacks are 16K words */
#define ACE_IMR2_MT_DSTK_50P_ROVER	0x00400000

#define ACE_MSG_BCTORT				  0 	/*   0		0		0		0	*/
#define ACE_MSG_RTTOBC				  1 	/*   0		0		0		1	*/
#define ACE_MSG_RTTORT				  2 	/*   0		0		1		0	*/
#define ACE_MSG_MODENODATA		  3 	/*   0		1		0		1	*/
#define ACE_MSG_MODEDATARX		  4 	/*   0		1		1		0	*/
#define ACE_MSG_MODEDATATX		  5 	/*   0		1		1		1	*/
#define ACE_MSG_BRDCST				  6 	/*   1		0		0		0   */
#define ACE_MSG_BRDCSTRTTORT		  7 	/*   1		0		1		0	*/
#define ACE_MSG_BRDCSTMODENODATA	8 	/*   1		1		0		1   */
#define ACE_MSG_BRDCSTMODEDATA		9	/*   1		1		1		0   */
#define ACE_MT_STKLOC_ACTIVE		0	/* Use the active stk */

#define ACE_MSG_INVALID				10	/*	 1		1		1		1   */
/* Global (used for all modes) Message Structure for decoded 1553 msgs */
typedef struct MSGSTRUCT
{
	U16BIT wType;				/* Contains the msg type (see above) */
	U16BIT wBlkSts;				/* Contains the block status word */
	U16BIT wTimeTag;			/* Time tag of message */
	U16BIT wCmdWrd1;			/* First command word */
	U16BIT wCmdWrd2;			/* Second command word (RT to RT) */
	U16BIT wCmdWrd1Flg;		/* Is command word 1 valid? */
	U16BIT wCmdWrd2Flg;		/* Is command word 2 valid? */
	U16BIT wStsWrd1;			/* First status word */
	U16BIT wStsWrd2;			/* Second status word */
	U16BIT wStsWrd1Flg;			/* Is status word 1 valid? */
	U16BIT wStsWrd2Flg;			/* Is status word 2 valid? */
	U16BIT wWordCount;			/* Number of valid data words */
	U16BIT aDataWrds[32];		/* An array of data words */

	/* The following are only applicable in BC mode */
	U16BIT wBCCtrlWrd;			/* Contains the BC control word */
	U16BIT wBCGapTime;			/* Message gap time word */
	U16BIT wBCLoopBack1;		/* First looped back word */
	U16BIT wTimeTag2;			/* wBCLoopBack2 is redefined as TimeTag2 */
	U16BIT wBCLoopBack1Flg;		/* Is loop back 1 valid? */
	U16BIT wTimeTag3;			/* wBCLoopBack2Flg is redefined as TimeTag3 */
	
}MSGSTRUCT;

typedef struct MTINFO
{
	U16BIT wStkMode;
	U16BIT wCmdStkSize;
	U16BIT wDataStkSize;
	U16BIT b1553aMCodes;
	U32BIT dwHBufSize;
}MTINFO;

#endif


