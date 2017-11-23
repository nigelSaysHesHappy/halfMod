#ifndef nigsock
#define nigsock

#define VER		"v0.4.3-limited-halfShell"

class nigSock
{
	public:
		std::string mark;
		std::string ip;
		int port;
		int sock = -1;
		nigSock();
		~nigSock();
		
		bool isOpen();
		void assign(int socket, struct sockaddr_in addr);
//		int nRead(std::string &buffer,int size);
		int nRead(std::string &buffer);
		int nWrite(std::string buffer, int flags = 0);
		void nClose();
};

#endif

