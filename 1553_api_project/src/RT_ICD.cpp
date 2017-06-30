#include "RT_ICD.h"

RT_ICD::RT_ICD(void)
{
}

RT_ICD::~RT_ICD(void)
{
}



timeInter_obj::timeInter_obj()
{
	//m_IcdMan = NULL;
	//m_SysServ = NULL;
	//parser = NULL;
	//block = NULL;
	//m_interface = NULL;
	memset((void*)m_timeData,0,32 * sizeof(U16BIT));
}

timeInter_obj::~timeInter_obj()
{

}

/* zhanghao close.
// 设置ICDMan和系统服务
void timeInter_obj::SetIcdManAndServ(const ICD::ICDMan * icd,Agent::ISysServ * sysServ,const Interface * pInterface)
{
	m_IcdMan = icd;
	m_SysServ = sysServ;
	m_interface = pInterface;
	if (NULL == m_interface)
	{
		printf("\t[commu_RT]: pInterface is NULL! \n");	
	}
	if (NULL == m_IcdMan)
	{
		printf("\t [commu_RT]:icd is NULL! \n");	
	}
	if (NULL == m_SysServ)
	{
		printf("\t [commu_RT]:sysServ is NULL! \n");	
	}
}


// 设置接口指针
// 得到块
void timeInter_obj::SetTimeCode(_TimeCode & m_TimeCode)
{
	if ( NULL != m_interface && m_IcdMan != NULL ) 
	{
		std::vector<ICD::Routing *> routings = const_cast<ICDMan*>(m_IcdMan)->MatchRoutes(const_cast<Interface*>(m_interface),
			m_TimeCode.rout_addr,m_TimeCode.rout_subAddr,
			m_TimeCode.rout_des_addr,m_TimeCode.rout_des_subAddr);

		// 这里有多个块的话，应该遍历多个块
		if ( routings.size() == 1 )
		{	
			BlockAttr * blockAttr = const_cast<ICDMan*>(m_IcdMan)->GetBlock(routings[0]);
			block = const_cast<ICDMan*>(m_IcdMan)->GetBlock(blockAttr);

			// 构造
			parser = new BrBasicParser(const_cast<ICDMan*>(m_IcdMan), block, (char*)m_timeData, 32*sizeof(U16BIT));
			if (routings.size() > 1)
			{
				printf("[commu_RT]:time_obj:: routings.size() >1 \n");
			}
		}
		else if(routings.size() > 1)
		{
			// 哪个块下有时间码
			for (int i=0; i< (int) routings.size();i++)
			{
				BlockAttr * blockAttr = const_cast<ICDMan*>(m_IcdMan)->GetBlock(routings[i]);
				block = const_cast<ICDMan*>(m_IcdMan)->GetBlock(blockAttr);

				ICDMan::MemNode::iterator itMem = m_IcdMan->GetMembers(block);
				for( ; itMem != ICDMan::MemNode::end(); itMem++ )
				{
					Field *field = itMem->GetField();
					Parameter *para = field->GetParamPtr();
					if (para->GetParamType() == "BusRobot.ICD.Parameter.Type.TimeBase")
					{
						break;
					}
				}
			}
			// 构造
			parser = new BrBasicParser(const_cast<ICDMan*>(m_IcdMan), block, (char*)m_timeData, 32*sizeof(U16BIT));
			if (routings.size() > 1)
			{
				printf("[commu_RT]:time_obj:: routings.size() >1 \n");
			}
		}
		else
		{
#ifdef _DEBUG
			printf("\t[commu_RT]: Not Find timeCode block! \n");	
			printf("\t [commu_RT]:m_TimeCode info: %s,%s,%s,%s ! \n",m_TimeCode.rout_addr.c_str(),m_TimeCode.rout_subAddr.c_str(),
			m_TimeCode.rout_des_addr.c_str(),m_TimeCode.rout_des_subAddr.c_str());		
#endif
		}
	}
	else
	{
		printf("\t [commu_RT]:set pInterface is NULL! \n");
	}
}
*/

/* zhanghao close.
// 时间码转换,编码,将time编码到pData中
int timeInter_obj::ConvertTime_Code(char * pData,int len,const GIST::BrTime & time)
{
#ifdef _DEBUG
	if (NULL == block)
	{
		printf("\t[commu_RT]: ConvertTime_Code:: block is NULL! \n");	
	}
	if (NULL == m_IcdMan)
	{
		printf("\t[commu_RT]: ConvertTime_Code:: icd is NULL! \n");	
	}
	if (NULL == m_SysServ)
	{
		printf("\t[commu_RT]: ConvertTime_Code:: sysServ is NULL! \n");	
	}
#endif
	if (len > 32 || m_IcdMan == NULL || m_SysServ == NULL || block == NULL)
	{
		return false;
	}
	try
	{
		GIST::BrVariant var_shiptime;
		var_shiptime.set((int64_t)time);

		bool bSucess = false;
		ICDMan::MemNode::iterator itMem = m_IcdMan->GetMembers(block);
		
		for( ; itMem != ICDMan::MemNode::end(); itMem++ )
		{
			Field *field = itMem->GetField();
			Parameter *para = field->GetParamPtr();
			if (para->GetParamType() == "BusRobot.ICD.Parameter.Type.TimeBase")
			{ 
				(*parser)[para] = var_shiptime;

				// 得到时间信息,拷贝两个字
				memcpy((void *)pData,(void *)m_timeData,len);
				bSucess = true;
			}
		}
		if (!bSucess)
		{
#ifdef _DEBUG
			printf("\t ConvertTime_Code fail !\n");	
#endif
		}
	}
	catch (std::exception& e)
	{
		(*pData)=1;
		*(pData+1)=1;
		*(pData+2)=1;
		*(pData+3)=1;
		string strError =e.what();
#ifdef _DEBUG
		printf("[commu_RT error]::SetTimecode:[exception]%s\n", strError.c_str());
#endif
	}	
	return false;
}


// 时间码转换,解码,从pData中解析出time
int timeInter_obj::ConvertTime_Encode(const char * pData,int len,GIST::BrTime & time)
{
#ifdef _DEBUG
	if (NULL == block)
	{
		printf("\t [commu_RT]:ConvertTime_Encode:: block is NULL! \n");	
	}
	if (NULL == m_IcdMan)
	{
		printf("\t[commu_RT]: ConvertTime_Encode::icd is NULL! \n");	
	}
	if (NULL == m_SysServ)
	{
		printf("\t[commu_RT]: ConvertTime_Encode::sysServ is NULL! \n");	
	}
#endif
	if (len > 32 || m_IcdMan == NULL || m_SysServ == NULL || block == NULL )
	{
		return false;
	}
	try
	{
		memset((void *)m_timeData,0,32 * sizeof(U16BIT));
		memcpy((void *)m_timeData,pData,len);
		GIST::BrVariant var_shiptime;

		bool bSucess = false;
		ICDMan::MemNode::iterator itMem = m_IcdMan->GetMembers(block);
		for( ; itMem != ICDMan::MemNode::end(); itMem++ )
		{
			Field *field = itMem->GetField();
			Parameter *para = field->GetParamPtr();
			if (para->GetParamType() == "BusRobot.ICD.Parameter.Type.TimeBase")
			{ 
				var_shiptime = (int64_t)(*parser)[para];
				if(var_shiptime.is_integar())
				{
					time = (GIST::BrTime)var_shiptime.as<int64_t>();
					bSucess = true;
#ifdef _DEBUG
					printf("\t[commu_RT]: ConvertTime_Encode sucess !\n");
#endif
					return true;
				}
			}
		}
		if (!bSucess)
		{
#ifdef _DEBUG
			printf("\t [commu_RT]:ConvertTime_Encode fail !\n");
#endif	
		}

	}
	catch (std::exception& e)
	{
		string strError =e.what();
#ifdef _DEBUG
		printf("[commu_RT error]::SetTimecode:[exception]%s\n", strError.c_str());
#endif
	}

	return false;
}
*/
