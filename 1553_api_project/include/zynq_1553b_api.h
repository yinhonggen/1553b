#ifndef __ZYNQ_1553B_API_H__
#define __ZYNQ_1553B_API_H__

#include <string>
using namespace std;

/*是否保存数据的枚举*/
typedef enum _enum_msg_is_saves
{
	ENUM_MSG_SAVE          = 0,  /*保存消息*/
	ENUM_MSG_NOT_SAVE  = 1, /*不保存消息*/
}msg_is_save;

/*操作消息的枚举*/
typedef enum _enum_operate_type
{
	ENUM_DEL_MSG         = 0,     /*删除消息的操作*/
	ENUM_ADD_MSG        = 1,	   /*增加新的消息*/
	ENUM_MODIFY_MSG   = 2,   /*修改消息的操作*/
} enum_operate_type;

/*消息结构体，包括命令字，数据字，状态字的信息*/
typedef struct msg_struct
{
	unsigned short  cmdwrd;              /*命令字*/
	unsigned short  iaddrs;               /*RT地址*/
	unsigned short  trflag;               /*发送与接收*/
	unsigned short	  subaddr;          /*子地址*/
	unsigned short  datalen;          /*数据长度*/
	unsigned short  stswrd;           /*状态字*/
	unsigned short  datawrd[32];   /*数据字*/
}stmsg_struct;

/*需要修改的消息结构体*/
typedef struct ctl_data_wrd_info
{
       enum_operate_type type;
	size_t msg_lenth;
	unsigned short msg_id;
	unsigned short pos;
	char *data;
} ctl_data_wrd_info;

#define RET_OK     0  /*成功*/
#define RET_ERR   1  /*失败*/

/*收数据实时性相关函数*/
typedef  void (*ZYNQ_MSG_CALLBACK)(stmsg_struct *pmsg);
extern void zynq_reg_func(ZYNQ_MSG_CALLBACK pfunc);
/*BC相关的操作函数*/
extern int  zynq_bc_set_config(const std::string cfg);
extern int  zynq_bc_init(void);
extern int  zynq_bc_start(void);
extern int zynq_bc_stop(void);
extern int zynq_bc_msg_write(ctl_data_wrd_info *datahead);
extern void zynq_bc_ioctl(unsigned short msg_id, msg_is_save flag, const char *save_path);
/*RT相关的操作函数*/
extern int zynq_rt_set_config(const std::string cfg);
extern int zynq_rt_init(void);
extern int zynq_rt_start(void);
extern int zynq_rt_stop(void);
extern int zynq_rt_msg_write(unsigned short subaddr, const char *data, size_t len);
extern void zynq_rt_ioctl(unsigned short subaddr, msg_is_save flag, const char *save_path);
/*MT相关的操作函数*/
extern int  zynq_mt_init(void);
extern int  zynq_mt_start(void);
extern int zynq_mt_stop(void);
extern int zynq_mt_msg_save(const char *save_path);

#endif
