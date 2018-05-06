#include "halfshell_func.h"
using namespace std;

string mcver;

int main(int argc , char *argv[])
{
	if (argc < 2)
	{
		cerr<<"Missing minecraft screen name"<<endl;
		return 1;
	}
	bool showInput = false;
	string screen = argv[1];
	mcver = "";
	cout<<"Minecraft server screen defined as "<<screen<<"."<<endl;
	if (argc > 2)
	    mcver = argv[2];
	int addrlen, master_socket;
	int connectedClients = 0;
	struct sockaddr_in address;
	nigSock *sockets;
	
	string bindIP, allowed;
	int bindPort;
	loadConfig(CONF,bindIP,bindPort,allowed);
	int error;
	sockets = serverInit(master_socket,addrlen,address,bindIP,bindPort,MAXCLIENTS,error);
	switch (error)
	{
	    case 1:
	    {
	        cerr<<"Failure creating master socket . . .\n";
	        break;
        }
        case 2:
        {
            cerr<<"Failure enabling simultaneous socket connections . . .\n";
            break;
        }
        case 3:
        {
            cerr<<"Unable to begin listening for incoming connections . . .\n";
            break;
        }
        case 4:
        {
            cerr<<"Unable to bind listening server to "<<bindIP<<":"<<bindPort<<" Is something already using this port? . . .";
            break;
        }
        case 0:
        {
            cout<<"Listening for connections on "<<bindIP<<":"<<bindPort<<" ...\n";
            break;
        }
        default:
            cerr<<"An unknown error has occurred . . .\n";
    }
    if (error)
        return error;
	
	thread([&sockets, &showInput, &master_socket, &connectedClients]()
	//thread([&]
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
				    sendToAllClients(sockets,in + "\n");
				if (in == "]:-:-:-:[###[THREAD COMPLETE]###]:-:-:-:[")
				{
				    closeAllSockets(master_socket,sockets);
					break;
				}
				in = "";
			}
		}
		cout<<"[!!] Minecraft server offline, halfShell restarting in 5 seconds . . ."<<endl;
		sleep(5);
		terminate();
	}).detach();
	
	int max_sd, activity;
	//set of socket descriptors
	fd_set readfds;
	
	while (1) 
	{
	    max_sd = prepSockets(readfds,master_socket,sockets);
		
		//wait for an activity on one of the sockets , timeout is null so wait forever
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) 
		{
			cout<<"[!!] Minecraft server offline, halfShell restarting in 5 seconds . . ."<<endl;
			break;
		}
		else
		{
			if (FD_ISSET(master_socket, &readfds)) 
			    acceptNewClient(address,addrlen,master_socket,sockets,connectedClients,allowed,screen);
			handleClients(readfds,sockets,connectedClients,screen);
		}
	}
	sleep(5);
	return 0;
}

