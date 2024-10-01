#include "PhiveSocketConnector.h"

#include "Logger.h"
#include "BinaryVectorWriter.h"

sockaddr_in PhiveSocketConnector::ClientService;
SOCKET PhiveSocketConnector::ClientSocket = INVALID_SOCKET;
WSADATA PhiveSocketConnector::WsaData;
int PhiveSocketConnector::WsErr = 0;
bool PhiveSocketConnector::IsConnected = false;

void PhiveSocketConnector::Initialize()
{
	WORD wVersionRequested = MAKEWORD(2, 2);
	WsErr = WSAStartup(wVersionRequested, &WsaData);

	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for socket creation success
	if (ClientSocket == INVALID_SOCKET)
	{
		Logger::Error("PhiveSocketConnector", "Could not create socket");
		WSACleanup();
		return;
	}
	else
	{
		Logger::Info("PhiveSocketConnector", "Socket created");
	}

	ClientService.sin_family = AF_INET;
	ClientService.sin_addr.s_addr = inet_addr("127.0.0.1");
	ClientService.sin_port = htons(8778);
}

bool PhiveSocketConnector::Connect()
{
	if (connect(ClientSocket, reinterpret_cast<SOCKADDR*>(&ClientService), sizeof(ClientService)) == SOCKET_ERROR)
	{
		Logger::Warning("PhiveSocketConnector", "Could not connect to socket");
		return false;
	}
	
	Logger::Info("PhiveSocketConnector", "Connected to socket");
	IsConnected = true;

	return true;
}

std::vector<unsigned char> PhiveSocketConnector::GetData()
{
	InHeader Header;
	int RByteCount = recv(ClientSocket, reinterpret_cast<char*>(&Header), sizeof(Header), 0);
	if (RByteCount < 0)
	{
		Logger::Error("PhiveSocketConnector", "Could not get response header");
		return std::vector<unsigned char>();
	}

	if (Header.mResponseType == RespType::Phive)
	{
		std::vector<unsigned char> Bytes;
		while (Bytes.size() < Header.mMessageSize)
		{
			std::vector<unsigned char> Data(Header.mMessageSize - Bytes.size());
			RByteCount = recv(ClientSocket, reinterpret_cast<char*>(Data.data()), Header.mMessageSize - Bytes.size(), 0);
			Data.resize(RByteCount);
			Bytes.insert(Bytes.end(), Data.begin(), Data.end());
		}
		return Bytes;
	}

	return std::vector<unsigned char>();
}

bool PhiveSocketConnector::SendData(OperationType Op, std::vector<unsigned char> Data)
{
	BinaryVectorWriter Writer;
	Writer.WriteInteger((uint32_t)Op, sizeof(uint32_t));
	Writer.WriteInteger(Data.size(), sizeof(uint32_t));
	Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(Data.data()), Data.size());
	std::vector<unsigned char> FormattedData = Writer.GetData();

	uint32_t SentSize = 0;

	while (SentSize < FormattedData.size())
	{
		int Size = send(ClientSocket, reinterpret_cast<const char*>(FormattedData.data() + SentSize), FormattedData.size() - SentSize, 0);
		if (Size < 0)
		{
			Logger::Error("PhiveSocketConnector", "Error while sending data");
			return false;
		}
		SentSize += Size;
	}
	
	Logger::Info("PhiveSocketConnector", "Sent " + std::to_string(SentSize) + " bytes");

	return true;
}

void PhiveSocketConnector::Disconnect()
{
	WSACleanup();
}