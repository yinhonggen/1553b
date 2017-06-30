#include "ConfigSplit_v.h"
#include "utility.h"
#include <stdlib.h>
using namespace RT;

// 提供一些帮助解析用的函数
class SplitHelp
{
public:
	// 解析:{2_2_0:[1,3,4],3_2_0:[1,3,4]}
	void static SubData_msgCopy(string str,vector< std::pair<string,string> > & pairStrVector);

	// 解析:{2_2_0:[1,3,4],3_2_0:[1,3,4]}
	void static SubData_chgMsg(string str,vector< std::pair<string,string> > & pairStrVector);

	// 解析:{2:[32],3:[32]}
	void static SubData_subAddr(string str,vector< std::pair<string,string> > & pairStrVector);

	// 解析:1[1,b,c],2[1,b,c],3[1,b,c],4[1,b,c]
	void static SubDataby_ddd(string str,vector<string> & vecStr);

	// 解析 10,32,4
	void static GetMoreSubAddr(string str,vector<S16BIT> & vecS);

	// 从一行配置的子地质信息得到，培植的周期
	bool static SplitLine_sub(_CycleIndex & cycle,S16BIT & subAddr ,string str);

	// 
	string static GetVecStr(const vector<S16BIT> & vec);
protected:
private:
};

// 解析:{2:[32],3:[32]}
void SplitHelp::SubData_subAddr(string str,vector< std::pair<string,string> > & pairStrVector)
{
	SubData_msgCopy(str,pairStrVector);
	return ;
}

// 解析:{2_2_0:[1,3,4],3_2_0:[1,3,4]}
void SplitHelp::SubData_chgMsg(string str,vector< std::pair<string,string> > & pairStrVector)
{
	SubData_msgCopy(str,pairStrVector);
	return ;
}


string SplitHelp::GetVecStr(const vector<S16BIT> & vec)
{		
	string strSubAddr;
	for (int i=0;i<(int)vec.size();i++)
	{
		if (strSubAddr.length() >0)
		{
			strSubAddr += ",";
		}
		char buff[10];
		memset(buff,0,10);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,"sa%d",(int)vec.at(i));
#ifdef WIN32
#pragma warning( pop )
#endif
		strSubAddr += buff;
	}
	return strSubAddr;
}
// 解析:{2_2_0:[1,3,4],3_2_0:[1,3,4]}
void SplitHelp::SubData_msgCopy(string str,vector< std::pair<string,string> > & pairStrVector)
{
	if (str.length() <= 2)
	{
		return ;
	}
	str.erase(0,1);
	str.erase(str.length()-1,1);

	vector<string > vecStr;
	SubDataby_ddd(str,vecStr);

	for (size_t i=0;i<vecStr.size(); i++)
	{
		string strSub = vecStr.at(i);

		vector<string> vecSubSub;
		SubData(strSub,vecSubSub,':');

		// 存入pairStr中
		if (vecSubSub.size() == 0)
		{
			continue;
		}
		if( vecSubSub.size() == 1)
		{	
			std::pair<string,string> pairNode(vecSubSub.at(0),"");
			pairStrVector.push_back(pairNode);
		}
		if (vecSubSub.size() == 2)
		{
			string subAddrArr = vecSubSub.at(1);

			// 移除'[',']'
			if (subAddrArr.length() >= 2)
			{
				subAddrArr.erase(0,1);
				subAddrArr.erase(subAddrArr.length()-1,1);
			}
			else
			{
				printf("\t[commu_RT error] : config error (SubData_more)! \n");
			}

			std::pair<string,string> pairNode(vecSubSub.at(0),subAddrArr);
			pairStrVector.push_back(pairNode);
		}		
	}
}


// 解析:1:[1],2,3:[1,b,c],4:[1,b,c]
// 得到:  1:[2] 2 3:[1,b,c] 4:[1,b,c]
void SplitHelp::SubDataby_ddd(string str,vector<string> & vecStr)
{
	char cc = ',';
	int pos = 0;
	int iFind = str.find(cc);

	while(iFind > 0)
	{
		string sub = str.substr(pos,iFind - pos);

		if (sub.find('[') != str.npos && sub.find(']') == str.npos)
		{
			iFind = str.find(']',iFind);
			if (iFind > 0)
			{
				sub = str.substr(pos,iFind - pos+1);
				vecStr.push_back(sub);
				iFind++;
			}
			else
			{
				return ;
			}
		}
		else
		{
			vecStr.push_back(sub);
		}

		pos = iFind + 1;
		iFind = str.find(cc,pos);
	}

	if ((int)str.length() > pos )
	{
		string sub = str.substr(pos,str.length() - pos);

		if (sub.find('[') != str.npos && sub.find(']') == str.npos)
		{
			int iFind_2 = str.find(']',iFind>0?iFind:0);
			if (iFind_2 > 0)
			{
				sub = str.substr(pos,iFind_2 - pos+1);
				vecStr.push_back(sub);
			}
			else
			{
				return ;
			}
		}
		else
		{
			vecStr.push_back(sub);
		}
	}
}

// 解析 10,32,4
void SplitHelp::GetMoreSubAddr(string str,vector<S16BIT> & vecS)
{
//	if (str.length() >= 0)
	{
		vector<string> vecStrAddr;

		SubData(str,vecStrAddr,',');

		vector<string>::iterator subIt = vecStrAddr.begin();

		while (subIt != vecStrAddr.end())
		{
			S16BIT subAddr = (int)atol_my(subIt->c_str());
			vecS.push_back(subAddr);
			subIt ++;
		}
	}
}

// 从一行配置的子地质信息得到，培植的周期
bool SplitHelp::SplitLine_sub(_CycleIndex & cycle,S16BIT & subAddr ,string str)
{
	vector<string> vecStr_Sub;
	SubData(str,vecStr_Sub,'_');
	if (3 != vecStr_Sub.size())
	{
		return false;
	}

	subAddr = (S16BIT)atol_my(vecStr_Sub.at(0).c_str()); // 子地址
	long lCyle = atol_my(vecStr_Sub.at(1).c_str());				// 周期
	long updataLen = atol_my(vecStr_Sub.at(2).c_str());			// 数据字长度
	if (updataLen >32 || updataLen <= 0 )
	{
		updataLen = 32;
	}
	cycle.cycle = lCyle;
	cycle.cyleIndex = 1;
	cycle.updataLen =(S16BIT) updataLen;
	return true;
}


bool RTconfigSeq_v::LoadFile(const string & fileName)
{
	// 读入文件内容
	this->clear();
	m_StrConfig.clear();
	m_config.clear();
	char buff[1024];

	FILE * pFile;

	string strFullPath = "agent.rc/" + fileName;
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	pFile = fopen(strFullPath.c_str(),"r");
#ifdef WIN32
#pragma warning( pop )
#endif

	if (NULL == pFile )
	{
		return false;
	}

	int iRead = fread(buff,sizeof(char),1024,pFile);
	while(iRead > 0)
	{
		string strLine(buff,iRead);
		m_StrConfig += strLine;
		iRead = fread(buff,sizeof(char),1024,pFile);
	}
	fclose(pFile);
	return LoadConfigStr(m_StrConfig);
}

RTconfigSeq_v::RTconfigSeq_v()
{

}
bool RTconfigSeq_v::LoadConfigStr(const std::string &cfg)
{
	try
	{
		string cfg_n;
		if (cfg.length() != 0)
		{
			cfg_n = cfg;
		}
		else
		{
			printf("\t[commu_RT error]: set_config error !\n");
			return false;
		}
		m_config.SetConfig(cfg_n);
	}
	catch(...)
	{
		printf("[commu_RT :ERROR] : Set_config error : loadConfigStr exp !\n");
		return false;
	}
	return true;
}

void RTconfigSeq_v::clear()
{
	m_config.clear();
}
//
void RTconfigSeq_v::GetChgMsgData(__out map<RT_MSG, vector<S16BIT> > & m_chgMsg_subAddr)
{
	string chgMsg_dArr;
	m_config.get_item("CHG_msg_data",chgMsg_dArr);

	// 解析出配置的多个数据变化消息
	if(chgMsg_dArr.length() > 0)
	{
		vector< std::pair<string,string> > pairStrVector;
		SplitHelp::SubData_chgMsg(chgMsg_dArr,pairStrVector);

		for (size_t i=0;i<pairStrVector.size();i++)
		{
			string strMsg = pairStrVector.at(i).first;
			string strSub = pairStrVector.at(i).second;
			RT_MSG msg(strMsg);

			// 配置的消息, 配置的子地址
			map<RT_MSG,vector<S16BIT> >::iterator itSub_Msg = m_chgMsg_subAddr.find(msg);
			if (itSub_Msg == m_chgMsg_subAddr.end())
			{
				//这里有多个子地址
				vector<S16BIT> vecSubAddr;
				SplitHelp::GetMoreSubAddr(strSub,vecSubAddr);
				if (vecSubAddr.size() > 0)
				{
					m_chgMsg_subAddr[msg] = vecSubAddr;
				}
			}
			else
			{
				// 配置重复。。
				printf("\t[commu_RT error]: config error(Resource_copy)! \n");
			}
		}
	}
	return ;
}

void RTconfigSeq_v::GetChgMsgState(__out set<RT_MSG> & m_chgMsg_subState)
{
	string chgMsg_sArr;
	m_config.get_item("CHG_msg_state",chgMsg_sArr);
	// 解析出配置的多个状态位变化消息
	if(chgMsg_sArr.length() >= 2)
	{
		vector<string> vecStr;
		chgMsg_sArr.erase(0,1);
		chgMsg_sArr.erase(chgMsg_sArr.length()-1,1);
		SubData(chgMsg_sArr,vecStr,',');

		for (size_t i=0;i<vecStr.size();i++)
		{
			string str = vecStr.at(i);
			RT_MSG msg(str);

			set<RT_MSG>::iterator it = m_chgMsg_subState.find(msg);
			if (it == m_chgMsg_subState.end())
			{
				m_chgMsg_subState.insert(msg);
			}
		}
	}
	return ;
}

void RTconfigSeq_v::GetResourceCopy(__out map<RT_MSG,vector<S16BIT> > & m_ResourceCopy_subAddr)
{
	string Resource_copy;
	m_config.get_item("Resource_copy",Resource_copy);
	// 解析出配置的多个拷贝源消息
	if(Resource_copy.length() > 0)
	{
		vector< std::pair<string,string> > pairStrVector;
		SplitHelp::SubData_msgCopy(Resource_copy,pairStrVector);

		for (size_t i=0;i<pairStrVector.size();i++)
		{
			string strMsg = pairStrVector.at(i).first;
			string strSub = pairStrVector.at(i).second;
			RT_MSG msg(strMsg);

			// 配置的消息, 配置的子地址
			map<RT_MSG,vector<S16BIT> >::iterator itSub_msg = m_ResourceCopy_subAddr.find(msg);
			if (itSub_msg == m_ResourceCopy_subAddr.end())
			{
				//这里有多个子地址
				vector<S16BIT> vecSubAddr;
				SplitHelp::GetMoreSubAddr(strSub,vecSubAddr);

				if (vecSubAddr.size() > 0)
				{
					m_ResourceCopy_subAddr[msg] = vecSubAddr;
				}
			}
			else
			{
				// 配置重复。。
				printf("\t[commu_RT error]: config error(Resource_copy)! \n");
			}

		}
	}
	return ;
}


void RTconfigSeq_v::GetStateWord(__out RT_StateWord & stateWord)
{
	string strTemp_sr;
	string strTemp_sf;
	string strTemp_b;
	string strTemp_tf;
	m_config.get_item("ServiceRequest", strTemp_sr);
	m_config.get_item("SubSystem_Flag", strTemp_sf);
	m_config.get_item("Busy", strTemp_b);
	m_config.get_item("Terminal_Flag", strTemp_tf);

	// 状态字标识位
	if (strTemp_sr.length() > 0)
	{
		stateWord.ServiceRequest = atol_my(strTemp_sr.c_str());
	}
	if (strTemp_sf.length() >0)
	{
		stateWord.SubSystem_Flag = atol_my(strTemp_sf.c_str());
	}

	if (strTemp_b.length() > 0)
	{
		stateWord.Busy_flag = atol_my(strTemp_b.c_str());
	}
	if (strTemp_tf.length() > 0)
	{
		stateWord.Terminal_Flag = atol_my(strTemp_tf.c_str());
	}
}

long RTconfigSeq_v::getCyleState()
{
	string strCycle;
	m_config.get_item("CHG_cyle_state",strCycle);
	return atol_my(strCycle.c_str());
}

string RTconfigSeq_save::GetConfigStr_f(const _fileConfig & config,S16BIT subAddr)
{
	string strFile;
	int filelen = config.fileName.length();
	char* buff = new char[100];
	memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	sprintf(buff,"sa%d:%s",(int)subAddr,config.fileName.c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
	strFile = buff;

	if (config.bAffect)
	{
		memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,":%d",config.defWord);
#ifdef WIN32
#pragma warning( pop )
#endif
		strFile += buff;

		memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,":sim%d",config.bagBag.GetLen());
#ifdef WIN32
#pragma warning( pop )
#endif
		strFile += buff;

		vector<S16BIT> vecSubLen;
		config.bagBag.GetSublen(vecSubLen);
		for (int i=0;i<(int)vecSubLen.size();i++)
		{
			memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"_%d",vecSubLen.at(i));
#ifdef WIN32
#pragma warning( pop )
#endif
			strFile += buff;
		}
	}
	delete [] buff;
	buff = NULL;
	return strFile;
}

// 配置信息
void RTconfigSeq_save::GetThreeChg_or_s(S16BIT subAddr,string & str_orStr,string & str_config_orName,map<U16BIT,int> & mapVal)
{
	if (mapVal.size() > 0)
	{
		map<U16BIT,int>::iterator itMap = mapVal.begin();
		while (itMap != mapVal.end())
		{
			if (str_orStr.length() > 0)
			{
				str_orStr += ",";
			}

			char buff_m[100];
			memset(buff_m,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff_m,"0x%x_%d",itMap->first,itMap->second);
#ifdef WIN32
#pragma warning( pop )
#endif
			str_orStr += buff_m;

			itMap++;
		}
	}
	if (str_orStr.length() >0)
	{
		char buff_s[100];
		memset(buff_s,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff_s,"ThreeChg_or_sa%d",subAddr);
#ifdef WIN32
#pragma warning( pop )
#endif
		str_config_orName = buff_s;

		str_orStr = "={" + str_orStr;
		str_orStr = str_orStr  + "};";
		str_orStr = str_config_orName + str_orStr;
	}
}

// 配置信息
void RTconfigSeq_save::GetThreeChg_end_s(S16BIT subAddr,string & str_orStr,string & str_config_orName,map<U16BIT,ThreeResult> & mapVal)
{
	if (mapVal.size() > 0)
	{
		map<U16BIT,ThreeResult>::iterator itMap = mapVal.begin();
		while (itMap != mapVal.end())
		{
			if (str_orStr.length() > 0)
			{
				str_orStr += ",";
			}

			char buff_m[300];
			memset(buff_m,0,300);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff_m,"0x%x_%s",itMap->first,itMap->second.getLineStr().c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
			str_orStr += buff_m;

			itMap++;
		}
	}
	if (str_orStr.length() >0)
	{
		char buff_s[100];
		memset(buff_s,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff_s,"ThreeChg_end_sa%d",subAddr);
#ifdef WIN32
#pragma warning( pop )
#endif
		str_config_orName = buff_s;

		str_orStr = "={" + str_orStr;
		str_orStr = str_orStr  + "};";
		str_orStr = str_config_orName + str_orStr;
	}
}




// 得到每个子地址对应的配置
bool RTconfigSeq_v::GetRT_SAConfig(map<S16BIT,SA_Config> & map_SaInfo)
{
	// 子地址
	string strSubAddressArr;
	// 状态周期
	string strCycle ;
	// 消息变化
	string chgMsg_dArr;
	string chgMsg_sArr;
	//文件数据源
	string Resource_file;
	string Resource_copy;
	// 时间码
	string strTimeCode;
	// 需要取走后刷新的
	string strForceFlush;

	m_config.get_item("Sub_Address",strSubAddressArr);
	// 解析出配置的多个子地址,配置的文件数据源
	if (strSubAddressArr.length() >0)
	{
		string strThreeChg; // 三取二配置项

		m_config.get_item("Resource_file",Resource_file);
		m_config.get_item("TimeCode",strTimeCode);
		m_config.get_item("ThreeChg",strThreeChg);

		map<S16BIT,_TimeCode> m_mapTimeCode;
		map<S16BIT,_three_struct> m_mapThreeStu;

		// 三取二配置
		GetThreeChg(strThreeChg,m_mapThreeStu);

		// 得到时间码
		GetTimeCode(strTimeCode,m_mapTimeCode);	

		// 子地址配置信息
		vector<string> strVector;
		SubData(strSubAddressArr,strVector,',');
		if (strVector.size() == 0){  //没有配置子地址
			return false;
		}
		for (size_t i=0;i<strVector.size();i++)
		{
			string strLine = strVector.at(i);

			// sa1:[len30]:[che1]:[cheIndex37] 
			vector<string> subVec;
			SA_Config saCfg;
			SubData(strLine,subVec,':');
			if (subVec.size() == 1)
			{
				string strSubAddr = subVec.at(0);
				string strDataLen = "32";
				saCfg.saAddr = (S16BIT)atol_my(strSubAddr.c_str()); // 子地址
				saCfg.wordLen = (S16BIT)atol_my(strDataLen.c_str()); // 子地址数据长度
				saCfg.IsBigWord = false; // 是否大端
			}
			else if (subVec.size() == 2)
			{
				string strSubAddr = subVec.at(0);
				string strDataLen = subVec.at(1);
				saCfg.saAddr = (S16BIT)atol_my(strSubAddr.c_str()); // 子地址
				saCfg.wordLen= (S16BIT)atol_my(strDataLen.c_str()); // 子地址数据长度
				saCfg.IsBigWord = false; // 是否大端
			}
			else if (subVec.size() == 3)
			{
				string strSubAddr = subVec.at(0);
				string strDataLen = subVec.at(1);
				string strBigWord = subVec.at(2);
				saCfg.saAddr = (S16BIT)atol_my(strSubAddr.c_str()); // 子地址
				saCfg.wordLen= (S16BIT)atol_my(strDataLen.c_str()); // 子地址数据长度
				S16BIT bigWord =(S16BIT)atol_my(strBigWord.c_str()); // 是否大端
				saCfg.IsBigWord = bigWord==1?true:false;
			}
			else if (5 == subVec.size())
			{
				string strSubAddr = subVec.at(0);
				string strDataLen = subVec.at(1);
				string strBigWord = subVec.at(2);
				saCfg.saAddr = (S16BIT)atol_my(strSubAddr.c_str()); // 子地址
				saCfg.wordLen = (S16BIT)atol_my(strDataLen.c_str()); // 子地址数据长度
				S16BIT bigWord =(S16BIT)atol_my(strBigWord.c_str()); // 是否大端
				saCfg.IsBigWord = bigWord==1?true:false;

				// 校验方式,校验字的位置	
				string strChe = subVec.at(3);  // 校验方式
				string strCheIndex = subVec.at(4); // 校验值所在位置
				saCfg.theckOut.type = (enum_Check)atol_my(strChe.c_str());
				saCfg.theckOut.wordIndex = (S16BIT)atol_my(strCheIndex.c_str());
			}

			map_SaInfo[saCfg.saAddr] = saCfg;
		}

		// 文件配置信息, 配置的文件数据源
		map<string,int> mapFileName;
		vector<string> vecStr;
		if (Resource_file.length() > 2)
		{
			Resource_file.erase(0,1);
			Resource_file.erase(Resource_file.length()-1,1);
			SubData(Resource_file,vecStr,',');
			for (size_t i=0;i<vecStr.size();i++)
			{
				string str = vecStr.at(i);
				vector<string> vecStr_Sub;
				// sa1:fileName.data:def0000:sim67_30_30_7
				SubData(str,vecStr_Sub,':');
				if ( vecStr_Sub.size() < 2)
				{
					continue;
				}
				else if (vecStr_Sub.size() == 2)
				{
					S16BIT subAddr = (S16BIT)atol_my(vecStr_Sub.at(0).c_str()); // 子地址

					map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
					if (itSa != map_SaInfo.end())  // 判断是否有该地址
					{
						map_SaInfo[subAddr].fileCfg.fileName = vecStr_Sub.at(1);
						map_SaInfo[subAddr].fileCfg.bAffect = false;
					}
				}
				else if (vecStr_Sub.size() == 3)
				{
					// 没有小包信息，使用文件作为默认值。
					S16BIT subAddr = (S16BIT)atol_my(vecStr_Sub.at(0).c_str()); // 子地址

					map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
					if (itSa != map_SaInfo.end())  // 判断是否有该地址
					{
						map_SaInfo[subAddr].fileCfg.fileName= vecStr_Sub.at(1);
						map_SaInfo[subAddr].fileCfg.bAffect = false;
					}
				}
				else if (vecStr_Sub.size() == 4)
				{
					S16BIT subAddr = (S16BIT)atol_my(vecStr_Sub.at(0).c_str()); // 子地址
					S16BIT defWord = (S16BIT)atol_my(vecStr_Sub.at(2).c_str()); //默认值
					string simStr = vecStr_Sub.at(3).c_str(); //文件中的小包

					// 这里要比较小包是否有效
					SimlBag fileBag ;
					if (fileBag.SplitBag(simStr))
					{
						map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
						if (itSa != map_SaInfo.end())  // 判断是否有该地址
						{
							map_SaInfo[subAddr].fileCfg.defWord = defWord;
							map_SaInfo[subAddr].fileCfg.fileName = vecStr_Sub.at(1);
							map_SaInfo[subAddr].fileCfg.bAffect = true;
							map_SaInfo[subAddr].fileCfg.bagBag = fileBag;
						}
					}
					else
					{
						map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
						if (itSa != map_SaInfo.end())  // 判断是否有该地址
						{
							map_SaInfo[subAddr].fileCfg.fileName = vecStr_Sub.at(1);
							map_SaInfo[subAddr].fileCfg.bAffect = false;
						}
					}
				}
				else if(vecStr_Sub.size() == 5)
				{
					// 有小包，是文件默认值。
					S16BIT subAddr = (S16BIT)atol_my(vecStr_Sub.at(0).c_str()); // 子地址
					S16BIT defWord = (S16BIT)atol_my(vecStr_Sub.at(2).c_str()); //默认值
					string simStr = vecStr_Sub.at(3).c_str(); //文件中的小包

					// 这里要比较小包是否有效
					SimlBag fileBag ;
					if (fileBag.SplitBag(simStr))
					{
						map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
						if (itSa != map_SaInfo.end())  // 判断是否有该地址
						{
							map_SaInfo[subAddr].fileCfg.defWord  = defWord;
							map_SaInfo[subAddr].fileCfg.fileName = vecStr_Sub.at(1);
							map_SaInfo[subAddr].fileCfg.bAffect = true;
							map_SaInfo[subAddr].fileCfg.bagBag = fileBag;
						}
					}
					else
					{
						map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
						if (itSa != map_SaInfo.end())  // 判断是否有该地址
						{
							map_SaInfo[subAddr].fileCfg.fileName = vecStr_Sub.at(1);
							map_SaInfo[subAddr].fileCfg.bAffect = false;
						}
					}
				}
				else
				{
					continue;
				}
			}
		}

		////////////////////////////////////////////////////////////
		// 默认值
		map<S16BIT,SA_Config>::iterator subAddrIt = map_SaInfo.begin();
		while (subAddrIt != map_SaInfo.end())
		{
			char buff[10];
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"%d",(int)(subAddrIt->first));
#ifdef WIN32
#pragma warning( pop )
#endif
			string strKey = "SA";
			strKey += buff;

			//取出,是否配置了默认值
			string strDefValue;
			m_config.get_item(strKey,strDefValue);
			if (strDefValue.length() > 0)
			{
				// 得到子地质的默认值
				if (strDefValue.length() <=2) continue;
				strDefValue.erase(0,1);
				strDefValue.erase(strDefValue.length()-1,1);

				vector<string> defVec;
				SubData(strDefValue, defVec,',');

				// 复制
				for (size_t i =0 ;i<defVec.size();i++)
				{
					if(i> 32) break;
					long val = atol_my_Hex(defVec.at(i).c_str());
					subAddrIt->second.defDataArry[i] =(S16BIT) val;
				}
			}
			subAddrIt ++;
		}

		m_config.get_item("ForceFlush",strForceFlush);
		// 需要强制刷新的
		if (strForceFlush.length() > 0)
		{
			vector<string> vecStr;
			SubData(strForceFlush,vecStr,',');
			for (size_t i=0;i<vecStr.size();i++)
			{
				S16BIT subAddr = (S16BIT)atol_my(vecStr.at(i).c_str());

				map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
				if (itSa != map_SaInfo.end())  // 判断是否有该地址
					map_SaInfo[subAddr].m_bForceFlush =true;
			}
		}

		// 时间码
		map<S16BIT,_TimeCode>::iterator itTime = m_mapTimeCode.begin();
		while (itTime != m_mapTimeCode.end())
		{
			map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(itTime->first);
			if (itSa != map_SaInfo.end())  // 判断是否有该地址
				map_SaInfo[itTime->first].timeCode = itTime->second;
			itTime++;
		}

		// 三取二
		map<S16BIT,_three_struct>::iterator itThree = m_mapThreeStu.begin();
		while (itThree != m_mapThreeStu.end())
		{
			map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(itThree->first);
			if (itSa != map_SaInfo.end())  // 判断是否有该地址
				map_SaInfo[itThree->first].threeObj = itThree->second;
			itThree++;
		}

		// 消息拷贝
		{

		}

		// 变化周期
		string chgCyle_dArr ;
		m_config.get_item("CHG_cyle_data",chgCyle_dArr);
		if (chgCyle_dArr.length()>2)
		{
			vector<string> vecStr;
			chgCyle_dArr.erase(0,1);
			chgCyle_dArr.erase(chgCyle_dArr.length()-1,1);
			SubData(chgCyle_dArr,vecStr,',');

			for (size_t i=0;i<vecStr.size();i++)
			{
				string str = vecStr.at(i);
				_CycleIndex cycle;
				S16BIT subAddr = -1;
				bool bSucess = SplitHelp::SplitLine_sub(cycle,subAddr,str);
				if (bSucess)
				{
					map<S16BIT,SA_Config>::iterator itSa= map_SaInfo.find(subAddr);
					if (itSa != map_SaInfo.end())  // 判断是否有该地址
						map_SaInfo[subAddr].m_CyleInfo = cycle; // 子地址的变化周期				
				}
			}
		}
	}
	return true;
}

// 拼接

// 输出:
// string & Sub_Address: Sub_Address = {sa1:[num10]:[che1]:[cheIndex11],SA2,SA3,SA4,SA5};
// string & Resource_file:  Resource_file = {SA1:fileName:def0000:sim67_30_30_7,SA2:fileName};
// string & ForceFlush:  ForceFlush={sa1,sa2 };
// vector<string> & subAddrDefval:  默认值的配置字符串
// 将配置信息保存
string RTconfigSeq_save::SaveRT_SAConfig(const map<S16BIT,SA_Config> & map_SaInfo)
{
	string subAddrContent;
	// 需要取走后刷新的
	string strForceFlush;
	string fileContent;
	string strTimeConfig ; 
	string strThreeCfg; // 三取二算法配置项
	string strThree_or_cfg; //
	string strThree_end_cfg; // 
	vector<string> subAddrDefval;//

	// 周期变化
	string strCyleContent;

	for (map<S16BIT,SA_Config>::const_iterator itSub = map_SaInfo.begin() ; itSub != map_SaInfo.end() ; itSub++)
	{
		S16BIT subAddr = itSub->first;
		SA_Config subData = itSub->second;
		S16BIT cheType = subData.theckOut.type,cheIndex= subData.theckOut.wordIndex;
		if (subAddrContent.length() > 0)
		{
			subAddrContent += ",";
		}
		char buff[100];
		memset(buff,0,100);
		if (_ERROR == cheType)
		{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"sa%d:num%d:BigWord%d",subAddr,subData.wordLen,subData.IsBigWord);
#ifdef WIN32
#pragma warning( pop )
#endif
		}
		else
		{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"sa%d:num%d:BigWord%d:che%d:cheIndex%d",subAddr,subData.wordLen,subData.IsBigWord,cheType,cheIndex);
#ifdef WIN32
#pragma warning( pop )
#endif
		}
		subAddrContent += buff;

		// 取走后刷新
		if (subData.m_bForceFlush)
		{
			if (strForceFlush.length()>0)
			{
				strForceFlush += ",";
			}
			char buff[10];
			memset(buff,0,10);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"sa%d",subAddr);
#ifdef WIN32
#pragma warning( pop )
#endif
			strForceFlush += buff;
		}

		// 周期刷新
		if(subData.m_CyleInfo.cycle > 0)
		{
			if (strCyleContent.length() >0)
			{
				strCyleContent += ",";
			}
			char buff[100];
			memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"sa%d_ms%d_len%d",(int)subAddr,(int)subData.m_CyleInfo.cycle,(int)subData.m_CyleInfo.updataLen);
#ifdef WIN32
#pragma warning( pop )
#endif
			strCyleContent += buff;
		}

		// 文件配置项
		if (subData.fileCfg.fileName.length() > 0)
		{
			if (fileContent.length() > 0)
			{
				fileContent+= ",";
			}
			string strFile =GetConfigStr_f(subData.fileCfg,subAddr);
			fileContent += strFile;
		}	

		// 默认值
		if (subData.fileCfg.fileName.length() == 0)
		{
			char buff[10];
			memset(buff,0,10);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"sa%d",subAddr);
#ifdef WIN32
#pragma warning( pop )
#endif
			string defValue = buff;
			defValue += "={";
			string strContent;
			for (int i=0;i<32;i++)
			{
				if (strContent.length() >0)
				{
					strContent += ",";
				}
				memset(buff,0,10);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
				sprintf(buff,"0x%x",(int)subData.defDataArry[i]);
#ifdef WIN32
#pragma warning( pop )
#endif
				strContent += buff;
			}
			defValue += strContent;
			defValue += "};";

			subAddrDefval.push_back(defValue);
		}

		// 时间码
		if (TIME_CODE_error != subData.timeCode.codeType)
		{
			char buff[100];
			if (strTimeConfig.length() >0)
			{
				strTimeConfig += ",";
			}
			memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"SR%d:type%d:%s:%s:%s:%s",subAddr, (int)subData.timeCode.codeType,subData.timeCode.rout_addr.c_str(),subData.timeCode.rout_subAddr.c_str(),subData.timeCode.rout_des_addr.c_str(),subData.timeCode.rout_des_subAddr.c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
			strTimeConfig += buff;
		}

		// 三取二配置
		switch (subData.threeObj.eType)
		{
		case _three_error:
			break;
		case _three_or:
			{
				if (strThreeCfg.length() >0)
				{
					strThreeCfg += ",";
				}

				// 得到配置的与信息
				string str_orStr;
				string str_config_orName;				
				GetThreeChg_or_s(subAddr,str_orStr,str_config_orName,subData.threeObj.orStu.mapVal);

				char buff[100];
				memset(buff,0,100);
				if (str_orStr.length() >0)
				{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
					sprintf(buff,"SR%d:%s:type%d:0x%.4x:%s",subAddr,subData.threeObj.msg.getLine().c_str(),subData.threeObj.eType,subData.threeObj.orStu.def,str_config_orName.c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
					strThree_or_cfg += str_orStr;
				}
				else
				{
					string strT = "not";
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
					sprintf(buff,"SR%d:%s:type%d:0x%.4x:%s",subAddr,subData.threeObj.msg.getLine().c_str(),subData.threeObj.eType,subData.threeObj.orStu.def,strT.c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
				}
				strThreeCfg += buff;
			}
			break;
		case _three_INS:
			{
				if (strThreeCfg.length() >0)
				{
					strThreeCfg += ",";
				}
				char buff[100];
				memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
				sprintf(buff,"SR%d:%s:type%d:num%d:%s",subAddr,subData.threeObj.msg.getLine().c_str(),subData.threeObj.eType,subData.threeObj.num,"not");
#ifdef WIN32
#pragma warning( pop )
#endif
				strThreeCfg += buff;
			}
			break;
		case _three_Array:
			{
				if (strThreeCfg.length() >0)
				{
					strThreeCfg += ",";
				}
				// 得到配置的与信息
				string str_orStr;
				string str_config_orName;				
				GetThreeChg_end_s(subAddr,str_orStr,str_config_orName,subData.threeObj.retArry.listVal);

				char buff[100];
				memset(buff,0,100);
				if (str_orStr.length() >0)
				{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
					sprintf(buff,"SR%d:%s:type%d:che%d:%s",subAddr,subData.threeObj.msg.getLine().c_str(),subData.threeObj.eType,subData.threeObj.retArry.che,str_config_orName.c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
					strThree_end_cfg += str_orStr;
				}
				else
				{
					string strT = "not";
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
					sprintf(buff,"SR%d:%s:type%d:che%d:%s",subAddr,subData.threeObj.msg.getLine().c_str(),subData.threeObj.eType,subData.threeObj.retArry.che,strT.c_str());
#ifdef WIN32
#pragma warning( pop )
#endif
				}
				strThreeCfg += buff;
			}
			break;
		default:
			break;
		}
	}

	// 地址配置信息
	if (subAddrContent.length() > 0)
	{
		subAddrContent = "Sub_Address={" + subAddrContent;
		subAddrContent += "};";
	}

	// 周期刷新子地址数据
	if (strCyleContent.length() >0)
	{
		strCyleContent = "CHG_cyle_data={" + strCyleContent;
		strCyleContent += "};";
	}

	// 文件数据源
	if (fileContent.length() >0)
	{
		fileContent = "Resource_file={" + fileContent;
		fileContent += "};";
	}

	// 强制刷新的
	if (strForceFlush.length() >0 )
	{
		strForceFlush = "ForceFlush={" + strForceFlush;
		strForceFlush += "};";
	}

	// 时间码
	string strTimeCode;
	if (strTimeConfig.length() >0)
	{
		strTimeCode = "TimeCode={";
		strTimeCode += strTimeConfig;
		strTimeCode += "};";
	}

	// 三取二配置
	string strThreeChg;
	if (strThreeCfg.length() > 0)
	{
		strThreeChg = "ThreeChg={";
		strThreeChg += strThreeCfg;
		strThreeChg += "};";
	}

	// 各项的配置信息
	string strAllConfig = subAddrContent;
	// 将各项组合
	if (fileContent.length() >0)
	{
		strAllConfig += "\r\n";
		strAllConfig += fileContent;
	}
	if (strForceFlush.length() >0)
	{
		strAllConfig += "\r\n";
		strAllConfig += strForceFlush;
	}
	if(strCyleContent.length() >0)
	{
		strAllConfig += "\r\n";
		strAllConfig += strCyleContent;
	}
	if (strTimeCode.length() > 0)
	{
		strAllConfig += "\r\n";
		strAllConfig += strTimeCode;
	}
	for (vector<string>::iterator i=subAddrDefval.begin();i!= subAddrDefval.end();i++)
	{
		strAllConfig += "\r\n";
		strAllConfig += (*i);		
	}

	// 三取二有三项配置
	if (strThreeChg.length() >0)
	{
		strAllConfig += "\r\n";
		strAllConfig += strThreeChg;
	}
	if (strThree_or_cfg.length() >0)
	{
		strAllConfig += "\r\n";
		strAllConfig += strThree_or_cfg;
	}
	if (strThree_end_cfg.length() >0)
	{
		strAllConfig += "\r\n";
		strAllConfig += strThree_end_cfg;
	}
	return strAllConfig;
}


// 取得三取二配置中，需要与运算的配置
bool RTconfigSeq_v::GetThreeChg_or(string str_or,map<U16BIT,int> & mapVal)
{
	if(str_or.length() >2)
	{
		str_or.erase(0,1);
		str_or.erase(str_or.length()-1,1);

		vector<string> strVec;
		SubData(str_or,strVec,',');
		for (size_t i=0;i<strVec.size();i++)
		{
			string strLine = strVec.at(i);
			vector<string> strVecSub;
			SubData(strLine,strVecSub,'_');
			if (strVecSub.size() == 2)
			{
				U16BIT val = (U16BIT)atol_my_Hex(strVecSub.at(0).c_str());
				S16BIT index = (S16BIT)atol_my(strVecSub.at(1).c_str());
				mapVal[val] = index;
			}
		}
		return true;
	}
	else
		return false;
}

// 取得三取二配置中，是哪个消息得到一个数据的配置
bool RTconfigSeq_v::GetThreeChg_end(string str_end,map<U16BIT,ThreeResult> & mapVal)
{
	if(str_end.length() >2)
	{
		str_end.erase(0,1);
		str_end.erase(str_end.length()-1,1);

		vector<string> strVec;
		SubData(str_end,strVec,',');
		for (vector<string>::iterator i=strVec.begin();i !=strVec.end();i++)
		{
			string strLine = (*i);
			vector<string> strVecSub;
			SubData(strLine,strVecSub,'_');
			if (strVecSub.size() == 2)
			{
				U16BIT val = (U16BIT)atol_my_Hex(strVecSub.at(0).c_str());
				mapVal[val] = ThreeResult(strVecSub.at(1));
			}
		}
		return true;
	}
	else
		return false;
}
// 解析出配置的三取二项
// ThreeChg={ sa4:msg0_sa1_mode0:type0:def0:{ThreeChg_or_sa4},
//			 sa5:msg0_sa2_mode0:type1:num4,
//			 sa6:msg0_sa3_mode0:type2:che1:{ ThreeChg_or_sa5} }; 
void RTconfigSeq_v::GetThreeChg(string strThreeChg,__out map<S16BIT,_three_struct> & mapThreeChg)
{
	mapThreeChg.clear();
	if (strThreeChg.length() > 2)
	{
		strThreeChg.erase(0,1);
		strThreeChg.erase(strThreeChg.length()-1,1);

		vector<string> strVec;
		SubData(strThreeChg,strVec,',');

		// 取出配置的每个信息 
		for (size_t i=0;i<strVec.size();i++)
		{
			string strLine = strVec.at(i);
			vector<string> strSubVec;
			SubData(strLine,strSubVec,':');

			if (strSubVec.size() >= 5)
			{
				S16BIT wSa = (U16BIT)atol_my(strSubVec.at(0).c_str());
				string strMsg = strSubVec.at(1);
				S16BIT type = (U16BIT)atol_my(strSubVec.at(2).c_str());

				switch (type)
				{
				case _three_or: // 按位与
					{
						U16BIT def = (U16BIT)atol_my_Hex(strSubVec.at(3).c_str());
						string configName= strSubVec.at(4);
						string str_or ;
						m_config.get_item(configName,str_or);
						_three_struct threeStu;						
						GetThreeChg_or(str_or,threeStu.orStu.mapVal); // 添加

						threeStu.eType = _three_or;
						threeStu.msg = RT_MSG(strMsg);
						threeStu.orStu.def = def;
						mapThreeChg[wSa] = threeStu;
					}
					break;
				case _three_INS: // 一条消息内三取二，可能包含多条数据
					{
						U16BIT num = (U16BIT)atol_my(strSubVec.at(3).c_str());

						_three_struct threeStu;
						threeStu.eType = _three_INS;
						threeStu.msg = RT_MSG(strMsg);
						threeStu.num = num;
						mapThreeChg[wSa] = threeStu;  // 添加

					}
					break;
				case _three_Array: // 三条消息得到一条数据
					{
						U16BIT che = (U16BIT)atol_my(strSubVec.at(3).c_str());
						string configName = strSubVec.at(4);
						_three_struct threeStu;
						if (1 == che)
						{
							// 有其他配置项
							string str_end;
							m_config.get_item(configName,str_end);
							GetThreeChg_end(str_end,threeStu.retArry.listVal); // 添加
							threeStu.eType = _three_Array;
							threeStu.msg = RT_MSG(strMsg);
							threeStu.retArry.che= true;

							mapThreeChg[wSa] = threeStu;
						}
						else
						{
							threeStu.eType = _three_Array;
							threeStu.msg = RT_MSG(strMsg);
							threeStu.retArry.che =false;

							mapThreeChg[wSa] = threeStu;
						}
					}
					break;
				case _three_error:
					break;
				default:
					break;
				}
			}
		}
	}
}

// 得到时间码信息,时间码
// TimeCode={SR4:type2:addr:subAddr};
void RTconfigSeq_v::GetTimeCode(string strTimeCode,__out map<S16BIT,_TimeCode> & m_TimeCode)
{
	m_TimeCode.clear();
	if (strTimeCode.length() > 2)
	{
		strTimeCode.erase(0,1);
		strTimeCode.erase(strTimeCode.length()-1,1);

		vector<string> strVec;
		SubData(strTimeCode,strVec,',');

		// 取出配置的每个信息 
		for (size_t i=0;i<strVec.size();i++)
		{
			string strLine = strVec.at(i);
			vector<string> strSubVec;
			SubData(strLine,strSubVec,':');

			if (strSubVec.size() >= 2)
			{
				_TimeCode timeCodeInfo ;
				string strSa = strSubVec.at(0);
				string strType = strSubVec.at(1);
				timeCodeInfo.codeType =(enum_TimeCode) atol_my(strType.c_str());
				S16BIT subAddr = (S16BIT)atol_my(strSa.c_str());
				//	memset((void *)timeCodeInfo.dataArr,0,sizeof(U16BIT) * 32); // 清零

				if (strSubVec.size() >= 3)
				{
					timeCodeInfo.rout_addr= strSubVec.at(2);
				}
				if (strSubVec.size() >= 4)
				{
					timeCodeInfo.rout_subAddr = strSubVec.at(3);
				}
				if (strSubVec.size() >= 5)
				{
					timeCodeInfo.rout_des_addr= strSubVec.at(4);
				}
				if (strSubVec.size() >= 6)
				{
					timeCodeInfo.rout_des_subAddr = strSubVec.at(5);
				}
				m_TimeCode[subAddr] = timeCodeInfo;
			}
		}
	}
}


// CHG_msg_data = {MSG0_SA2_MODE0:[SA1],msg3-sa0_mode0:[sa4]};
void RTconfigSeq_save::GetChgMsgData_s(__in const map<RT_MSG, vector<S16BIT> > & m_chgMsg_subAddr,string & CHG_msg_data)
{
	string strContent;
	map<RT_MSG,vector<S16BIT> >::const_iterator it = m_chgMsg_subAddr.begin();
	while (it != m_chgMsg_subAddr.end())
	{
		RT_MSG rt = it->first;
		vector<S16BIT>  vec = it->second;

		string strMsg = rt.getLine();		
		string subAddr = SplitHelp::GetVecStr(vec);

		if (subAddr.length() >0)
		{
			if (strContent.length() >0)
			{
				strContent+=",";
			}
			strContent += strMsg ;
			strContent += ":[";
			strContent += subAddr;
			strContent += "]";
		}

		it++;
	}

	if (strContent.length() >0)
	{
		CHG_msg_data = "CHG_msg_data={";
		CHG_msg_data += strContent;
		CHG_msg_data +="};";
	}
	return;
}

// CHG_msg_state = {MSG3-SA7-MODE16,MSG4-SA7-MODE10};
void RTconfigSeq_save::GetChgMsgState_s(__in const set<RT_MSG> & m_chgMsg_subState,string & CHG_msg_state)
{
	string strContent;
	set<RT_MSG>::const_iterator itMsg = m_chgMsg_subState.begin();
	while(itMsg != m_chgMsg_subState.end())
	{
		if (strContent.length() > 0)
		{
			strContent +=",";
		}
		string strMsg = itMsg->getLine();

		strContent += strMsg;
		itMsg++;
	}

	if (strContent.length() > 0)
	{
		CHG_msg_state = "CHG_msg_state={";
		CHG_msg_state += strContent;
		CHG_msg_state += "};";
	}

	return;
}

// Resource_copy = {MSG5-SA0-MODE1:[SA1,SA4],MSG6-SA0-MODE16:[SA2,SA3]}; 
void RTconfigSeq_save::GetResourceCopy_s(__in const map<RT_MSG,vector<S16BIT> > & m_ResourceCopy_subAddr,string & Resource_copy)
{
	string strContent;
	map<RT_MSG,vector<S16BIT> >::const_iterator it = m_ResourceCopy_subAddr.begin();
	while (it != m_ResourceCopy_subAddr.end())
	{
		RT_MSG rt = it->first;
		vector<S16BIT>  vec = it->second;

		string strMsg = rt.getLine();		
		string strSubAddr;

		for (int i=0;i<(int)vec.size();i++)
		{
			if (strSubAddr.length() >0)
			{
				strSubAddr += ",";
			}
			char buff[10];
			memset(buff,0,10);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
			sprintf(buff,"sa%d",(int)vec.at(i));
#ifdef WIN32
#pragma warning( pop )
#endif
			strSubAddr += buff;
		}

		if (strSubAddr.length() >0)
		{
			if (strContent.length() >0)
			{
				strContent+=",";
			}

			strContent += strMsg ;
			strContent += ":[";
			strContent += strSubAddr;
			strContent += "]";
		}

		it++;
	}

	if (strContent.length() >0)
	{
		Resource_copy = "Resource_copy={";
		Resource_copy += strContent;
		Resource_copy +="};";
	}
	return;
}


// ServiceRequest = 0; 	//服务请求位,初始值
// SubSystem_Flag = 0; 	//子系统特征位,初始值
// Busy = 0; 		//忙位,初始值
// Terminal_Flag = 0; //终端特征位,初始值
void RTconfigSeq_save::GetStateWord_s(__out RT_StateWord & stateWord,string & ServiceRequest,string & SubSystem_Flag,string & Busy,string & Terminal_Flag)
{
	char buff[100];
	memset(buff,0,100);

	if (stateWord.ServiceRequest >= 0)
	{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,"ServiceRequest=%d;",stateWord.ServiceRequest);
#ifdef WIN32
#pragma warning( pop )
#endif
		ServiceRequest = buff;
		memset(buff,0,100);
	}	
	if (stateWord.SubSystem_Flag >= 0)
	{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,"SubSystem_Flag=%d;",stateWord.SubSystem_Flag);
#ifdef WIN32
#pragma warning( pop )
#endif
		SubSystem_Flag = buff;
		memset(buff,0,100);
	}

	if (stateWord.Busy_flag >= 0)
	{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,"Busy=%d;",stateWord.Busy_flag);
#ifdef WIN32
#pragma warning( pop )
#endif
		Busy = buff;
		memset(buff,0,100);
	}
	if (stateWord.Terminal_Flag >= 0)
	{
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,"Terminal_Flag=%d;",stateWord.Terminal_Flag);
#ifdef WIN32
#pragma warning( pop )
#endif
		Terminal_Flag = buff;
		memset(buff,0,100);
	}
	return;
}

// 输出
// CHG_cyle_state = ms2000;
void RTconfigSeq_save::getCyleState_s(long cyle,string & CHG_cyle_state)
{
	if (cyle >= 0)
	{
		char buff[100];
		memset(buff,0,100);
#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		sprintf(buff,"ms%d",(int)cyle);
#ifdef WIN32
#pragma warning( pop )
#endif
		CHG_cyle_state = "CHG_cyle_state=";
		CHG_cyle_state += buff;
		CHG_cyle_state += ";";
	}
	return;
}

