#include "stdafx.h"
#include "socket.h"

#include "tool.h"
#include "../../../include/Utility/SmartPtr.hpp"
#include "../../../include/I8Type.hpp"
#include "../../../include/Utility/utility.h"
#include "../../../include/AsyncIO/Network/IPAddress.hpp"
#include "Serialize.h"

#include <limits> 

#ifdef max
#undef max
#endif

#ifdef _DEBUG 
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__) 
#endif

#define CONFIG_FILE				_T("Data\\I8SyncSvr.ini")


namespace i8desk
{


	CSocket::CSocket(i8desk::ISvrPlug* pPlug, i8desk::ILogger* pLogger, HANDLE hExited) 
		: m_pPlug(pPlug)
		, m_pLogger(pLogger)
		, m_Socket(INVALID_SOCKET)
		, m_hExited(hExited)
	{
		WSAData wsaData;
		WSAStartup(0x0202, &wsaData);
		TCHAR path[MAX_PATH] = {0};
		GetModuleFileName(NULL, path,MAX_PATH);
		PathRemoveFileSpec(path);
		PathAddBackslash(path);
		lstrcat(path, CONFIG_FILE);

		TCHAR buf[MAX_PATH] = {0};
		GetPrivateProfileString(_T("SYSTEM"), _T("SvrIp"), _T(""),  buf, MAX_PATH, path);
		m_SvrIp = buf;
	}

	CSocket::~CSocket()
	{
		WSACleanup();
	}


	bool CSocket::CreateSocket()
	{
		DWORD dwError = 0;
		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_Socket == INVALID_SOCKET)
		{
			dwError = WSAGetLastError();
			_com_error error(dwError);
			m_pLogger->WriteLog(LM_INFO, TEXT("Create Socket Fail:%d:%s"), dwError, error.ErrorMessage());
			return false;
		}

		sockaddr_in server = {0};
		server.sin_family = AF_INET;
		server.sin_port = htons(SERVER_PORT);
		server.sin_addr.s_addr = inet_addr(CT2A(m_SvrIp.c_str()));
		if (SOCKET_ERROR == connect(m_Socket, (PSOCKADDR)&server, sizeof(server)))
		{
			dwError = WSAGetLastError();
			_com_error error(dwError);
			m_pLogger->WriteLog(LM_INFO, TEXT("Connect %s Fail:%d:%s"), m_SvrIp.c_str(),dwError, error.ErrorMessage());
			CloseSocket();
			return false;
		}
		//m_pLogger->WriteLog(LM_INFO, TEXT("Create Socket Success"));
		return true;
	}

	void CSocket::CloseSocket()
	{
		if (m_Socket != INVALID_SOCKET)
		{
			::shutdown(m_Socket, SD_BOTH);
			::closesocket(m_Socket);
			m_Socket = INVALID_SOCKET;
		}
		//m_pLogger->WriteLog(LM_INFO, TEXT("Close Socket"));
	}


	void CSocket::FreeVDisk(std::map<DWORD, tagVDInfoEx*>& VDList)
	{
		for (std::map<DWORD, tagVDInfoEx*>::iterator it = VDList.begin(); it != VDList.end(); it++)
		{
			tagVDInfoEx* pVDisk = it->second;
			if (pVDisk->handle != NULL)
			{
				m_pPlug->SendMessage(VDISK_CMD_STOP, (DWORD)pVDisk->handle, 0);				
				m_pLogger->WriteLog(LM_INFO, TEXT("Stop VDisk:%c"), pVDisk->szSvrDrv);
			}
			delete pVDisk;
		}
		VDList.clear();
	}

	bool CSocket::GetVDiskList(std::map<DWORD, tagVDInfoEx*>& VDList)
	{
		using namespace i8desk;
		VDList.clear();

		char buf[4096] = {0};
		VDiskPkHelper out(buf, 4096, CMD_GET_VDISK_CONFIG);

		char* pData = NULL;
		int length = 0;
		if (!_SendCmd(buf, out.GetLength(), pData, length))
		{
			m_pLogger->WriteLog(LM_INFO, TEXT("Exec Get VDisk List cmd Error"));
			return false;
		}

		try
		{
			utility::CArrayCharPtr data(pData);
			CPkgHelper in(data);
			DWORD dwStatus = 0;
			in.Pop(dwStatus);
			if (dwStatus == CMD_RET_FAIL)
			{
				char Error[8096] = {0};
				in.PopString(Error);				
				m_pLogger->WriteLog(LM_INFO, TEXT("Recv VDisk List Error:%s"), (LPCTSTR)_bstr_t(Error));
				return false;
			}
			DWORD dwCount = 0;
			in.Pop(dwCount);
			//m_pLogger->WriteLog(LM_INFO, TEXT("Get VDisk List Count:%d"), dwCount);
			for (DWORD idx=0; idx<dwCount; idx++)
			{
				tagVDInfoEx* pVDisk = new tagVDInfoEx;
				ZeroMemory(pVDisk, sizeof(tagVDInfoEx));

				int svrDrv = 0, cliDrv = 0;
				in >> pVDisk->ID >> pVDisk->dwIp >> pVDisk->nPort;
				in >> svrDrv >> cliDrv;
				in >> pVDisk->Type >> pVDisk->LoadType >> pVDisk->svrMode;
				in >> pVDisk->cacheSize >> pVDisk->ssdDrv;

				pVDisk->szSvrDrv = svrDrv;
				pVDisk->szCliDrv = cliDrv;

				pVDisk->handle  = NULL;
				if (pVDisk->Type == VDISK_TYPE_I8DESK)
					pVDisk->IsLocalMachine = _IsLocalMachineVDisk(pVDisk);
				else
					pVDisk->IsLocalMachine = FALSE;

				if (pVDisk->IsLocalMachine)
					VDList.insert(std::make_pair(pVDisk->szSvrDrv, pVDisk));
				else
					delete pVDisk;
			}
			//m_pLogger->WriteLog(LM_INFO, TEXT("End VDisk List."));
		}
		catch (...)
		{
			FreeVDisk(VDList);
			m_pLogger->WriteLog(LM_INFO, TEXT("GetVDiskList Unknown Error."));
			return false;
		}

		return true;
	}


	bool CSocket::ReportVDiskStatus(std::map<DWORD, tagVDInfoEx*>& VDisk)
	{
		char outBuf[64 * 1024] = {0}; 

		try
		{
			VDiskPkHelper out(outBuf, 64 * 1024, CMD_VDSVR_REPSTATUS);
			out.Push(VDisk.size());

			for (std::map<DWORD, tagVDInfoEx*>::iterator it = VDisk.begin();
				it != VDisk.end(); it++)
			{
				tagVDInfoEx* pVDisk = it->second;
				tagVDInfo* pInfo = pVDisk;
				m_pPlug->SendMessage(VDISK_CMD_GETINFO, (DWORD)pVDisk->handle, (DWORD)pInfo);

				out << pVDisk->ID << pVDisk->dwVersion << pVDisk->dwConnect;
			}

			//10新增：最后多一个计算机名
			{

				TCHAR szCmpName[MAX_PATH] = {0};
				DWORD dwNameLen = _countof(szCmpName) - 1;
				GetComputerName(szCmpName, &dwNameLen);
				out.PushString(szCmpName, lstrlen(szCmpName));
			}


			char* pData = NULL;
			int nSize = 0;
			if (!_SendCmd(out.GetBuffer(), out.GetLength(), pData, nSize))
			{

				m_pLogger->WriteLog(LM_INFO, TEXT("Exec Report VDisk Status cmd Error"));
				return false;
			}

			utility::CArrayCharPtr data(pData);
			CPkgHelper in(pData);
			DWORD dwStatus = 0;
			in.Pop(dwStatus);

			if (dwStatus == CMD_RET_FAIL)
			{
				TCHAR Error[8096] = {0};
				in.PopString(Error);
				m_pLogger->WriteLog(LM_INFO, TEXT("Report VDisk Status Error:%s"), Error);
				return false;
			}
			//m_pLogger->WriteLog(LM_INFO, TEXT("Report VDisk Status Success."));
		}
		catch (...) 
		{ 
			assert(0);
			m_pLogger->WriteLog(LM_INFO, TEXT("Report VDisk Status Unknown Error."));
		}

		return true;
	}

	bool CSocket::ReportVDiskClientStatus(std::map<DWORD, tagVDInfoEx*> &vDisks)
	{
		try
		{
			typedef std::vector<VDiskClientStatus *> VDiskClientStatuses;
			typedef std::vector<VDiskClientStatus *> VDiskClientStatusPtr;
			VDiskClientStatuses vDiskClients;
			VDiskClientStatusPtr vDiskClientsPtr;

			for(std::map<DWORD, tagVDInfoEx*>::const_iterator iter = vDisks.begin();
				iter != vDisks.end(); ++iter)
			{
				VDiskClientStatus *tmp = 0;
				size_t cnt = m_pPlug->SendMessage(VDISK_CMD_GETINFOEX, (DWORD)iter->second->handle, (DWORD)&tmp);
				if( cnt == 0 )
					continue;

				for(size_t i = 0; i != cnt; ++i)
				{
					tmp[i].vDiskInfo = iter->second;
					vDiskClients.push_back(&tmp[i]);
				}

				vDiskClientsPtr.push_back(tmp);
			}

			size_t clientsCnt = vDiskClients.size();
			if( clientsCnt == 0 )
				return false;

			std::vector<char> outBuf(sizeof(VDiskClientStatus) * clientsCnt + 128);
			VDiskPkHelper out(&outBuf[0], outBuf.size(), CMD_VDSVR_VDISKCLIENT);

			out.Push(clientsCnt);
			for(VDiskClientStatuses::const_iterator it = vDiskClients.begin(); it != vDiskClients.end(); ++it)
			{
				VDiskClientStatus *vDiskClient = *it;
				if( vDiskClient == 0 )
					continue;

				utility::Strcpy(vDiskClient->VID, vDiskClient->vDiskInfo->ID);
				vDiskClient->VIP = vDiskClient->vDiskInfo->dwIp;
				out << vDiskClient->ClientIP << vDiskClient->VID
					<< vDiskClient->VIP << vDiskClient->ReadCount
					<< vDiskClient->ReadSpeed << vDiskClient->ReadMax << vDiskClient->ConnectTime
					<< vDiskClient->CacheShooting;
			}

			std::for_each(vDiskClientsPtr.begin(), vDiskClientsPtr.end(), ::CoTaskMemFree);


			char* pData = NULL;
			int nSize = 0;
			if (!_SendCmd(out.GetBuffer(), out.GetLength(), pData, nSize))
			{
				m_pLogger->WriteLog(LM_INFO, TEXT("Exec Report VDiskClient Status cmd Error"));
				return false;
			}

			utility::CArrayCharPtr data(pData);
			CPkgHelper in(pData);
			DWORD dwStatus = 0;
			in.Pop(dwStatus);

			if (dwStatus == CMD_RET_FAIL)
			{
				TCHAR Error[8096] = {0};
				in.PopString(Error);
				m_pLogger->WriteLog(LM_INFO, TEXT("Report VDiskClient Status Error:%s"), Error);
				return false;
			}
		}
		catch (...) 
		{ 
			assert(0);
			m_pLogger->WriteLog(LM_INFO, TEXT("Report VDiskClient Status Unknown Error."));
		}

		return true;
	}


	bool CSocket::ReportServerStatus(std::map<DWORD, tagVDInfoEx*>& VDisks, DWORD bInnerUpdate, ISvrPlug* pPlug)
	{
		/*
		帧域		内容		数据类型	说明
		HostName	服务器名称	String	长度<64
		I8DeskSvr	主服务状态	DWORD	0：此字段无效，
		1：正常，
		2：未安装
		3：未启用
		-1：异常，
		下同
		I8VDiskSvr	虚拟盘服务状态	DWORD	
		DNAService	三层下载服务状态	DWORD	
		I8MallCashier	商城收银端状态	DWORD	
		LastBackupDBTime	上次备份数据库时间	DWORD	Time32_t
		CPUUtilization	利用率	DWORD	%
		MemoryUsage	内存使用率	DWORD	KB
		内存剩余值  DWORD   KB
		网络发送速度DWORD   KB	修改成网络利用率
		网络接收速度DWORD   KB	未用

		DiskCount	磁盘数目	DWORD	
		第一个磁盘数据	Partition	DWORD	盘符
		Type	DWORD	1：FAT32，2：NTFS
		0：其他
		Capacity	ULONGLONG	Byte
		UsedSize	ULONGLONG	Byte
		FreeSize	ULONGLONG	Byte
		speed		DWORD		kb/s.
		…			
		*/
		using namespace i8desk;

		char outBuf[1024 * 64] = {0};

		try
		{
			VDiskPkHelper out(outBuf, 1024 * 64, CMD_SVRRPT_SERVERINFO_EX);
			TCHAR szBuf[MAX_PATH] = {0};
			DWORD nBufSize = _countof(szBuf) - 1;
			GetComputerName(szBuf, &nBufSize);

			std::vector<stdex::tString> localIPs;
			utility::GetLocalIPs(localIPs);

			ulonglong dwLeft = 0;

			out << szBuf;							//计算机名
			for(size_t i = 0; i != 4; ++i)			// IP
			{
				DWORD ip = 0;
				if( localIPs.size() > i )
					ip = async::network::IPAddress::Parse((LPCSTR)CT2A(localIPs[i].c_str()));
				out << ::htonl(ip);
			}
				
			out << 0								//主服务 (0无效)
				<< 1								//虚拟盘服务状态 (1正常)
				<< bInnerUpdate						//内网更新状态
				<< 0								//DNAService (0无效)
				<< 0								//商城收银端 (0无效)
				<< 0								//数据库上备份的时间
				<< i8desk_GetCpuUsage()				//cpu利用率
				<< i8desk_GetMemoryUsage(dwLeft)	//memory利用率
				<< dwLeft							//内存剩余值
				<< 0								//网络发送速度 -->改变成网络利用率.2010公测版本
				<< 0;								//网络接收速度 -->未用(0)

			stdex::tString str = GetServerDeviceInfo();
			out << str;					//设备信息

			std::set<TCHAR> setDrivers;
			{
				//添加虚拟盘的盘符
				for (std::map<DWORD, tagVDInfoEx*>::iterator it = VDisks.begin();
					it != VDisks.end(); it++)
				{
					setDrivers.insert((TCHAR)::toupper(it->first));
				}

				//添加同步任务的源目录盘符
				try
				{
					if (pPlug != NULL)
					{
						TCHAR buf[30] = {0};
						DWORD dwSize = 0;
						//cmd = 1.表示取同步任务的盘符命令。wparam:表示存储盘符的缓存区。lparam:表示存储返回写进缓冲的盘符个数的地址。
						pPlug->SendMessage(1, (DWORD)(&buf), (DWORD)(&dwSize));
						for (DWORD idx=0; idx< dwSize; idx++)
						{
							setDrivers.insert(::toupper(buf[idx]));
						}
					}
				}
				catch (...) 
				{
					assert(0);
				}
			}

			out.Push(setDrivers.size());
			for (std::set<TCHAR>::const_iterator it = setDrivers.begin();
				it != setDrivers.end(); it++)
			{
				DWORD dwType = 0, dwSize = 0, dwUsed = 0, dwFree = 0;

				TCHAR chDrv = (*it);
				uint32 Partition = (*it);
				i8desk_GetDriverInfo(chDrv, dwType, dwSize, dwUsed, dwFree);
				out << Partition << dwType << (ulonglong)dwSize * 1024 << (ulonglong)dwUsed * 1024 << (ulonglong)dwFree * 1024 << 0;

			}

			char* pData = NULL;
			int nSize = 0;
			if (!_SendCmd(out.GetBuffer(), out.GetLength(), pData, nSize))
			{
				m_pLogger->WriteLog(LM_INFO, TEXT("Exec Report Server Status cmd Error"));
				return false;
			}

			utility::CArrayCharPtr data(pData);
			CPkgHelper in(data);
			DWORD dwStatus = 0;
			in.Pop(dwStatus);
			if (dwStatus == CMD_RET_FAIL)
			{
				TCHAR Error[8096] = {0};
				in.PopString(Error);
				m_pLogger->WriteLog(LM_INFO, TEXT("Report Server Status Error:%s"), Error);
				return false;
			}
		}
		catch (...) 
		{ 
			m_pLogger->WriteLog(LM_INFO, TEXT("Report Server Status Unknown Error."));
		}

		return true;
	}

	void CSocket::ProcessVDisk(std::map<DWORD, tagVDInfoEx*>& VDList, std::map<DWORD, tagVDInfoEx*>& newVDList)
	{
		std::map<DWORD, tagVDInfoEx*>::iterator it = VDList.begin();

		stdex::tString str;
		for (; it != VDList.end(); it++)
		{
			//如果不在新的虚拟盘列表时则停止
			if (newVDList.find(it->first) == newVDList.end() && it->second->handle != NULL)
			{
				m_pPlug->SendMessage(VDISK_CMD_STOP, (DWORD)it->second->handle, 0);
				//StopServer(it->second->handle);
				m_pLogger->WriteLog(LM_INFO, TEXT("Stop VDisk: [%C] %s"), (TCHAR)it->first, 
					it->second->handle != NULL ? TEXT("Success") : TEXT("Fail"));
				it->second->handle = NULL;
				str += (TCHAR)it->first;
			}
		}
		for (size_t idx=0; idx<str.size(); idx++)
		{
			it = VDList.find((DWORD)str[idx]);
			if (it != VDList.end())
			{
				delete it->second;
				VDList.erase(it);
			}
		}

		it = newVDList.begin();
		for (; it != newVDList.end(); it ++)
		{
			//如果不在己经启动的虚拟盘列表里，则添加虚拟盘并启动
			if (VDList.find(it->first) == VDList.end() && 
				it->second->Type == 0 &&	//而且还需要是i8desk类型的虚拟盘
				it->second->IsLocalMachine  //而且还需要是本机的虚拟盘
				)
			{
				tagVDInfoEx* pVDisk = new tagVDInfoEx;
				*pVDisk = *it->second;
				tagVDInfo* pInfo = pVDisk;
				pVDisk->handle = (HANDLE)m_pPlug->SendMessage(VDISK_CMD_START, (DWORD)pInfo, 0);
				m_pLogger->WriteLog(LM_INFO, TEXT("Start VDisk: [%c] %s"), pVDisk->szSvrDrv, 
					(pVDisk->handle != NULL) ? TEXT("Success") : TEXT("Fail"));

				if (pVDisk->handle != NULL)
					VDList.insert(std::make_pair(pVDisk->szSvrDrv, pVDisk));
				else
					delete pVDisk;
			}
		}
	}


	std::tr1::tuple<DWORD, DWORD> CSocket::GetFileInfo(const stdex::tString &file)
	{		
		char outBuf[64 * 1024] = {0};

		VDiskPkHelper out(outBuf, 64 * 1024, CMD_VDSVR_GETUPDATE_FILEINFO);
		out << file.length() << file;

		char *outData = NULL;
		int outLen = 0;
		if( !_SendCmd(out.GetBuffer(), out.GetLength(), outData, outLen) )
		{
			stdex::tString error = _T("获取文件: ") + file + _T(" 信息出错!");
			throw std::logic_error((char *)CT2A(error.c_str()));
		}

		utility::CArrayCharPtr data(outData);
		CPkgHelper in(data);
		DWORD dwStatus = 0;
		in.Pop(dwStatus);
		if( dwStatus == 0 )
		{
			stdex::tString error = _T("获取文件: ") + file + _T(" 信息出错!");
			throw std::runtime_error((char *)CT2A(error.c_str()));
		}

		DWORD dwSize = 0;	// File Size
		DWORD dwCRC = 0;	// File Data CRC

		in >> dwSize >> dwCRC;

		return std::tr1::make_tuple(dwSize, dwCRC);
	}

	void CSocket::GetFileData(const stdex::tString &file, char *&fileDataBuf, DWORD &recvSize)
	{
		// 首先获取文件长度
		DWORD dwFileSize = 0;

		{
			char outBuf[4 * 1024] = {0};

			VDiskPkHelper out(outBuf, 4 * 1024, CMD_CLIUPT_DOWNFILE_START);
			out << 1 << FP_REALPATH << file;

			char *outData = NULL;
			int outLen = 0;
			if( !_SendCmd(out.GetBuffer(), out.GetLength(), outData, outLen) )
			{
				stdex::tString error = _T("获取文件") + file + _T("长度出错!");
				throw std::logic_error((char *)CT2A(error.c_str()));
			}

			utility::CArrayCharPtr data(outData);
			CPkgHelper in(data);
			DWORD dwStatus = 0;
			in.Pop(dwStatus);
			if( dwStatus == 0 )
			{
				stdex::tString error = _T("获取文件: ") + file + _T(" 数据出错!");
				throw std::runtime_error((char *)CT2A(error.c_str()));
			}

			dwFileSize = 0;
			in.Pop(dwFileSize);	// File Size
		}

		// 循环获取文件数据
		char outBuf[4 * 1024] = {0};
		utility::CArrayCharPtr pRecvDataBuf(new char[dwFileSize]);

		DWORD recvFileSize = 0;
		while(recvFileSize < dwFileSize)
		{
			VDiskPkHelper out(outBuf, 4 * 1024, CMD_DOWNFILE_NEXT);

			char *outData = NULL;
			int outLen = 0;
			if( !_SendCmd(out.GetBuffer(), out.GetLength(), outData, outLen) )
			{
				stdex::tString error = _T("获取文件") + file + _T("数据出错!");
				throw std::logic_error((char *)CT2A(error.c_str()));
			}

			utility::CArrayCharPtr data(outData);
			CPkgHelper in(data);
			DWORD dwStatus = 0;
			in.Pop(dwStatus);
			if( dwStatus != 1 )
			{
				stdex::tString error = _T("获取文件") + file + _T("数据出错!");
				throw std::logic_error((char *)CT2A(error.c_str()));
			}

			DWORD dwLastFlag = 0;
			in.Pop(dwLastFlag);
			DWORD curSize = 0;
			in.Pop(curSize);
			if( curSize > (DWORD)std::numeric_limits<int>::max() || 
				recvFileSize + curSize > dwFileSize )
			{
				stdex::tString error = _T("获取文件") + file + _T("数据出错!");
				throw std::logic_error((char *)CT2A(error.c_str()));
			}

			in.PopPointer(reinterpret_cast<BYTE *>(pRecvDataBuf.m_Handle) + recvFileSize, curSize);

			recvFileSize += curSize;

			// 最后一个包
			if( dwLastFlag == 1 )
				break;

			// 网络接收延时
			::Sleep(10);
		}

		if( recvFileSize != dwFileSize )
		{
			stdex::tString info = file + _T("接收的文件数据大小出错");
			throw std::logic_error((char *)CT2A(info.c_str()));
		}

		fileDataBuf = pRecvDataBuf.Detach();
		recvSize = dwFileSize;
	}

	bool CSocket::_SendCmd(const char* pOutData, int outlength, char*& pInData, int& inlength)
	{
		if (pOutData == NULL || outlength == 0)
		{
			m_pLogger->WriteLog(LM_INFO, TEXT("parameter error or func call erorr."));
			return false;
		}
		pInData = NULL;	inlength = 0;

		//send data.
		int left = outlength;
		DWORD dwError = 0;
		while (left)
		{
			int len = send(m_Socket, &pOutData[outlength-left], left, 0);
			if (len <= 0)
			{
				dwError = WSAGetLastError();
				_com_error error(dwError);
				m_pLogger->WriteLog(LM_INFO, TEXT("Send Data Fail:%d:%s"), dwError, error.ErrorMessage());
				CloseSocket();
				return false;
			}
			left -= len;
		}

		//recv data.
		DWORD dwSize = 1024 * 16;
		char* pData = new char[dwSize];
		DWORD dwRecvLen = 0;
		int nTimes = 0;	//超时重接收次数.一次只使用1秒.因此有几次就是几秒接时才超时
		while (1)
		{
			FD_SET fdset;
			timeval tv = {1, 0};
			FD_ZERO(&fdset);
			FD_SET(m_Socket, &fdset);
			int ret = select(0, &fdset, NULL, NULL, &tv);
			if (ret == 0)
			{
				//timeout.
				if (nTimes < 10)
				{
					nTimes ++;
					if (WaitForSingleObject(m_hExited, 0) == WAIT_OBJECT_0)
					{
						m_pLogger->WriteLog(LM_INFO, TEXT("Service Should Exit."));
						delete []pData;
						return false;
					}
					continue;
				}
				m_pLogger->WriteLog(LM_INFO, TEXT("Recv Ack Data Timeouts."));
				CloseSocket();
				delete []pData;
				return false;
			}
			else if (ret < 0)
			{
				dwError = WSAGetLastError();
				_com_error error(dwError);
				m_pLogger->WriteLog(LM_INFO, TEXT("Select Fail:%d:%s"), dwError, error.ErrorMessage());
				delete []pData;
				CloseSocket();
				return false;
			}

			if (FD_ISSET(m_Socket, &fdset))
			{
				int len = recv(m_Socket, &pData[dwRecvLen], dwSize - dwRecvLen, 0);
				if (len <= 0)
				{
					dwError = WSAGetLastError();
					_com_error error(dwError);
					m_pLogger->WriteLog(LM_INFO, TEXT("Recv Data Fail:%d:%s"), dwError, error.ErrorMessage());
					delete []pData;
					CloseSocket();
					return false;
				}

				dwRecvLen += len;
				if (dwRecvLen >= sizeof(i8desk::pkgheader))
				{
					//过滤错误数据包。
					{
						WORD dwStartFlag = *(WORD*)pData;
						if (dwStartFlag != START_FLAG)
						{
							m_pLogger->WriteLog(LM_INFO, _T("Recv Data Flag Error."));
							delete[]pData;
							CloseSocket();
							return false;
						}
					}
					DWORD dwTotal = ((i8desk::pkgheader*)pData)->Length;
					if (dwSize < dwTotal)
					{
						dwSize = dwTotal+16;
						char* pTemp = new char[dwSize];
						memcpy(pTemp, pData, dwRecvLen);
						delete []pData;
						pData = pTemp;
					}
					if (dwRecvLen >= dwTotal)
						break;
				}
			}
		}
		pInData = pData;
		inlength = dwRecvLen;
		return true;
	}


	BOOL CSocket::_IsLocalMachineVDisk(tagVDInfoEx* pVDisk)
	{
		IP_ADAPTER_INFO info[16] = {0};
		DWORD dwSize = sizeof(info);
		if (ERROR_SUCCESS != GetAdaptersInfo(info, &dwSize))
		{
			DWORD dwError = GetLastError();
			_com_error Error(dwError);
			m_pLogger->WriteLog(LM_INFO, TEXT("GetAdaptersInfo Error:%d:%s"), dwError, Error.ErrorMessage());
			return FALSE;
		}

		PIP_ADAPTER_INFO pAdapter = info;
		while (pAdapter != NULL)
		{
			PIP_ADDR_STRING pAddr = &pAdapter->IpAddressList;
			while (pAddr != NULL)
			{
				DWORD dwIp = inet_addr(pAddr->IpAddress.String);
				if (dwIp == pVDisk->dwIp)
					return TRUE;
				pAddr = pAddr->Next;
			}
			pAdapter = pAdapter->Next;
		}
		return FALSE;
	}

}