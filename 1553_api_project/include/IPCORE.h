/**
 * IPCORE.h
 * IPCORE版本的1553B驱动接口，使用本驱动的用户需包含本头文件。
 * \date 2013-6-5
 * \author xiaoqing.lu
 */

#ifndef __IPCORE_H__
#define __IPCORE_H__

#include "types.h"
#include "msgop.h"
#include "thread.h"
#include "zynq_1553b_api.h"

//zhanghao add.
//#ifdef DUMP_RAM
int IrqConnect(UINT16 cardNum, UINT16 chnNum, void(*funcIsr)( UINT16 cardNum,  UINT16 chnNum));
int IrqEnable(UINT16 cardNum, UINT16 chnNum, int enable);
int GetCardCount();
int  open( UINT16 cardNum );
int  close( );
//#endif

/* ========================================================================== */
/*                              common FUNCTION                                   */
/* ========================================================================== */
//MT采用中断加轮询的方式读数据,使用Adapter传入的syncEvent
S16BIT aceInitialize(S16BIT DevNum, 
		U16BIT wAccess, 
		U16BIT wMode, 
		U32BIT dwMemWrdSize, 
		U32BIT dwRegAddr, 
		U32BIT dwMemAddr,
		SyncEvent* MTsync = NULL);
S16BIT aceFree(S16BIT DevNum);
S16BIT aceSetIrqConditions(S16BIT DevNum, 
		U16BIT bEnable, 
		U32BIT dwIrqMask, 
		void(*funcExternalIsr)(S16BIT DevNum, U32BIT dwIrqStatus));
S16BIT aceCmdWordCreate(U16BIT *pCmdWrd,
									  U16BIT wRT,
									  U16BIT wTR,
									  U16BIT wSA,
									  U16BIT wWC);
S16BIT aceCmdWordParse(U16BIT wCmdWrd,
									 U16BIT *pRT,
									 U16BIT *pTR,
									 U16BIT *pSA,
									 U16BIT *pWC);
S16BIT aceGetTimeTagValueEx(S16BIT DevNum, 
		U64BIT* ullTTValue);

bool g_1553B_BCISR(S16BIT DevNum);
void g_1553B_RTISR(S16BIT DevNum);
void g_1553B_MTISR(S16BIT DevNum);
void mt_set_file(FILE *fp);
/* ========================================================================== */
/*                              BC FUNCTION                                   */
/* ========================================================================== */
S16BIT aceBCConfigure(S16BIT DevNum, U32BIT dwOptions);
S16BIT aceBCMsgGapTimerEnable(S16BIT DevNum, U16BIT bEnable);
S16BIT aceBCDataBlkCreate(S16BIT DevNum, S16BIT nDataBlkID, U16BIT wDataBlkType, U16BIT *pBuffer, U16BIT wBufferSize);
S16BIT aceBCDataBlkDelete(S16BIT DevNum, S16BIT nDataBlkID);
S16BIT aceBCDataBlkWrite(S16BIT DevNum, S16BIT nDataBlkID, U16BIT *pBuffer, U16BIT wBufferSize, U16BIT wOffset);
S16BIT aceBCFrameCreate(S16BIT DevNum, S16BIT nFrameID, U16BIT wFrameType, S16BIT aOpCodeIDs[], U16BIT wOpCodeCount,
										U16BIT wMnrFrmTime, U16BIT wFlags);
S16BIT aceBCOpCodeCreate(S16BIT DevNum, S16BIT nOpCodeID, U16BIT wOpCodeType, U16BIT wCondition, U32BIT dwParameter1,
										U32BIT dwParameter2, U32BIT dwReserved);
S16BIT aceBCMsgCreateBCtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wSA, U16BIT wWC,
											U16BIT wMsgGapTime, U32BIT dwMsgOptions);
S16BIT aceBCMsgCreateRTtoBC(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wSA, U16BIT wWC,
											U16BIT wMsgGapTime, U32BIT dwMsgOptions);
S16BIT aceBCMsgCreateRTtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRTRx, U16BIT wSARx, U16BIT wWC,
											U16BIT wRTTx, U16BIT wSATx, U16BIT wMsgGapTime, U32BIT dwMsgOptions);
S16BIT aceBCMsgCreateMode(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wTR, U16BIT wModeCmd,
											U16BIT wMsgGapTime, U32BIT dwMsgOptions);
S16BIT aceBCMsgCreateBcst(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wSA, U16BIT wWC, U16BIT wMsgGapTime,
											U32BIT dwMsgOptions);
S16BIT aceBCMsgCreateBcstRTtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wSARx, U16BIT wWC, 
												U16BIT wRTTx, U16BIT wSATx, U16BIT wMsgGapTime, U32BIT dwMsgOptions);
S16BIT aceBCMsgCreateBcstMode(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wTR, U16BIT wModeCmd, 
												U16BIT wMsgGapTime, U32BIT dwMsgOptions);
S16BIT aceBCMsgModifyRTtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRTRx, U16BIT wSARx, U16BIT wWC,
											U16BIT wRTTx, U16BIT wSATx, U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT wModFlags);
S16BIT aceBCMsgModifyMode(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wTR, U16BIT wModeCmd,
											U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT wModFlags);
S16BIT aceBCMsgModify(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID1, U16BIT wBCCtrlWrd1, U16BIT wCmdWrd1_1,
										U16BIT wCmdWrd1_2, U16BIT wMsgGapTime1, S16BIT nDataBlkID2, U16BIT wBCCtrlWrd2, U16BIT wCmdWrd2_1,
										U16BIT wCmdWrd2_2, U16BIT wMsgGapTime2, U16BIT wModFlags);
S16BIT aceBCMsgModifyBcstMode(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wTR, U16BIT wModeCmd, 
												U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT wModFlags);
S16BIT aceBCMsgDelete(S16BIT DevNum, S16BIT nMsgBlkID);
S16BIT aceBCStart(S16BIT DevNum, S16BIT nMjrFrmID, S32BIT lMjrFrmCount);
S16BIT aceBCStop(S16BIT DevNum);
S16BIT aceBCSetGPFState(S16BIT DevNum, U16BIT wGPF, U16BIT wStateChange);
S16BIT aceBCGetMsgFromIDDecoded(S16BIT DevNum, S16BIT nMsgBlkID, MSGSTRUCT *pMsg, U16BIT bPurge);
S16BIT aceBCSetMsgRetry(S16BIT DevNum, U16BIT wNumOfRetries, U16BIT wFirstRetryBus, U16BIT wSecondRetryBus, U16BIT wReserved);
S16BIT aceBCAsyncMsgCreateBCtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wSA, U16BIT wWC,
													U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCAsyncMsgCreateRTtoBC(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wSA, U16BIT wWC,
													U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCAsyncMsgCreateRTtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRTRx, U16BIT wSARx, U16BIT wWC,
													U16BIT wRTTx, U16BIT wSATx, U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCAsyncMsgCreateMode(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wRT, U16BIT wTR, U16BIT wModeCmd,
												U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCAsyncMsgCreateBcst(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wSA, U16BIT wWC, U16BIT wMsgGapTime,
												U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCAsyncMsgCreateBcstRTtoRT(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wSARx, U16BIT wWC, 
														U16BIT wRTTx, U16BIT wSATx, U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCAsyncMsgCreateBcstMode(S16BIT DevNum, S16BIT nMsgBlkID, S16BIT nDataBlkID, U16BIT wTR, U16BIT wModeCmd, 
													U16BIT wMsgGapTime, U32BIT dwMsgOptions, U16BIT *pBuffer);
S16BIT aceBCSendAsyncMsgHP(S16BIT DevNum, U16BIT nMsgID, WORD *pData, WORD nDataLen, U16BIT wTimeFactor);
S16BIT aceBCEmptyAsyncList(S16BIT DevNum);
S16BIT aceBCResetAsyncPtr(S16BIT DevNum);
S16BIT aceBCGetMsg(S16BIT DevNum, S16BIT nMsgBlkID, U16BIT bPurge);

/* ========================================================================== */
/*                              RT FUNCTION                                   */
/* ========================================================================== */
S16BIT aceRTSetAddress
(
    S16BIT DevNum,
    U16BIT wRTAddress
);
S16BIT aceRTDataBlkCreate
(
    S16BIT DevNum,
    S16BIT nDataBlkID,
    U16BIT wDataBlkType,
    U16BIT *pBuffer,
    U16BIT wBufferSize
);
S16BIT aceRTDataBlkWrite
(
    S16BIT DevNum,
    S16BIT nDataBlkID,
    U16BIT *pBuffer,
    U16BIT wBufferSize,
    U16BIT wOffset
);
S16BIT aceRTDataBlkMapToSA
(
    S16BIT DevNum,
    S16BIT nDataBlkID,
    U16BIT wSA,
    U16BIT wMsgType,
    U16BIT wIrqOptions,
    U16BIT wLegalizeSA
);
S16BIT aceRTModeCodeIrqEnable
(
    S16BIT DevNum,
    U16BIT wModeCodeType,
    U16BIT wModeCodeIrq
);
S16BIT aceRTMTStart(S16BIT DevNum);
S16BIT aceRTMTStop(S16BIT DevNum);
S16BIT aceRTGetStkMsgDecoded
(
    S16BIT DevNum,
    MSGSTRUCT *pMsg,
    U16BIT wMsgLoc
);
S16BIT aceRTStatusBitsSet
(
    S16BIT DevNum,
    U16BIT wStatusBits
);
S16BIT aceRTStatusBitsClear
(
    S16BIT DevNum,
    U16BIT wStatusBits
);
S16BIT aceRTModeCodeWriteData
(
    S16BIT DevNum,
    U16BIT wModeCode,
    U16BIT wMCData
);

/* ========================================================================== */
/*                              MT FUNCTION                                   */
/* ========================================================================== */
S16BIT aceMTGetInfo(S16BIT DevNum, MTINFO *pInfo);
S16BIT aceMTInstallHBuf(S16BIT DevNum, U32BIT dwHBufSize);
S16BIT aceMTConfigure
(
    S16BIT DevNum,
    U16BIT wMTStkType,
    U16BIT wCmdStkSize,
    U16BIT wDataStkSize,
    U32BIT dwOptions
);
S16BIT aceMTStart(S16BIT DevNum);
S16BIT aceMTStop(S16BIT DevNum);
S16BIT aceMTGetHBufMsgDecoded(S16BIT DevNum,
                                            MSGSTRUCT *pMsg,
                                            U32BIT *pdwMsgCount,
                                            U32BIT *pdwMsgLostStk,
                                            U32BIT *pdwMsgLostHBuf,
                                            U16BIT wMsgLoc);
S16BIT aceMTGetStkMsgDecoded(S16BIT DevNum,
                                           MSGSTRUCT *pMsg,
                                           U16BIT wMsgLoc,
                                           U16BIT wStkLoc);

#endif /* __IPCORE_H__ */

