#pragma once
#include "Define.h"


// 子地址消息源三取二算法结构
typedef struct _three_struct 
{
	RT_MSG msg;

	_three_Type eType;     // 三取二的类型
	_three_Type_or orStu;  // 按位与
	_three_Type_Array retArry; // 比对
	U16BIT num;			// 一包多值
	U16BIT lastData[3]; //最近三次的数据
	int iIndex; // 当前的索引	
	_three_struct(){
		eType = _three_error;
		iIndex = 0;
		memset((void *)lastData,0,sizeof(U16BIT)*3);
	};
} _three_struct;

/// 三取二数据源类
class ThreeCatch
{
public:
	ThreeCatch(void);
	~ThreeCatch(void);

	// 设置值
	void SetDefVal(U16BIT * pData,S16BIT dataLen);

	// 设置三取二的数据源
	bool SetThreeSource(U16BIT * pData,S16BIT dataLen);

	// 取得比较后的数据
	S16BIT CopyData(U16BIT pData[]);

	// 消息数据源，三取二算法
	_three_struct m_threeStu;

private:

	// 三取二的缓存值,有时有多个三取二结果所以用数组
	U16BIT m_threeBuff[32];

	// 三取二中缓存的长度
	int m_BuffLen;

	// 三取二算法
	U16BIT ThreeGetTwo(U16BIT one,U16BIT one2,U16BIT one3);

	// 判断是否a!=b!=c
	bool IsNoData(U16BIT a,U16BIT b,U16BIT c);

	// 三取二的结果与配置项比对得到结果
	bool GetArrayValue(U16BIT par,U16BIT threeBuff[],int len,const map<U16BIT,ThreeResult> & mapVal);

	// 三取二的结果与配置项与运算得到一个结果
	// 三取二结果，可以设置32个字中的任意一位
	// threeBuff: 32个字的缓存
	// len: 缓存长度
	// mapVal: 比较值与位置的map
	// result_or: 三取二得到的结果,map的key值
	void SetBitValue(const map<U16BIT,int> & mapVal,U16BIT result_or);


	// 将32个字中某一位置1
	void func_bit(long l_bit,bool val);
};
