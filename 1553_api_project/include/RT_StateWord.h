#pragma once
#include "Define.h"

// RT的状态字功能
class RT_StateWord
{
public:
	RT_StateWord(void);
	~RT_StateWord(void);

public:

	// 服务请求位
	int ServiceRequest;

	// 子系统特征位
	int SubSystem_Flag ;

	// 忙位
	int Busy_flag;

	// 终端特征位
	int Terminal_Flag;
private:

	// RT的地址
	S16BIT  m_RTAddr;

public:

	// 设置RT地址
	int SetRtAddr(S16BIT rtAddr);

	// 得到状态字
	int GetStaus(S16BIT & wStatus);

	// 重置服务请求位
	void ResetSerRequest();
};
