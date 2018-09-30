#ifndef nigsock
#define nigsock

#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define VER		"v0.4.5-limited-halfShell"

extern ssize_t (*original_read)(int, void*, size_t);

class nigSock
{
	public:
		std::string mark;
		std::string ip;
		int port = 0;
		int sock = -1;
		nigSock() {}
		nigSock(int socket, const sockaddr_in &addr) { assign(socket,addr); }
		~nigSock() { nClose(); }
		
		bool isOpen()
		{
            bool r = true;
	        if (sock < 1)
	            r = false;
	        return r;
        }
        
		void assign(int socket, const sockaddr_in &addr)
		{
	        sock = socket;
	        ip = inet_ntoa(addr.sin_addr);
	        port = ntohs(addr.sin_port);
	        mark.clear();
        }
        
		ssize_t nRead(std::string &buffer)
		{
	        buffer.clear();
	        char c[1];
	        ssize_t s = 0;
	        while (original_read(sock,c,1) > 0)
	        {
		        if ((c[0] == '\r') || (c[0] == '\n'))
		        {
			        if (!s) s = -1;
			        break;
		        }
		        buffer += c[0];
		        s++;
	        }
	        return s;
        }
        
		ssize_t nWrite(const std::string &buffer, int flags = 0)
		{
            ssize_t r = 0;
            if (sock > 0)
            	r = send(sock,buffer.c_str(),buffer.size(),flags);
	        return r;
        }
        
		void nClose()
		{
	        close(sock);
	        sock = -1;
	        mark.clear();
	        ip.clear();
	        port = -1;
        }
};

#endif

