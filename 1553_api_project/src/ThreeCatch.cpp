#include "ThreeCatch.h"
#include "common_qiu.h"
#include <bitset>
using namespace std;

ThreeCatch::ThreeCatch(void)
{
	m_BuffLen = 32;
	memset((void*)m_threeBuff,0,32 * sizeof(U16BIT));
}

ThreeCatch::~ThreeCatch(void)
{

}


S16BIT ThreeCatch::CopyData(U16BIT pData[])
{
	memcpy((void *)pData,(void *)m_threeBuff,sizeof(U16BIT)*m_BuffLen);
	return m_BuffLen;
}

// 设置值
void ThreeCatch::SetDefVal(U16BIT * pData,S16BIT dataLen)
{
	memcpy((void *)m_threeBuff,(void *)pData,sizeof(U16BIT) * dataLen);
}

/*
当A≠B≠C时，返回数据规则如下： 
对于三取二返回指令，字长依据消息规定字长，数据部分返回默认值（填零或其他）；
对于三取二按位修改返回值，返回默认值，或者上次接收正确三取二指令时返回的结果；
对于三取二返回匹配数组，字长依据消息规定字长。返回匹配数组内容为默认值（填零或其他）。
*/
// 设置三取二的数据源
bool ThreeCatch::SetThreeSource(U16BIT * pData,S16BIT dataLen)
{
	if (dataLen >32 )
	{
		return false;
	}
	switch (m_threeStu.eType)
	{
	case _three_error:
		return false;
	case _three_INS:
		// 一条消息内有多个指令
		{
			if (m_threeStu.num*3 >32)
			{
				return false;
			}
			U16BIT a,b,c,tNum = m_threeStu.num;
			for (int i=0;i<tNum;i++)
			{
				a = *(pData +i);
				b = *(pData + i + tNum);
				c = *(pData + i + tNum*2);
				m_threeBuff[i] = ThreeGetTwo(a,b,c);
			}
		
			m_BuffLen = tNum;
		}
		break;
	case _three_or:
		{
			// 按位与运算,三取二的结果和配置的值比较,得到位置,用这个位置与运算
			U16BIT a = (*pData);
			U16BIT b = *(pData +1);
			U16BIT c = *(pData +2);

			if(!IsNoData(a,b,c))//a!=b!=c
			{
				U16BIT ret = ThreeGetTwo(a,b,c);
				
				SetBitValue(m_threeStu.orStu.mapVal,ret);
			}
			m_BuffLen = 32;
		}
		break;
	case _three_Array:
		{
			// 三取二返回数组
			m_threeStu.lastData[0] = pData[0];
			m_threeStu.lastData[1] = pData[1];
			m_threeStu.lastData[2] = pData[2];
			if(IsNoData(pData[0],pData[1],pData[2]))//a!=b!=c
			{
				return false;
			}	
			
			U16BIT ret = ThreeGetTwo(m_threeStu.lastData[0],m_threeStu.lastData[1],m_threeStu.lastData[2]);

			// 比较得到另一组值
			if (m_threeStu.retArry.che)
			{
				// 比较得到另一个值
				return GetArrayValue(ret ,m_threeBuff,m_BuffLen,m_threeStu.retArry.listVal);
			}
			else
			{
				m_threeBuff[0]= ret;
			}

			m_BuffLen = 32;
		}
		break;
	}
	return true;
}

// 三取二的结果与配置项比对得到数组
// 没有比对值，返回false
bool ThreeCatch::GetArrayValue(U16BIT par,U16BIT threeBuff[],int len,const map<U16BIT,ThreeResult> & mapVal)
{
	map<U16BIT,ThreeResult>::const_iterator itList = mapVal.find(par);
	if(itList != mapVal.end())
	{
		memcpy((void*)threeBuff,(void *)(itList->second.dataArr),sizeof(U16BIT)*32);
		return true;
	}
	else
	{
		return false;
	}
}

// 三取二的结果和配置项与运算得到一个结果
void ThreeCatch::SetBitValue(const map<U16BIT,int> & mapVal,U16BIT resultT)
{
	map<U16BIT,int>::const_iterator itVal = mapVal.find(resultT);

	if( itVal != mapVal.end())
	{
		// 与运算得到结果
		int iVal = itVal->second;
		// 设置32个字中任意一位
		func_bit(iVal,1);
	}
}

void ThreeCatch::func_bit(long l_bit,bool val)
{
	if (l_bit <= 0)
	{
		return ;
	}
	if (l_bit > 512)
	{
		return;
	}

	long idx = (l_bit + 15) / 16 - 1;
	long pos = (l_bit + 15) % 16;
	bitset<16> b16(m_threeBuff[idx]);
	b16.set(pos,val);

	ULONG new_val = b16.to_ulong();
	m_threeBuff[idx] = (WORD)new_val;
}

// 判断是否a!=b!=c
bool ThreeCatch::IsNoData(U16BIT a,U16BIT b,U16BIT c)
{
	if (a != b && a != c && b != c)
	{
		return true;
	}
	return false;
}

// 三取二算法
U16BIT ThreeCatch::ThreeGetTwo(U16BIT a,U16BIT b,U16BIT c)
{
	if (a == b && a == c && b == c)
	{
		return a;
	}
	else
	{
		if (a == b && b != c)
		{
			return a;
		}
		if (b == c && a != c)
		{
			return b;
		}
		if (a == c && b != c)
		{
			return a;
		}

		return 0;
	}
}




