//#include <iostream>
//#include <time.h>
#include <string>
//#include <fstream>
#include <stdio.h>
#include <cstdlib>
//#include <errno.h>
#include <unistd.h>
//#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "nigsock.h"

using namespace std;

nigSock::nigSock()
{
	sock = -1;
	mark.clear();
	ip.clear();
	port = 0;
}

nigSock::~nigSock()
{
	nClose();
}

bool nigSock::isOpen()
{
	if (sock < 1) return false;
	return true;
}

void nigSock::assign(int socket, struct sockaddr_in addr)
{
	sock = socket;
	ip = inet_ntoa(addr.sin_addr);
	port = ntohs(addr.sin_port);
	mark.clear();
}

/*
int nigSock::nRead(string &buffer,int size)
{
	buffer = "";
	char *in = (char*)malloc(sizeof(char) * (size+1));
	int r = read(sock,in,size);
	in[r] = '\0';
	buffer = in;
	free(in);
	return r;
}
*/

int nigSock::nRead(string &buffer)
{
	buffer.clear();
	char c[1];
	int s = 0;
	while (read(sock,c,1) > 0)
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

int nigSock::nWrite(string buffer,int flags)
{
    if (sock != -1)
    	return send(sock,buffer.c_str(),buffer.size(),flags);
	return 0;
}

void nigSock::nClose()
{
	close(sock);
	sock = -1;
	mark.clear();
	ip.clear();
	port = -1;
}

