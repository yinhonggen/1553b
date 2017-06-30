#pragma once
#include "RT_DataSource.h"
#include "RT_StateWord.h"
#include "utility.h"


///
//  配置文件解析
//
class SplitBase
{
public:
	SplitBase(void);
	~SplitBase(void);

	void clear()
	{	
	
	};
public:

	// 配置的子地址
	void Split_SubAddrInfo(__in string strSubAddr,__in string ConfigFileStr,__out map<S16BIT,_addrData> & m_SubAddr);

	void GetSubAddrCyle(__in string str,__out map<S16BIT,_CycleIndex> & m_SubAddrCyle);

	//
	void GetChgMsgData(__in string str,__out map<RT_MSG, vector<S16BIT> > & m_chgMsg_subAddr);

	void GetChgMsgState(__in string str,__out set<RT_MSG> & m_chgMsg_subState);

	void GetResourceCopy(__in string str,__out map<RT_MSG,vector<S16BIT> > & m_ResourceCopy_subAddr);

private:

	// 读文件配置信息
	void GetResourceFile(__in string str,__out map<S16BIT,_fileConfig> & ResourceFile_sub);

	// 解析出小包信息
	bool SplitSimlBag(string str,SimlBag & fileBag);

public:

};

// RT配置文件解析和保存成字符串
class RTconfigSeq
{
public:
	RTconfigSeq();

public:

	// 通过文件名称装载配置字符串信息
	// fileName: 文件路径名称的全称
	// 返回是否载入成功
	bool LoadFile(const string & fileName);

	// 载入配置字符串内容
	// cfg: 配置字符串
	// 返回是否载入成功
	bool LoadConfigStr(const std::string & cfg);

	// 清理类中的各个变量
	void clear();


// 解析
public:
	// 得到配置中的RT地址
	// iAddress: 返回的RT地址
	bool GetRTAddress(S16BIT & iAddress);

	// 配置的子地址数据源配置信息
	// subAddrInfo: 配置中子地址数据源信息
	// forceFlush: 子地址强制刷新的信息
	void GetSubAddrInfo(__out map<S16BIT,_addrData> & subAddrInfo,__out set<S16BIT> & forceFlush);

	// 子地址中需要周期刷新的信息
	void GetSubAddrCyle(__out map<S16BIT,_CycleIndex> & m_SubAddrCyle);

	// 得到状态子得周期
	// 返回负值说明没有配置
	long getCyleState();

	// 得到消息变化的子地址
	void GetChgMsgData(__out map<RT_MSG, vector<S16BIT> > & m_chgMsg_subAddr);

	// 得到状态字信息变化的子地址
	void GetChgMsgState(__out set<RT_MSG> & m_chgMsg_subState);

	// 得到消息拷贝数据源
	void GetResourceCopy(__out map<RT_MSG,vector<S16BIT> > & m_ResourceCopy_subAddr);

	// 得到状态字
	void GetStateWord(__out RT_StateWord & stateWord);

private:

	// 得到时间码信息
	void GetTimeCode(string strTimeCode,__out map<S16BIT,_TimeCode> & m_TimeCode);

	// 得到配置的三取二
	void GetThreeChg(string strThreeChg,__out map<S16BIT,_three_struct> & mapThreeChg);

	// 取得三取二配置中，是哪个消息得到一个数据的配置
	bool GetThreeChg_end(string str_end,map<U16BIT,ThreeResult> & mapVal);

	// 取得三取二配置中，需要与运算的配置
	bool GetThreeChg_or(string str_or,map<U16BIT,int> & mapVal);

// 拼接
public:

	// 输出:
	// Address=9;
	bool GetRTAddress(S16BIT iAddress,string & outStr);

	// 输出:
	// string & Sub_Address: Sub_Address = {sa1:[num10]:[che1]:[cheIndex11],SA2,SA3,SA4,SA5};
	// string & Resource_file:  Resource_file = {SA1:fileName:def0000:sim67_30_30_7,SA2:fileName};
	// string & ForceFlush:  ForceFlush={sa1,sa2 };
	// vector<string> & subAddrDefval:  默认值的配置字符串
	void GetSubAddrInfo(__in const map<S16BIT,_addrData> & m_SubAddr,__in const set<S16BIT> & m_ForceFlush,string & Sub_Address,string & Resource_file,string & ForceFlush,vector<string> & subAddrDefval);

	// 输出
	// CHG_cyle_data = {sa1_ms2000_num1, SA1_ms200_num2}
	void GetSubAddrCyle(__in const map<S16BIT,_CycleIndex> & m_SubAddrCyle,string & CHG_cyle_data);

	// 输出
	// CHG_cyle_state = ms2000;
	void getCyleState(long cyle,string & CHG_cyle_state);

	// CHG_msg_data = {SA2_MSG0_MODE0:[SA1],sa0_msg3-mode0:[sa4]};
	void GetChgMsgData(__in const map<RT_MSG, vector<S16BIT> > & m_chgMsg_subAddr,string & CHG_msg_data);

	// CHG_msg_state = {SA7-MSG3-MODE16,SA7-MSG4-MODE10};
	void GetChgMsgState(__in const set<RT_MSG> & m_chgMsg_subState,string & CHG_msg_state);

	// Resource_copy = {SA0-MSG5-MODE1:[SA1,SA4],SA0-MSG6-MODE16:[SA2,SA3]}; 
	void GetResourceCopy(__in const map<RT_MSG,vector<S16BIT> > & m_ResourceCopy_subAddr,string & Resource_copy);

	// ServiceRequest = 0; 	//服务请求位,初始值
	// SubSystem_Flag = 0; 	//子系统特征位,初始值
	// Busy = 0; 		//忙位,初始值
	// Terminal_Flag = 0; //终端特征位,初始值
	void GetStateWord(__in RT_StateWord & stateWord,string & ServiceRequest,string & SubSystem_Flag,string & Busy,string & Terminal_Flag);

private:
	// SA1:fileName:def0000:sim67_30_30_7
	string GetConfigStr_f(const _fileConfig & config,S16BIT subAddr);
private:

	// 得到子地址的默认值
	void SetSubAddrDefValue(_addrData & addrData,const string & strValue);


	// 配置信息
	void GetThreeChg_end_s(S16BIT subAddr,string & str_orStr,string & str_config_orName,map<U16BIT,ThreeResult> & mapVal);
	
	// 配置信息
	void GetThreeChg_or_s(S16BIT subAddr,string & str_orStr,string & str_config_orName,map<U16BIT,int> & mapVal);

private:
	
	// 解析对象
	SplitBase config_split;

	// 配置字符串
	string m_StrConfig; 

	RT::BrConfig m_config;
private:

	// 子地址
	string strSubAddressArr;

	// 状态周期
	string strCycle ;

	// 变化周期
	string chgCyle_dArr ;

	// 消息变化
	string chgMsg_dArr;
	string chgMsg_sArr;

	//文件数据源
	string Resource_file;
	string Resource_copy;

	// 时间码
	string strTimeCode;
	
	// 状态字
	string strTemp_sr;
	string strTemp_sf;
	string strTemp_b;
	string strTemp_tf;

	// 需要取走后刷新的
	string strForceFlush;

	// 保存
public:

	// 保存
	//void SaveConfig(const string & fileFullName,const map<S16BIT,_SubAddrConfig> & config);
};

