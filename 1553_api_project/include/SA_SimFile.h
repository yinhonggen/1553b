#pragma once

#include <time.h>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <queue>
#include <set>
using namespace std;

// 
// 数据文件类
// 提供数据文件的读取缓存操作
class SimFile
{
public:
	SimFile();
	~SimFile();

private:
	int fileOffset; 

	// 文件缓存长度
	int m_fileLen;

	// 文件缓存
	char * m_pFileBuff;

	//仿真文件
	FILE * m_pFile;

	void clear();

public:

	// 文件是否打开成功
	bool IsOpen(){
		return m_pFileBuff!= NULL?true:false;
	};
	// 打开文件
	// 返回是否打开成功
	bool open(const string & fileNameStr);

	// 读文件缓存
	// pBuff: 目的地址
	// readLen: 读入的长度
	int fread_buff(char * pBuff,int readLen);

	// 缓存重新定位
	void fseek_buff(int offset);

	// 读入文件缓存,将这个文件一次读入内存
	void LoadFileData();
};




