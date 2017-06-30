#include <stdio.h>
#include <stdlib.h>
#include "zynq_1553b_api.h"
#include "callback.h"


/*回调函数*/
ZYNQ_MSG_CALLBACK  g_pfucCallback = NULL;

/******************************************************************
*函数名 ：zynq_reg_func 
*函数功能描述 ：回调函数的设置
*函数参数 ：pfunc：回调函数原函数
*函数返回值 ：无
********************************************************************/
void zynq_reg_func(ZYNQ_MSG_CALLBACK pfunc)
{
		g_pfucCallback = pfunc;
}
