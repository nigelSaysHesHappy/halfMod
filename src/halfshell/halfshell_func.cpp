#include "halfshell_func.h"
using namespace std;

void loadConfig(string configFile, string &ip, int &port, string &allow)
{
    ip = LOCALHOST;
	port = DEFPORT;
	allow = LOCALHOST;
    ifstream rFile(configFile);
    string line;
	if (rFile.is_open())
	{
		while (getline(rFile,line))
		{
			if (iswm(nospace(line),"#*")) continue;
			line = lower(line);
			if (gettok(line,1,"=") == "ip") ip = gettok(line,2,"=");
			if (gettok(line,1,"=") == "port") port = str2int(gettok(line,2,"="));
			if (gettok(line,1,"=") == "allowed-ips") allow = deltok(line,1,"=");
		}
		rFile.close();
	}
	else
		cerr<<configFile<<" file missing. Using defaults."<<endl;
}

nigSock *serverInit(int &master_socket, int &addrlen, sockaddr_in &address, string bindIP, int bindPort, int max, int &err)
{
    err = 0;
    if((master_socket = socket(AF_INET,SOCK_STREAM,0)) == 0)
    {
        err = 1;
        return nullptr;
    }
    
    int opt = 1;
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        err = 2;
        return nullptr;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr( bindIP.c_str() );
    address.sin_port = htons( bindPort );
    
    if (::bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        err = 4;
        return nullptr;
    }
    
    if (listen(master_socket,3) < 0)
    {
        err = 3;
        return nullptr;
    }
    
    addrlen = sizeof(address);
    return new nigSock[max];
}

void sendToAllClients(nigSock *sockets, string buffer)
{
    for (int i = 0;i < MAXCLIENTS;i++)
        if (sockets[i].mark != "")
            sockets[i].nWrite(buffer);
}

void closeAllSockets(int &master_socket, nigSock *sockets)
{
    for (int i = 0; i < MAXCLIENTS; i++)
	{
		if (sockets[i].isOpen())
			sockets[i].nClose();
	}
	close(master_socket);
}

int prepSockets(fd_set &readfds, int &master_socket, nigSock *sockets)
{
    //clear the socket set
	FD_ZERO(&readfds);
	//add master socket to set
	FD_SET(master_socket, &readfds);
	int max_sd = master_socket;
	
	//add child sockets to set
	for (int i = 0;i < MAXCLIENTS;i++) 
	{
		//if valid socket descriptor then add to read list
		if(sockets[i].sock > 0)
			FD_SET( sockets[i].sock , &readfds);
		
		//highest file descriptor number, need it for the select function
		if(sockets[i].sock > max_sd)
			max_sd = sockets[i].sock;
	}
	return max_sd;
}

void acceptNewClient(sockaddr_in &address, int &addrlen, int &master_socket, nigSock *sockets, int &connectedClients, string &allow, string screen)
{
    int new_socket = 0;
    if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
	{
		cerr<<"accept failure"<<endl;
		return;
	}
	string ip = inet_ntoa(address.sin_addr);
	if (!istok(allow,ip," "))
	{
		close(new_socket);
		cout<<"Denied incoming connection from "<<ip<<". Not in the allowed-ips list."<<endl;
	}
	else
	{
	    int i;
        for (i = 0;i < MAXCLIENTS;i++)
        {
	        if (!sockets[i].isOpen())
	        {
		        sockets[i].assign(new_socket,address);
		        connectedClients++;
		        cout<<"New connection <"<<i<<"> established with "<<sockets[i].ip<<":"<<sockets[i].port<<endl;
	            cout<<"Total open connections: "<<connectedClients<<endl;
	            sockets[i].nWrite(string(VERSION) + "\t" + screen + "\t" + mcver + "\n");
		        break;
	        }
	        else if (i == MAXCLIENTS-1)
	        {
		        cerr<<"Too many open connections. . ."<<endl;
		        //send(new_socket,"Sorry, no room for you right now!\n",35,0);
		        close(new_socket);
	        }
        }
    }
}

void handleClients(fd_set &readfds, nigSock *sockets, int &connectedClients, string screen)
{
    static const string internals[] =
    {
        "relay",   // Relay a command back to the requesting mod as the server
        "relayto", // Relay a command back to the Nth mod as the server
        "relayall",// Relay a command back to all mods as the server
        "wall",    // Send a message to all mods, note: can be intercepted as a trigger by the mods
        "list",    // Replies to the requesting mod listing the currently connected mods
        "raw",     // Sends raw text to the requesting mod
        ""         // Replies to the requesting mod with version info
    };
    int valread;
    string temp, buffer;
    for (int i = 0;i < MAXCLIENTS;i++)
	{
		if ((sockets[i].sock == 0) || (!FD_ISSET(sockets[i].sock,&readfds)))
			continue;
		if ((valread = sockets[i].nRead(buffer)) == 0)
		{       //socket is closing
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
			    handleHandshake(sockets[i],buffer,screen);
				cout<<"Client <"<<i<<"> is identifying as: "<<sockets[i].mark<<endl;
			}
			else
			{
			    if (buffer.compare("hs") == 0)
		        {       // hs command with no params
                    commandVersion(sockets[i],screen);
	            }
	            else if (buffer.compare(0,3,"hs ") == 0)
	            {       // hs command with possible params
	                temp = buffer.substr(3,buffer.find(" ",3,1)-3);
	                int subcmd = 0;
	                for (;subcmd < MAX_INTERNAL;subcmd++)
	                    if (internals[subcmd] == temp)
	                        break;
                    switch (subcmd)
                    {
                        case INTERNAL_RELAY:
                        {
                            commandRelay(sockets[i],buffer);
                            break;
                        }
                        case INTERNAL_RELAYTO:
                        {
                            commandRelayTo(sockets[i],sockets,buffer);
                            break;
                        }
                        case INTERNAL_RELAYALL:
                        {
                            commandRelayAll(sockets[i],sockets,buffer);
                            break;
                        }
                        case INTERNAL_WALL:
                        {
                            commandWall(sockets[i],sockets,buffer);
                            break;
                        }
                        case INTERNAL_LIST:
                        {
                            commandList(sockets[i],sockets,connectedClients);
                            break;
                        }
                        case INTERNAL_RAW:
                        {
                            commandRaw(sockets[i],buffer);
                            break;
                        }
                        case INTERNAL_VER:
                        {
                            commandVersion(sockets[i],screen);
                            break;
                        }
                        default:
                        {
                            commandUnknown(sockets[i],buffer);
                        }
                    }
                }
			    else
			        sendToScreen(screen,buffer);
			}
		}
	}
}

void handleHandshake(nigSock &socket, string identity, string screen)
{
    socket.mark = identity;
	if (wasLateLoad())
	    sendList(screen);
}

void sendList(string screen)
{
    system(string("screen -S " + screen + " -p 0 -X stuff 'list\r'").c_str());
}

bool wasLateLoad()
{
    ifstream lateLoad("listo.nada");
    if (lateLoad.is_open())
    {
        lateLoad.close();
        remove("listo.nada");
        return true;
    }
    return false;
}

void commandVersion(nigSock &socket, string screen)
{
    socket.nWrite("[HS] " + string(VERSION) + " is running Minecraft version \"" + mcver + "\" on screen \"" + screen + "\".\n");
}

void commandRelay(nigSock &socket, string buffer)
{
    
    if (9 > buffer.size())
        socket.nWrite("[HS] Usage: hs relay <command ...>\n");
    else
        socket.nWrite("[99:99:99] [Server thread/INFO]: <#SERVER> !rcon " + buffer.substr(9,string::npos) + "\n");
}

void commandRelayTo(nigSock &socket, nigSock *sockets, string buffer)
{
    if (11 > buffer.size())
    {
        socket.nWrite("[HS] Usage: hs relayto <N> <command ...>\n");
        return;
    }
    string temp = buffer.substr(11,buffer.find(" ",11,1)-11);
    if ((!stringisnum(temp)) || ((12 + temp.size()) > buffer.size()))
        socket.nWrite("[HS] Usage: hs relayto <N> <command ...>\n");
    else
    {
        int sockTo = stoi(temp);
        if ((sockTo < 1) || (sockTo > MAXCLIENTS) || (sockets[sockTo].mark == ""))
            socket.nWrite("[HS] Invalid mod: " + temp + "\n");
        else
        {
            temp = "[99:99:99] [Server thread/INFO]: <#SERVER> !rcon " + buffer.substr(12 + temp.size(),string::npos) + "\n";
            sockets[sockTo].nWrite(temp);
        }
    }
}

void commandRelayAll(nigSock &socket, nigSock *sockets, string buffer)
{
    if (12 > buffer.size())
    {
        socket.nWrite("[HS] Usage: hs relayall <command ...>\n");
        return;
    }
    else
        sendToAllClients(sockets,"[99:99:99] [Server thread/INFO]: <#SERVER> !rcon " + buffer.substr(12,string::npos) + "\n");
}

void commandWall(nigSock &socket, nigSock *sockets, string buffer)
{
    if (8 > buffer.size())
        socket.nWrite("[HS] Usage: hs wall <message ...>\n");
    else
        sendToAllClients(sockets,"[HS] WALL <" + socket.mark + "> " + buffer.substr(8,string::npos) + "\n");
}

void commandList(nigSock &socket, nigSock *sockets, int connectedClients)
{
    string temp = "[HS] All currently connected mods (" + to_string(connectedClients) + "/" + to_string(MAXCLIENTS) + "):\n";
    for (int i = 0;i < MAXCLIENTS;i++)
        if (sockets[i].mark != "")
            temp = temp + "[HS] " + to_string(i) + ":\t" + sockets[i].mark + "\n";
    socket.nWrite(temp);
}

void commandRaw(nigSock &socket, string buffer)
{
    socket.nWrite(buffer.substr(7,string::npos) + "\n");
}

void commandUnknown(nigSock &socket, string buffer)
{
    socket.nWrite("[HS] Unknown command sequence: " + buffer + "\n");
}

void sendToScreen(string screen, string buffer)
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
    int valread = buffer.size();
    string temp;
    for (int i = 0;i < valread;i += MAXSCREENLEN)
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

