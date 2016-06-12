#define _WINSOCKAPI_
#include <iostream>
#include "thread.h"
#include "var.h"


#undef _WINSOCKAPI_

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

struct __socket_library
{
	__socket_library()
	{
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~__socket_library()
	{
		WSACleanup();
	}
};

class mysrv :public lyramilk::system::threading::threads
{
	SOCKET sock;
	virtual int svc()
	{
		struct sockaddr_in caddr = { 0 };
		int addrlen = sizeof(caddr);

		while (true){
			SOCKET chsock = accept(sock, (sockaddr*)&caddr, &addrlen);
			char buff[65536];
			int r = recv(chsock, buff, 65536, 0);
			std::cout.write(buff, r);

			lyramilk::data::string q = "<html><head><title>lyramilk的个人主页SSL</title></head><body>HTTP测试</body></html>";

			lyramilk::data::stringstream ss;
			ss << "HTTP/1.1 200 OK\r\nServer:teapoy/3.0\r\nContent-Type:text/html;charset=gbk\r\nContent-Length:" << q.size() << "\r\n\r\n" << q << std::endl;
			lyramilk::data::string p = ss.str();
			send(chsock, p.c_str(), p.size(), 0);
			closesocket(chsock);
		}


		return 0;
	}
public:
	mysrv() :sock(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
	{}
	virtual ~mysrv()
	{
		closesocket(sock);
	}
	bool open(unsigned short port)
	{
		struct sockaddr_in addr = { 0 };

		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);

		if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
			return false;
		}

		listen(sock, 5);

		return true;
	}
};





class mysrv2 :public lyramilk::system::threading::threads
{
	SOCKET sock;
	HANDLE hroot;
	virtual int svc()
	{
		struct mydata{
			enum type{
				dt_accept,
				dt_recv,
				dt_send,
			}t;
			SOCKET sock;
			DWORD bytescount;
			OVERLAPPED ov;
			WSABUF buf;
			char buff[4096];
			mydata()
			{
				std::cout << "创造" << this << std::endl;
				sock = 0;
				bytescount = 0;
				memset(&ov, 0, sizeof(ov));
				ov.Pointer = this;
				buf.buf = buff;
				buf.len = 4096;
			}
			~mydata()
			{
				std::cout << "释放" << this << std::endl;
			}
		};
		int addrlen = sizeof(sockaddr_in);
		char* recvbuff = new char[4096];
		{
			SOCKET chsock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			CreateIoCompletionPort((HANDLE)chsock, hroot, (ULONG_PTR)4, 0);

			mydata* d = new mydata;
			d->t = mydata::dt_accept;
			d->sock = chsock;
			BOOL bret = AcceptEx(sock, chsock, recvbuff, 4096, addrlen + 16, addrlen + 16, &d->bytescount, &d->ov);
			if (bret == FALSE){
				if (WSAGetLastError() != ERROR_IO_PENDING){
					std::cout << WSAGetLastError() << std::endl;
					return -1;
				}
			}

		}

		while (true){
			DWORD dwTransfered = 0;
			ULONG_PTR ulptr = 0;
			LPOVERLAPPED ovp = NULL;

			int ret = GetQueuedCompletionStatus(hroot, &dwTransfered, &ulptr, &ovp, INFINITE);
			mydata* pdata = (ovp == NULL ? NULL : (mydata*)ovp->Pointer);
			std::cout << "dwTransfered=" << dwTransfered << std::endl;
			if (pdata && pdata->t != mydata::dt_accept){
				if (dwTransfered == 0){
					//closesocket(pdata->sock);
					delete pdata;
					continue;
				}
				if (ret == 0){
					std::cout << WSAGetLastError() << std::endl;
					continue;
				}
			}
			if (pdata->t == mydata::dt_accept){
				std::cout << "accept " << pdata->sock << std::endl;
				//CreateIoCompletionPort((HANDLE)pdata->sock, hroot, (ULONG_PTR)5, 0);

				{
					DWORD flag = 0;
					mydata* d = new mydata;
					d->t = mydata::dt_recv;
					d->sock = pdata->sock;
					int ret = WSARecv(d->sock, &d->buf, 1, &d->bytescount, &flag, &d->ov, NULL);
					if (ret == -1){
						if (WSAGetLastError() != ERROR_IO_PENDING){
							std::cout << WSAGetLastError() << std::endl;
							return -1;
						}
					}
				}
				if (dwTransfered > 0){

					lyramilk::data::string q = "<html><head><title>lyramilk的个人主页SSL</title></head><body>HTTP测试</body></html>";

					lyramilk::data::stringstream ss;
					ss << "HTTP/1.1 200 OK\r\nServer:teapoy/3.0\r\nContent-Type:text/html;charset=gbk\r\nContent-Length:" << q.size() << "\r\n\r\n" << q << std::endl;
					lyramilk::data::string p = ss.str();
					DWORD flag = 0;
					mydata* d = new mydata;
					d->t = mydata::dt_send;
					d->sock = pdata->sock;
					p.copy(d->buf.buf, 4096);
					d->buf.len = p.size();

					int ret = WSASend(d->sock, &d->buf, 1, &d->bytescount, 0, &d->ov, NULL);
					if (ret == -1){
						if (WSAGetLastError() != ERROR_IO_PENDING){
							std::cout << WSAGetLastError() << std::endl;
							return -1;
						}
					}
				}


				pdata->sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
				CreateIoCompletionPort((HANDLE)pdata->sock, hroot, (ULONG_PTR)4, 0);

				BOOL bret = AcceptEx(sock, pdata->sock, recvbuff, 4096, addrlen + 16, addrlen + 16, &pdata->bytescount, &pdata->ov);
				if (bret == FALSE){
					if (WSAGetLastError() != ERROR_IO_PENDING){
						std::cout << WSAGetLastError() << std::endl;
						return -1;
					}
				}

			}else if (pdata->t == mydata::dt_recv){
				std::cout << "recv " << pdata->sock << std::endl;
				if (dwTransfered > 0){

					std::cout.write(recvbuff, dwTransfered);

					lyramilk::data::string q = "<html><head><title>lyramilk的个人主页SSL</title></head><body>HTTP测试</body></html>";

					lyramilk::data::stringstream ss;
					ss << "HTTP/1.1 200 OK\r\nServer:teapoy/3.0\r\nContent-Type:text/html;charset=gbk\r\nContent-Length:" << q.size() << "\r\n\r\n" << q << std::endl;
					lyramilk::data::string p = ss.str();
					DWORD flag = 0;
					mydata* d = new mydata;
					d->t = mydata::dt_send;
					d->sock = pdata->sock;
					p.copy(d->buf.buf, 4096);
					d->buf.len = p.size();

					int ret = WSASend(d->sock, &d->buf, 1, &d->bytescount, 0, &d->ov, NULL);
					if (ret == -1){
						if (WSAGetLastError() != ERROR_IO_PENDING){
							std::cout << WSAGetLastError() << std::endl;
							return -1;
						}
					}

				}
				else{
					closesocket(pdata->sock);
					delete pdata;
				}
			}
			else if (pdata->t == mydata::dt_send){
				std::cout << "send " << pdata->sock << std::endl;
				closesocket(pdata->sock);
				delete pdata;
			}



		}


		return 0;
	}
public:
	mysrv2() :sock(WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED))
	{
		hroot = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
		CreateIoCompletionPort((HANDLE)sock, hroot, (ULONG_PTR)5, 0);
		std::cout << "listen = " << sock << std::endl;
	}
	virtual ~mysrv2()
	{
		closesocket(sock);
	}
	bool open(unsigned short port)
	{
		struct sockaddr_in addr = { 0 };

		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);

		if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
			return false;
		}

		listen(sock, 5);

		return true;
	}
};


int main(int argc,const char* argv[])
{
	__socket_library __init_socket;
	mysrv2 s;
	s.open(80);
	s.active(1);


	int i = 0;
	std::cin >> i;
	return 0;
}
