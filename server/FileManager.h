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
		// �ļ����
		long hFile = 0;
		// �ļ���Ϣ
		struct _finddata_t fileinfo;

		std::string p;

		if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
		{
			do
			{
				// �����ļ���ȫ·��
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));

			}
			while (_findnext(hFile, &fileinfo) == 0);   //Ѱ����һ�����ɹ�����0������-1

			_findclose(hFile);
		}
	}


	//��ȡָ��Ŀ¼�µ������ļ����������ļ��У�
	void getAllFiles1(std::string path, std::vector<std::string>& files)
	{
		//�ļ����
		intptr_t  hFile = 0;
		//�ļ���Ϣ
		struct _finddata_t fileinfo;
		std::string p;
		if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
		{
			do
			{
				if ((fileinfo.attrib & _A_SUBDIR))   //�Ƚ��ļ������Ƿ����ļ���
				{
					if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					{
						files.push_back(p.assign(path).append("\\").append(fileinfo.name));
						//�ݹ�����
						getAllFiles1(p.assign(path).append("\\").append(fileinfo.name), files);
					}
				}
				else
				{
					files.push_back(p.assign(path).append("\\").append(fileinfo.name));
				}
			}
			while (_findnext(hFile, &fileinfo) == 0);   //Ѱ����һ�����ɹ�����0������-1
			_findclose(hFile);
		}
	}
	/*
	path: ָ��Ŀ¼
	files: ������
	fileType: ָ�����ļ���ʽ���� .jpg
	*/
	void getAllFiles(std::string path, std::vector<std::string>& files, std::string fileType)
	{
		// �ļ����
		long long hFile = 0;
		// �ļ���Ϣ
		struct _finddata_t fileinfo;

		std::string p;

		if ((hFile = _findfirst(p.assign(path).append("\\*" + fileType).c_str(), &fileinfo)) != -1)
		//if ((hFile = _findfirst(p.assign(path).append("\\" + fileType + "*.log" ).c_str(), &fileinfo)) != -1)
		{
			do
			{
				// �����ļ���ȫ·��
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));

			}
			while (_findnext(hFile, &fileinfo) == 0);   //Ѱ����һ�����ɹ�����0������-1

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