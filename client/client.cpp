// client.cpp: 定义控制台应用程序的入口点。
//



#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <WinSock2.h>
#include "../common/netpack.h"
#include <thread>
#include <vector>
#include <map>
#include "../common/MyMd5.h"
#include <mutex>

#define PORT 8087
std::string SERVER_IP = "127.0.0.1";
//#define SERVER_IP "192.168.0.96"
#define BUFFER_SIZE 1576 * 10

#define CACHE_SIZE 1576 * 100

#define FILE_NAME_MAX_SIZE 512
#pragma comment(lib, "WS2_32")

long long getFileSize(char* path);

SOCKET StartUp()
{
	//初始化 socket dll
	WSADATA wsaData;
	WORD socketVersion = MAKEWORD(2, 2);
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return SOCKET_ERROR;
	}

	//创建socket
	SOCKET c_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == c_socket)
	{
		return SOCKET_ERROR;
	}

	//指定服务端的地址
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(SERVER_IP.c_str());
	server_addr.sin_port = htons(PORT);
	if (SOCKET_ERROR == connect(c_socket, (LPSOCKADDR)&server_addr, sizeof(server_addr)))
	{
		return SOCKET_ERROR;
	}
	return c_socket;
}

char                m_cbDataBuf[10240];


struct FileData
{
	int size = 0;
	bool isLast = false;
	std::string data = "";
};

struct WriteFileData
{
	std::string filename = "";
	FILE *fp = nullptr;
	long long index = -1;
	long long filesize = 0;
	std::map<long long, FileData> m_cache;
};


std::mutex mtx;
long long curlength = 0;

void printProgress(long long alllength, std::string filename)
{
	std::cout << "\n\n开始接收文件 : " << filename.c_str() << " 文件大小 : " << alllength / 1024 / 1024 << " M" << std::endl;
	std::cout << "正在获取数据...... ";
	while (curlength < alllength)
	{
		std::cout.width(3);//i的输出为3位宽
		std::cout << (int)((float)curlength / (float)alllength * 100) << "%";
		Sleep(50);
		std::cout << "\b\b\b\b";//回删三个字符，使数字在原地变化
	}

	std::cout.width(3);
	std::cout << (int)((float)curlength / (float)alllength * 100) << "%";
	std::cout << std::endl;
	mtx.lock();
	curlength = 0;
	mtx.unlock();
}


int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		SERVER_IP = argv[1];
	}

	//system("mkdir des");

	SOCKET c_socket = SOCKET_ERROR;
	while (c_socket == SOCKET_ERROR)
	{
		c_socket = StartUp();
		std::cout << "Waiting for a server connection ......" << std::endl;
		Sleep(100);
	}

	std::cout << "server connection !!! " << std::endl;

	std::vector<WriteFileData>fileNameVec;

	long long length = 0;
	long long m_nRecvSize = 0;
	long long offindex = 0;

	char buffer[BUFFER_SIZE];
	char m_cbRecvBuf[CACHE_SIZE];
	memset(buffer, 0, BUFFER_SIZE);



	LARGE_INTEGER t1, t2, tc;

	while ((length = recv(c_socket, buffer, BUFFER_SIZE, 0)) > 0)
	{
		if ((m_nRecvSize + length) > CACHE_SIZE)
		{
			if ((m_nRecvSize - offindex) > 0)
			{
				/*
				char temp[CACHE_SIZE] = { 0 };
				memcpy(temp, m_cbRecvBuf + offindex, m_nRecvSize - offindex);
				*/
				memmove(m_cbRecvBuf, m_cbRecvBuf + offindex, m_nRecvSize - offindex);
				/*
				memcpy(m_cbRecvBuf, temp, m_nRecvSize - offindex);
				*/


				m_nRecvSize = m_nRecvSize - offindex;
				offindex = 0;
			}
			else
			{
				memset(m_cbRecvBuf, 0, CACHE_SIZE);
				offindex = 0;
				m_nRecvSize = 0;
			}
		}

		memcpy(m_cbRecvBuf + m_nRecvSize, buffer, length);
		m_nRecvSize += length;

		while ((m_nRecvSize - offindex) >= sizeof(NetPacketHeader))
		{
			NetPacketHeader* pHeader = (NetPacketHeader*)(m_cbRecvBuf + offindex);
			if (pHeader == nullptr)
			{
				continue;
			}

			if (pHeader->wCode != NET_CODE)
			{
				break;
			}
			if (pHeader->wOpcode == SENDDATA)
			{
				if (pHeader->wDataSize > (m_nRecvSize - offindex))
				{
					break;
				}

				std::string filename = "des\\";
				filename.append(pHeader->filename);
				bool isNew = true;
				WriteFileData *pfileData = NULL;

				for (auto it = fileNameVec.begin(); it != fileNameVec.end(); it++)
				{
					if ((*it).filename == filename)
					{
						isNew = false;
						pfileData = &(*it);
						break;
					}
				}

				if (isNew)
				{
					std::thread Tprint(std::bind(printProgress, pHeader->filesize, filename));
					Tprint.detach();
					QueryPerformanceFrequency(&tc);
					QueryPerformanceCounter(&t1);

					WriteFileData filedata;
					filedata.fp = fopen(filename.c_str(), "wb");
					if (filedata.fp == nullptr)
					{
						std::cout << "open file Error " << filename.c_str() << std::endl;
						system("pause");
						return -1;
					}

					filedata.filename = filename;
					fileNameVec.push_back(filedata);

					for (auto it = fileNameVec.begin(); it != fileNameVec.end(); it++)
					{
						if ((*it).filename == filename)
						{
							pfileData = &(*it);
							break;
						}
					}
				}

				NetPacket* msg = (NetPacket*)(m_cbRecvBuf + offindex);

				if (pHeader->wOrderindex != pfileData->index + 1)
				{
					FileData data;
					data.size = pHeader->wDataSize - sizeof(NetPacketHeader);
					data.data = msg->Data;
					data.isLast = pHeader->tail;

	
					mtx.lock();
					curlength += data.size;
					mtx.unlock();

					pfileData->m_cache[pHeader->wOrderindex] = data;

					offindex += sizeof(NetPacket);
				}
				else
				{
					pfileData->index = pHeader->wOrderindex;

					int templength = pHeader->wDataSize - sizeof(NetPacketHeader);

					mtx.lock();
					curlength += templength;
					mtx.unlock();

					if (fwrite(msg->Data, sizeof(char), templength, pfileData->fp) < templength)
					{
						std::cout << "write error " << std::endl;
						system("pause");
						return -1;
					}

					offindex += sizeof(NetPacket);
					memset(buffer, 0, BUFFER_SIZE);

					int tempindex = pHeader->wOrderindex + 1;
					while (pfileData->m_cache.find(tempindex) != pfileData->m_cache.end())
					{
						pfileData->index = tempindex;
						if (fwrite(pfileData->m_cache[tempindex].data.c_str(), sizeof(char), pfileData->m_cache[tempindex].size, pfileData->fp) < pfileData->m_cache[tempindex].size)
						{
							std::cout << "write error " << std::endl;
							system("pause");
							return -1;
						}

						if (pfileData->m_cache[tempindex].isLast)
						{
							long long mb = getFileSize((char*)pfileData->filename.c_str()) / 1048576;

							QueryPerformanceCounter(&t2);

							mtx.lock();
							float alltime = (t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart;
							printf("耗时 :%f 秒 \n", alltime);

							std::cout << "速度 : " << mb / alltime << " M/s" << std::endl;

							std::cout << "Receive File : " << pfileData->filename.c_str() << " From Server Successful !" << std::endl;
							fclose(pfileData->fp);
							std::cout << MD5_file((char*)pfileData->filename.c_str(), 32) << std::endl;
							mtx.unlock();
							break;
						}
						tempindex++;
					}

					if (pHeader->tail)
					{
						long long mb = getFileSize((char*)pfileData->filename.c_str()) / 1048576;

						QueryPerformanceCounter(&t2);

						mtx.lock();
						float alltime = (t2.QuadPart - t1.QuadPart) * 1.0 / tc.QuadPart;
						printf("耗时 :%f 秒 \n", alltime);

						std::cout << "速度 : " << mb / alltime << " M/s" << std::endl;

						std::cout << "Receive File : " << pfileData->filename.c_str() << " From Server Successful !" << std::endl;
						fclose(pfileData->fp);
						std::cout << " MD5 : " << MD5_file((char*)pfileData->filename.c_str(), 32) << std::endl;
						mtx.unlock();
						break;
					}
				}
			}
		}
	}

	closesocket(c_socket);
	//释放winsock 库	
	WSACleanup();

	system("pause");
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