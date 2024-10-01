#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <vector>

namespace PhiveSocketConnector
{
	enum class OperationType : uint32_t
	{
		FX_OP_LOADPHIVECONFIG = 0,
		FX_OP_PHIVETOOBJ = 1,
		FX_OP_OBJTOPHIVE = 2,
		FX_OP_RAWTOPHIVE = 3
	};

	enum class RespType
	{
		Error = 0,
		LoadedPhiveConfig = 1,
		ObjAndMats = 2,
		Phive = 3,
	};

	struct InHeader
	{
		RespType mResponseType;
		uint32_t mMessageSize;
	};

	extern sockaddr_in ClientService;
	extern SOCKET ClientSocket;
	extern WSADATA WsaData;
	extern int WsErr;
	extern bool IsConnected;

	void Initialize();
	bool Connect();
	bool SendData(OperationType Op, std::vector<unsigned char> Data);
	void Disconnect();
	std::vector<unsigned char> GetData();
}