#pragma once

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <io.h>
#include <string>
#include <vector>
#include <fstream>


class FileManager
{
public:
	FileManager();
	~FileManager();

	void getAllFiles(std::string path, std::vector<std::string>& files)
	{
		// 文件句柄
		long hFile = 0;
		// 文件信息
		struct _finddata_t fileinfo;

		std::string p;

		if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
		{
			do
			{
				// 保存文件的全路径
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));

			}
			while (_findnext(hFile, &fileinfo) == 0);   //寻找下一个，成功返回0，否则-1

			_findclose(hFile);
		}
	}


	//获取指定目录下的所有文件（搜索子文件夹）
	void getAllFiles1(std::string path, std::vector<std::string>& files)
	{
		//文件句柄
		intptr_t  hFile = 0;
		//文件信息
		struct _finddata_t fileinfo;
		std::string p;
		if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
		{
			do
			{
				if ((fileinfo.attrib & _A_SUBDIR))   //比较文件类型是否是文件夹
				{
					if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					{
						files.push_back(p.assign(path).append("\\").append(fileinfo.name));
						//递归搜索
						getAllFiles1(p.assign(path).append("\\").append(fileinfo.name), files);
					}
				}
				else
				{
					files.push_back(p.assign(path).append("\\").append(fileinfo.name));
				}
			}
			while (_findnext(hFile, &fileinfo) == 0);   //寻找下一个，成功返回0，否则-1
			_findclose(hFile);
		}
	}
	/*
	path: 指定目录
	files: 保存结果
	fileType: 指定的文件格式，如 .jpg
	*/
	void getAllFiles(std::string path, std::vector<std::string>& files, std::string fileType)
	{
		// 文件句柄
		long long hFile = 0;
		// 文件信息
		struct _finddata_t fileinfo;

		std::string p;

		if ((hFile = _findfirst(p.assign(path).append("\\*" + fileType).c_str(), &fileinfo)) != -1)
		//if ((hFile = _findfirst(p.assign(path).append("\\" + fileType + "*.log" ).c_str(), &fileinfo)) != -1)
		{
			do
			{
				// 保存文件的全路径
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));

			}
			while (_findnext(hFile, &fileinfo) == 0);   //寻找下一个，成功返回0，否则-1

			_findclose(hFile);
		}
	}

	void GetDirFiles(const char* pszDir, char* pszFileType, std::vector<std::string>& vtFileList, BOOL bRecursion)
	{
		if (pszDir == NULL || pszFileType == NULL)
		{
			return;
		}

		char   szTem[1024] = { 0 };
		char   szDir[1024] = { 0 };
		strcpy(szTem, pszDir);
		if (szTem[strlen(szTem) - 1] != '\\' || szTem[strlen(szTem) - 1] != '/')
		{
			strcat(szTem, "/");
		}

		strcpy(szDir, szTem);
		strcat(szDir, pszFileType);


		struct _finddata_t  tFileInfo = { 0 };
		long long hFile = _findfirst(szDir, &tFileInfo);
		if (hFile == -1)
		{
			return;
		}

		do
		{
			if (strcmp(tFileInfo.name, ".") == 0 || strcmp(tFileInfo.name, "..") == 0)
			{
				continue;
			}

			if ((tFileInfo.attrib   &  _A_SUBDIR) && bRecursion)
			{
				char   szSub[1024] = { 0 };
				strcpy(szSub, pszDir);
				if (szSub[strlen(szSub) - 1] != '\\' || szSub[strlen(szSub) - 1] != '/')
				{
					strcat(szSub, "/");
				}
				strcat(szSub, tFileInfo.name);
				GetDirFiles(szSub, pszFileType, vtFileList, bRecursion);
			}
			else
			{
				vtFileList.push_back(std::string(szTem) + std::string(tFileInfo.name));
			}
		} while (_findnext(hFile, &tFileInfo) == 0);
		_findclose(hFile);

	}

private:

};

FileManager::FileManager()
{
}

FileManager::~FileManager()
{
}

#endif