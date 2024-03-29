// server.cpp: 定义控制台应用程序的入口点。
//


#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <WinSock2.h>
#include <cassert>
#include <iostream>
#include<thread>
#include <vector>
#include "..\common\netpack.h"
#include <string>
#include "..\common\ThreadPool.h"
#include <functional>
#include "..\common\FileManager.h"
#include "..\common\MyMd5.h"

#define PORT 8087
#define SERVER_IP "127.0.0.1"
//#define BUFFER_SIZE 1024
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512
#pragma comment(lib, "WS2_32")

typedef std::string xstring;

typedef std::function<void()>ThreadTask;
fivestar::ThreadPool m_ThreadPool;

SOCKET StartUp()
{
	//声明地址结构
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	//初始化 socket dll
	WSADATA wsaData;
	WORD socketVersion = MAKEWORD(2, 2);

	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return SOCKET_ERROR;
	}

	//创建socket
	SOCKET m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == m_socket)
	{
		return SOCKET_ERROR;
	}

	//绑定
	if (SOCKET_ERROR == bind(m_socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		return SOCKET_ERROR;
	}

	//监听
	if (SOCKET_ERROR == listen(m_socket, 10))
	{
		return SOCKET_ERROR;
	}

	return m_socket;
}

std::vector<SOCKET>ClientScoketVec;

void SplitString(const xstring& s, std::vector<xstring>& v, const xstring& c);
long long getFileSize(char* path);
void DoSendData(SOCKET m_socket, std::vector<std::string>&filepath)
{
	//printf("now pid is %d", GetCurrentProcessId());
	//printf("now tid is %d \n", GetCurrentThreadId());
	for (auto it : filepath)
	{
		long long filesize = getFileSize((char*)it.c_str());
		std::cout << "file size :" << filesize<<" K " << std::endl;
		long long filesize2 = filesize;

		char buffer[BUFFER_SIZE];
		FILE *fp = fopen(it.c_str(), "rb");  //以只读，二进制的方式打开一个文件
		Sleep(700);
		if (NULL == fp)
		{
			std::cout << "open error " << std::endl;
			system("pause");
			return;
		}
		memset(buffer, 0, BUFFER_SIZE);
		int length = 0;
		int index = 0;
		
		std::vector<xstring> v;
		SplitString(it, v, "/");
		if (v.size() != 2)
		{
			std::cout << "Error PathName : " << it << std::endl;
			system("pause");
			return;
		}

		std::cout << "start send : " << it << std::endl;

		while ((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
		{
			filesize -= length;

			NetPacket sendPack;
			sendPack.Header.wOpcode = SENDDATA;
			sendPack.Header.wDataSize = length + sizeof(NetPacketHeader);
			sendPack.Header.wOrderindex = index;
			sendPack.Header.filesize = filesize2;
			memcpy(sendPack.Header.filename, v[1].c_str(), MAXNAME);

			memcpy(sendPack.Data, buffer, length);
			if (filesize == 0)
			{
				//std::cout << "send last char  " << std::endl;

				sendPack.Header.tail = 1;
			}

			int sendlen = send(m_socket, (const char*)&sendPack, sizeof(sendPack), 0);

			if (sendlen == -1)
			{
				fclose(fp);
				closesocket(m_socket);
				std::cout << "socket close " << std::endl;
				return;
			}

			while (sendlen < length)
			{
				sendlen = send(m_socket, ((const char*)&sendPack) + sendlen, sizeof(sendPack) - sendlen, 0);
				length -= sendlen;
				std::cout << "buff -- " << std::endl;
			}

			memset(buffer, 0, BUFFER_SIZE);
			index++;
		}
		std::cout <<"MD5 : "<< MD5_file((char*)it.c_str(), 32) << std::endl;
		fclose(fp);
		std::cout << it << " Transfer Successful !" << std::endl;
	}

	closesocket(m_socket);
}

std::vector<std::thread *>m_thradVec;

void Doaccept(SOCKET m_socket,std::vector<std::string>&filepath)
{
	while (true)
	{
		std::cout << "Listening To Client ..." << std::endl;
		sockaddr_in client_addr;
		int client_addr_len = sizeof(client_addr);
		SOCKET m_new_socket = accept(m_socket, (sockaddr *)&client_addr, &client_addr_len);
		if (SOCKET_ERROR == m_new_socket)
		{
			std::cout << "SOCKET_ERROR ..." << std::endl;
			continue;
		}
		ClientScoketVec.push_back(m_new_socket);
		std::cout << "New Client Connection" << std::endl;
		std::thread T(std::bind(DoSendData, m_new_socket, filepath));
		T.detach();
		//std::thread *Tsend = new std::thread(DoSendData, m_new_socket);
		//m_thradVec.push_back(Tsend);
	}
}

std::string GetCurrentExeDir()
{
	char szPath[1024] = { 0 }, szLink[1024] = { 0 };
#ifdef WIN32
	ZeroMemory(szPath, 1024);
	GetModuleFileNameA(NULL, szPath, 1024);
	char* p = strrchr(szPath, '\\');
	*p = 0;
#else
	snprintf(szLink, 1024, "/proc/%d/exe", getpid());/////////////
	readlink(szLink, szPath, sizeof(szPath));//////////////
#endif
	return std::string(szPath);
}


void SplitString(const xstring& s, std::vector<xstring>& v, const xstring& c)
{
	xstring::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (xstring::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

int main()
{
	FileManager file;
	std::vector<std::string> temp;
	std::string workSpacePath = GetCurrentExeDir();
	std::string sqlPath = workSpacePath + "\\src";
	std::vector<std::string>m_vtFileList;
	file.GetDirFiles(sqlPath.c_str(), (char*)"*", m_vtFileList, false);
	if (m_vtFileList.empty())
	{
		std::cout << "src No files : " << std::endl;
		system("pause");
		return 0;
	}

	for (auto it : m_vtFileList)
	{
		std::vector<xstring> v;
		SplitString(it, v, "/");
		if (v.size() != 2)
		{
			std::cout << "Error PathName : " << it << std::endl;
			system("pause");
			return 0;
		}
		std::cout << v[1] << std::endl;
	}
	
	SOCKET m_socket = StartUp();
	if (m_socket == SOCKET_ERROR)
	{
		std::cout << "SOCKET_ERROR " << std::endl;
		system("pause");
		return -1;
	}


	std::thread taccept(std::bind(Doaccept, m_socket, m_vtFileList));
	taccept.join();
	std::cout << "ThreadPool over" << std::endl;
	closesocket(m_socket);

	//释放 winsock 库
	WSACleanup();
	getchar();
	return 0;
}

long long getFileSize(char* path)
{
	FILE * pFile;
	long long size;

	pFile = fopen(path, "rb");
	if (pFile == NULL)
		perror("Error opening file");
	else
	{
		fseek(pFile, 0, SEEK_END);   ///将文件指针移动文件结尾
		size = _ftelli64(pFile); ///求出当前文件指针距离文件开始的字节数
		fclose(pFile);
		//printf("Size of file.cpp: %lld bytes.\n", size);
		return size;
	}

	return 0;
}