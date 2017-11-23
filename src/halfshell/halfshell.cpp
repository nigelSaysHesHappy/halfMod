#include <iostream>
#include <stdio.h>
#include <string.h>   //strlen
#include <cstdlib>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <time.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <regex>
//#include <fcntl.h>
#include ".hsBuild.h"
#include "str_tok.h"
#include "nigsock.h"
using namespace std;

//#define VERSION		"halfShell v0.2.42"
#define TRUE		1
#define FALSE		0
#define DEFPORT		9422
#define LOCALHOST	"127.0.0.1"
#define CONF		"halfshell.conf"
#define MAXCLIENTS	30
#define MAXSCREENLEN	400

int main(int argc , char *argv[])
{
	if (argc < 2)
	{
		cerr<<"Missing minecraft screen name"<<endl;
		return 1;
	}
	atomic<bool> showInput;
	showInput = false;
	string screen = argv[1];
	string mcver = "";
	cout<<"Minecraft server screen defined as "<<screen<<"."<<endl;
	if (argc > 2)
	    mcver = argv[2];
	int opt = TRUE;
	int addrlen, new_socket, activity, i, valread;
	int max_sd;
	int master_socket;
	atomic<int> connectedClients;
	connectedClients = 0;
	struct sockaddr_in address;
	struct timeval timeout;
	ifstream rFile;
	nigSock sockets[MAXCLIENTS];
	string line, buffer, temp;
	
	//set of socket descriptors
	fd_set readfds;//, stdinfds;
	
	for (i = 0;i < MAXCLIENTS;i++) 
	{
		sockets[i].sock = -1;
	}
	
	//create a master socket
	if((master_socket = socket(AF_INET,SOCK_STREAM,0)) == 0) 
	{
		cerr<<"socket creation failure . . ."<<endl;
		return 1;
	}
	
	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
	{
		cerr<<"setsockopt failure . . ."<<endl;
		return 1;
	}
	
	string bindIP = LOCALHOST;
	int bindPort = DEFPORT;
	string allowed = LOCALHOST;
	rFile.open(CONF);
	if (rFile.is_open())
	{
		while (getline(rFile,line))
		{
			if (iswm(nospace(line),"#*")) continue;
			line = lower(line);
			if (gettok(line,1,"=") == "ip") bindIP = gettok(line,2,"=");
			if (gettok(line,1,"=") == "port") bindPort = str2int(gettok(line,2,"="));
			if (gettok(line,1,"=") == "allowed-ips") allowed = deltok(line,1,"=");
		}
		rFile.close();
	}
	else
		cerr<<CONF<<" file missing. Using defaults."<<endl;
	cout<<"Binding connections to "<<bindIP<<":"<<bindPort<<endl;
	
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr( bindIP.c_str() );
	address.sin_port = htons( bindPort );
	
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
	{
		cerr<<"socket bind failure . . ."<<endl;
		return 1;
	}
	
	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket,3) < 0)
	{
		cerr<<"unable to start listen server . . ."<<endl;
		return 1;
	}
	
	//accept the incoming connection
	addrlen = sizeof(address);
	cout<<"Waiting for connections ...\n";
	
	thread([&]
	{
	    string in;
	    while (1)
	    {
	        getline(cin,in);
			if (in != "")
			{
				if (showInput)
					cout<<in<<endl;
				if (connectedClients > 0)
				{
					for (int j = 0; j < MAXCLIENTS; j++)
					{
						if (sockets[j].mark != "")
							sockets[j].nWrite(in + "\n");
					}
				}
				if (in == "]:-:-:-:[###[THREAD COMPLETE]###]:-:-:-:[")
				{
					for (int j = 0; j < MAXCLIENTS; j++)
					{
						if (sockets[j].isOpen())
							sockets[j].nClose();
					}
					close(master_socket);
					break;
				}
				in = "";
			}
		}
		cout<<"[!!] Minecraft server offline, halfShell restarting in 5 seconds . . ."<<endl;
		sleep(5);
		terminate();
	}).detach();
	
	while (1) 
	{
		//clear the socket set
		FD_ZERO(&readfds);
		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;
		
		//FD_ZERO(&stdinfds);
		//FD_SET(0, &readfds);
		
		//add child sockets to set
		for (i = 0;i < MAXCLIENTS;i++) 
		{
			//if valid socket descriptor then add to read list
			if(sockets[i].sock > 0)
				FD_SET( sockets[i].sock , &readfds);
			
			//highest file descriptor number, need it for the select function
			if(sockets[i].sock > max_sd)
				max_sd = sockets[i].sock;
		}
		
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		
		//wait for an activity on one of the sockets , timeout is 0, so skip if no activity
		//activity = select( max_sd + 1 , &readfds , NULL , NULL , &timeout);
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) 
		{
			//cerr<<"Select error.\n";
			cout<<"[!!] Minecraft server offline, halfShell restarting in 5 seconds . . ."<<endl;
			break;
		}
		//else if ((activity == 0) && (errno!=EINTR))
		//{
			// impossible timeout
		//}
		else
		{
			if (FD_ISSET(master_socket, &readfds)) 
			{
				if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
				{
					cerr<<"accept failure"<<endl;
					//exit(EXIT_FAILURE);
				}
				temp = inet_ntoa(address.sin_addr);
				if (!istok(allowed,temp," "))
				{
					close(new_socket);
					new_socket=-1;
					cout<<"Denied incoming connection from "<<temp<<". Not in the allowed-ips list."<<endl;
				}
				for (i = 0;i < MAXCLIENTS;i++)
				{
					if (!sockets[i].isOpen())
					{
						sockets[i].assign(new_socket,address);
						connectedClients++;
						break;
					}
					else if (i == MAXCLIENTS-1)
					{
						cerr<<"Too many open connections. . ."<<endl;
						//send(new_socket,"Sorry, no room for you right now!\n",35,0);
						close(new_socket);
						new_socket = -1;
					}
				}
				if (new_socket > 0)
				{
					cout<<"New connection <"<<i<<"> established with "<<sockets[i].ip<<":"<<sockets[i].port<<endl;
					cout<<"Total open connections: "<<connectedClients<<endl;
					temp = string(VERSION) + "\t" + screen + "\t" + mcver + "\n";
					sockets[i].nWrite(temp);
				}
			}
			
			for (i = 0;i < MAXCLIENTS;i++)
			{
				if (sockets[i].sock == 0)
					continue;
				if (FD_ISSET(sockets[i].sock,&readfds))
				{
					if ((valread = sockets[i].nRead(buffer)) == 0)
					{
							//socket is closing
						if (sockets[i].mark != "")
							cout<<"Host disconnect <"<<sockets[i].mark<<">: "<<sockets[i].ip<<":"<<sockets[i].port<<endl;
						else
							cout<<"Host disconnect <"<<i<<">: "<<sockets[i].ip<<":"<<sockets[i].port<<endl;
						sockets[i].nClose();
						connectedClients--;
						cout<<"Total open connections: "<<connectedClients<<endl;
					}
					else if (valread > 0)// read
					{
//						cout<<i<<">>"<<buffer<<endl;
						if (sockets[i].mark == "")
						{		// handshake
							cout<<"Client <"<<i<<"> is identifying as: "<<buffer<<endl;
							sockets[i].mark = buffer;
						}
						else
						{
						    //buffer = strreplace(buffer,"\"","\\\"") + "\r";
						    //buffer = strreplace(strreplace(buffer,"$","\\$"),"^","\\^") + "\r";
						    regex ptrn ("(\\\\+)?\\$");
						    buffer = regex_replace(buffer,ptrn,"\\$");
						    ptrn = "(\\\\+)?\\^";
						    buffer = regex_replace(buffer,ptrn,"\\^");
						    ptrn = "'";
						    buffer = regex_replace(buffer,ptrn,"`");
						    buffer += "\r";
							valread = buffer.size();
							for (int i = 0;i <= valread;i += MAXSCREENLEN)
							{
								temp = "screen -S " + screen + " -p 0 -X stuff '" + strmid(buffer,i,MAXSCREENLEN) + "'";
								while (temp.at(temp.size()-2) == '\\')
								{
								    temp.erase(temp.size()-2,1);
								    i--;
								}
								if (temp == "screen -S " + screen + " -p 0 -X stuff ''")
								    i += MAXSCREENLEN;
								system(temp.c_str());
							}
						}
					}
				}
			}
		}
	}
	sleep(5);
	return 0;
}

