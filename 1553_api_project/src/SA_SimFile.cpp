  
#include "SA_SimFile.h"
#include "utility.h"

//全局变量
bool RT_Adapter_changeEndian;

SimFile::SimFile()
{
	m_pFileBuff = NULL;
	m_fileLen = 0;
	fileOffset = 0;
	m_pFile = NULL;
}

SimFile::~SimFile()
{
	if (m_pFileBuff != NULL)
	{
		delete m_pFileBuff;
		m_pFileBuff = NULL;
	}
	if (m_pFile != NULL)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

bool SimFile::open(const string & fileNameStr)
{
	if (m_pFile != NULL)
	{
		fclose(m_pFile);
		m_pFile= NULL;
	}

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	m_pFile = fopen(fileNameStr.c_str(),"r");
#ifdef WIN32
#pragma warning( pop )
#endif


	return  NULL == m_pFile?false:true;
}

void SimFile::LoadFileData()
{
	if(m_pFile == NULL) 
		return ;
	// 临时缓存
	char buff[1024];

	// 将文件读入缓存，以后不用每次都访问I/O
	int readLen = fread(buff,1,1024,m_pFile);
	while (readLen >0)
	{
		m_fileLen += readLen;
		readLen = fread(buff,1,1024,m_pFile);
	}
	m_fileLen += readLen;
	fseek(m_pFile,0,SEEK_SET);

	// 读入
	if (m_pFileBuff != NULL)
	{
		delete m_pFileBuff;
	}
	m_pFileBuff=(char *) malloc(m_fileLen);

	if(m_pFileBuff == NULL)
	{
		printf("\t[commu_RT]: RT仿真文件，内存申请出错!!\n");
	}
	else
	{
		// 一次读入缓存
		readLen = fread(m_pFileBuff,1,m_fileLen,m_pFile);
		if (readLen != m_fileLen)
		{
			printf("\t[commu_RT]:读RT仿真文件异常\n");
		}
		m_fileLen = readLen;
		if(RT_Adapter_changeEndian){
			RT::swap_by_word(m_pFileBuff, m_fileLen);
		}
	}
}

// 读文件缓存
int SimFile::fread_buff(char * pBuff,int readLen)
{
	if (m_pFileBuff != NULL && pBuff != NULL)
	{
		if (m_fileLen > (fileOffset +readLen) )
		{
			memcpy(pBuff,m_pFileBuff + fileOffset,readLen);
			fileOffset += readLen;

			return readLen;
		}
		else  //移除后重头读取
		{
			int subLen = m_fileLen - fileOffset;
			memcpy(pBuff,m_pFileBuff + fileOffset,subLen);
			memset(pBuff+subLen,0,readLen-subLen); // 不够时赋值0
			fileOffset = 0;
			readLen = subLen;
		}
		return readLen;
	}
	else
		return -1;
}

// 缓存重新定位
void SimFile::fseek_buff(int offset)
{
	if (m_pFileBuff != NULL)
	{
		fileOffset = 0;
	}
}
