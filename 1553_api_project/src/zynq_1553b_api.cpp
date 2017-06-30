#include <string.h>
#include "zynq_1553b_api.h"
#include "common_bc.h"
#include "genmti.h"
#include "BrAda1553B_RT.h"
#include "actel1553io.h"


/*定义全局类，包括BC,RT,MT的类*/
BrAda1553B_RT  adaRT;
BrAda1553B_BC_COMM  adaBC;
GIST_MT2 adaMT;


/*************************************************************
*函数名 ：zynq_bc_set_config 
*函数功能描述 ：此函数用于在 BC 模式下，对配置文件的名字的设定,
                以及配置文件的解析，模块函数调用入口。 
*函数参数 ：cfg:输入参数，配置文件的名字
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_bc_set_config(const std::string cfg)
{
       std::string str;
			 
		adaBC.set_address("192.168.13.226", "1");
		str.clear();
		str = "ConfFile=" + cfg + ";ChangeEndian=FALSE;";
		adaBC.set_config(str); 

		return RET_OK;
}

/*************************************************************
*函数名 ：zynq_bc_init 
*函数功能描述 ：进行 BC 初始化操作，包括模式选择，寄存器配置，
                内存初始化
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_bc_init(void)
{
       U16BIT ret = 0;
			 
		open_device(); 
		ret = adaBC.source_init();
		return ret;
}

/*************************************************************
*函数名 ：zynq_bc_start 
*函数功能描述 ：线程处理函数的设置,启动 core 和线程， core和线
                程开始工作
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_bc_start(void)
{
		 return adaBC.start();
}

/*************************************************************
*函数名 ：zynq_bc_stop 
*函数功能描述 ：停止 core 和线程， core和线程停止工作
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int zynq_bc_stop(void)
{
       U16BIT ret = 0;
			 
		ret = adaBC.stop();
		close_device();
		return ret;
}

/******************************************************************
*函数名 ：zynq_rt_msg_write 
*函数功能描述 ：周期性消息时用于修改或删除数据字的内容，并周期性的发送出去；
                 非周期性消息时用于增加数据字的内容，并插入到周期性里发送出去，
                 数据发送完成后退出。
*函数参数 ：datahead:输入参数，需要修改的消息结构体
*函数返回值 ：成功：RET_OK   失败:RET_ERR
********************************************************************/
int zynq_bc_msg_write(ctl_data_wrd_info *datahead)
{
      return adaBC.write(datahead);
}

/******************************************************************
*函数名 ：zynq_bc_ioctl 
*函数功能描述 ：是否保存某一确定的地址，子地址所对应的数据
*函数参数 ：msg_id:输入参数，消息id,每一组消息都有唯一的消息id去加以区分的
            flag:是否保存消息的标记  save_path:输入参数，保存文件的路径
*函数返回值 ：无
********************************************************************/
void zynq_bc_ioctl(unsigned short msg_id, msg_is_save flag, const char *save_path)
{
		adaBC.ioctl(msg_id, flag,  save_path);
}


	
/*************************************************************
*函数名 ：zynq_rt_set_config 
*函数功能描述 ：此函数用于在 RT 模式下，对配置文件的名字的设定,
                以及配置文件的解析，模块函数调用入口。 
*函数参数 ：cfg:输入参数，配置文件的名字
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_rt_set_config(const std::string cfg)
{
       std::string str;
		adaRT.set_address("192.168.13.226", "0");
		str.clear();
		str = "ConfigFilePath=" + cfg + ";ChangeEndian=false;";
		adaRT.set_config(str); 

		return RET_OK;
}

/*************************************************************
*函数名 ：zynq_rt_init 
*函数功能描述 ：进行 RT 初始化操作，包括模式选择，寄存器配置，
                内存初始化
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_rt_init(void)
{
       U16BIT ret = 0;
       SyncEvent tim_mutex;
			 
		open_device(); 
		ret = adaRT.source_init(&tim_mutex);
		return ret;
}

/*************************************************************
*函数名 ：zynq_rt_start 
*函数功能描述 ：线程处理函数的设置,启动 core 和线程， core和线程开始工作
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_rt_start(void)
{
		 return adaRT.start();
}

/*************************************************************
*函数名 ：zynq_rt_stop 
*函数功能描述 ：停止 core 和线程， core和线程停止工作
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int zynq_rt_stop(void)
{
       U16BIT ret = 0;
			 
		ret = adaRT.stop();
		close_device();
		return ret;
}

/******************************************************************
*函数名 ：zynq_rt_msg_write 
*函数功能描述 ：修改某一确定的子地址所对应的数据字里的内容，主要用
                于RT向BC发数据时，修改数据字里的内容
*函数参数 ：subaddr:输入参数，RT的子地址  data:写入的数据
            len:输入参数，要写入数据的长度
*函数返回值 ：成功：RET_OK   失败:RET_ERR
********************************************************************/
int zynq_rt_msg_write(unsigned short subaddr, const char *data, size_t len)
{
      return adaRT.write(subaddr, data, len);
}

/******************************************************************
*函数名 ：zynq_rt_ioctl 
*函数功能描述 ：是否保存某一确定的地址，子地址所对应的数据
*函数参数 ：subaddr:输入参数，RT的子地址  flag:是否保存消息的标记
            save_path:输入参数，保存文件的路径
*函数返回值 ：无
********************************************************************/
void zynq_rt_ioctl(unsigned short subaddr, msg_is_save flag, const char *save_path)
{
		adaRT.ioctl(subaddr, flag,  save_path);
}


/*************************************************************
*函数名 ：zynq_mt_init 
*函数功能描述 ：MT初始化函数，设置为MT模式，MT模式下相应寄存器
                配置，内存的初始化。
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_mt_init(void)
{
		open_device(); 
		adaMT.set_address("192.168.13.226", "0");
		adaMT.set_config("BIG_ENDIAN=FALSE;");	 
		U16BIT ret  = adaMT.source_init(NULL);
		
		return ret;
}

/*************************************************************
*函数名 ：zynq_mt_start 
*函数功能描述 ：设置MT模式下线程的处理函数，启动core和线程，启动
                成功后core和线程就开始工作。
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int  zynq_mt_start(void)
{
		 return adaMT.start();
}

/*************************************************************
*函数名 ：zynq_rt_stop 
*函数功能描述 ：停止 core 和线程， core和线程停止工作
*函数参数 ：无
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int zynq_mt_stop(void)
{	 
		U16BIT ret = adaMT.stop();
		close_device();
		return ret;
}

/*************************************************************
*函数名 ：zynq_mt_msg_save 
*函数功能描述 ：MT模式下时用于保存所有的消息，并将以数据字，命令
                字，状态字，时间等信息保存于文本文件中
*函数参数 ：  save_path:输入参数，监控的数据存放的位置及名称
*函数返回值 ：成功：RET_OK   失败:RET_ERR
***************************************************************/
int zynq_mt_msg_save(const char *save_path)
{	 
	 return adaMT.msg_save (save_path);
}


   
