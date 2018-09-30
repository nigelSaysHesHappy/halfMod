/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
!-----------------------N--O--T--I--C--E-----------------------!
*                                                              *
*  Any modifications to this file should be made with extreme  *
*   care. This file is loaded directly into the Java Runtime   *
*    Evnvironment. If proper precautions are not taken, the    *
*      server can easily be slowed or internally damaged.      *
*                                                              *
*   halfShell, in its original state, is perfectly safe and    *
*   should cause no noticible or unwanted performance loss.    *
*                                                              *
*                       Written by nigel                       *
*             https://github.com/nigelSaysHesHappy             *
*         https://github.com/nigelSaysHesHappy/halfMod         *
*                                                              *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "nigsock.h"

#define HS_VERSION "halfShell v0.4.0"

struct hs_global
{
    std::vector<std::string> allow;
    nigSock *sockets;
    const int MAXCLIENTS = 10;
    int master_socket;
    int connectedClients = 0;
    int addrlen;
    sockaddr_in address;
    fd_set readfds;
    std::string world;
    std::string mcver;
};

static ssize_t (*original_write)(int fildes, const void *buf, size_t nbyte) = nullptr;
ssize_t (*original_read)(int fd, void *buf, size_t count) = nullptr;
static void (*original_exit)(int status) = nullptr;
static std::string hs_lower(std::string text);
static int hs_commandOverride(const std::string &in, nigSock *sock);
static void hs_setup(std::string configFile, std::string ip, int port);
static int hs_serverInit(std::string &bindIP, int &bindPort);
static void hs_prepSockets(int &max_sd);
static void hs_acceptNewClient();
static void hs_sendToAllClients(std::string buffer);
static void hs_closeAllSockets();
static ssize_t hs_handleClients(char *buf);
static int hs_commandVersion(nigSock *socket);
static int hs_commandRelay(nigSock *socket, const std::string &buffer);
static int hs_commandRelayTo(nigSock *socket, const std::string &buffer);
static int hs_commandRelayAll(nigSock *socket, const std::string &buffer);
static int hs_commandWall(nigSock *socket, std::string buffer);
static int hs_commandList(nigSock *socket);
static int hs_commandRaw(nigSock *socket, const std::string &buffer);
static int hs_commandUnknown(nigSock *socket, const std::string &buffer);
static int hs_commandQuit(nigSock *socket, std::string buffer);
static int hs_commandKick(nigSock *socket, std::string buffer);
static int hs_commandHelp(nigSock *socket);

static hs_global hs_data;

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
    static bool found_ver = false, found_world = false;
    if (original_write == nullptr)
        original_write = reinterpret_cast<ssize_t (*)(int,const void*,size_t)>(dlsym(RTLD_NEXT,"write"));
    int ret = original_write(fildes,buf,nbyte);
    if (fildes == 1)
    {
        std::string str = (char*)buf;
        if (str.size() > ret)
            str.erase(ret);
        if (!found_world)
        {
            if ((!found_ver) && (str.compare(11,56,"[Server thread/INFO]: Starting minecraft server version ") == 0))
            {
                hs_data.mcver = str.substr(67);
                hs_data.mcver.erase(hs_data.mcver.find('\n'));
                found_ver = true;
            }
            else if ((found_ver) && (str.compare(11,38,"[Server thread/INFO]: Preparing level ") == 0))
            {
                hs_data.world = str.substr(49);
                hs_data.world.erase(hs_data.world.find('\n'));
                found_world = true;
            }
        }
        hs_sendToAllClients(str);
    }
    return ret;
}

ssize_t read(int fd, void *buf, size_t count)
{
    if (original_read == nullptr)
    {
        original_read = reinterpret_cast<ssize_t (*)(int,void*,size_t)>(dlsym(RTLD_NEXT,"read"));
        hs_setup("halfshell.conf","127.0.0.1",9422); // these defaults can be changed in halfshell.conf
    }
    if (fd != 0)
        return original_read(fd,buf,count);
    static int max_sd, activity;
    ssize_t ret = -1;
    while (true)
    {
        hs_prepSockets(max_sd);
        activity = select(max_sd+1,&hs_data.readfds,NULL,NULL,NULL);
        if (activity > 0)
        {
            if (FD_ISSET(0,&hs_data.readfds))
            {
                ret = original_read(fd,buf,count);
                std::string str = (char*)buf;
                if (str.size() > ret)
                    str.erase(ret);
                str = str.substr(0,str.find('\n'));
                if (!hs_commandOverride(str,nullptr))
                    break;
                activity--;
            }
            if (FD_ISSET(hs_data.master_socket,&hs_data.readfds))
            {
                hs_acceptNewClient();
                activity--;
            }
            if (activity > 0)
            {
                ret = hs_handleClients((char*)buf);
                if (ret > 0)
                    break;
            }
        }
    }
    return ret;
}

void exit(int status)
{
    if (original_exit == nullptr)
        original_exit = reinterpret_cast<void (*)(int)>(dlsym(RTLD_NEXT,"exit"));
    hs_sendToAllClients("[HS] Server closed with exit status: " + std::to_string(status) + '\n');
    hs_closeAllSockets();
    original_exit(status);
    throw "lol noreturn function returns";
}

std::string hs_lower(std::string text)
{
    if (text.size() > 0)
        for (std::string::iterator it = text.begin(), ite = text.end();it != ite;++it)
            *it = tolower(*it);
    return text;
}

int hs_commandOverride(const std::string &in, nigSock *sock)
{
    static const std::string internals[] =
    {
        "relay",   // Relay a command back to the requesting mod as the server
        "relayto", // Relay a command back to the Nth mod as the server
        "relayall",// Relay a command back to all mods as the server
        "wall",    // Send a message to all mods, note: can be intercepted as a trigger by the mods
        "list",    // Replies to the requesting mod listing the currently connected mods
        "raw",     // Sends raw text to the requesting mod
        "quit",    // Disconnects the requesting client
        "kick",    // Allows server to disconnect a client
        "help",    // Displays help text
        "?",       // Displays help text
        ""         // Replies to the requesting mod with version info
    };
    static const int MAX_INTERNAL = 11;
    std::string cmd = in.substr(0,in.find(' '));
    std::string args = in.substr(((cmd.size() < in.size()) ? cmd.size()+1 : cmd.size()));
    if (cmd == "echo")
    {
        std::cout<<args<<std::endl;
        return 1;
    }
    if (cmd == "hs")
    {
        if (args.size() > 0)
        {
            cmd = args.substr(0,args.find(' '));
            args = args.substr(((cmd.size() < args.size()) ? cmd.size()+1 : cmd.size()));
        }
        else
        {
            cmd = args;
            args.clear();
        }
        int subcmd = 0;
        for (;subcmd < MAX_INTERNAL;subcmd++)
            if (internals[subcmd] == cmd)
                break;
        switch (subcmd)
        {
            case 0:
                return hs_commandRelay(sock,in);
            case 1:
                return hs_commandRelayTo(sock,in);
            case 2:
                return hs_commandRelayAll(sock,in);
            case 3:
                return hs_commandWall(sock,in);
            case 4:
                return hs_commandList(sock);
            case 5:
                return hs_commandRaw(sock,in);
            case 6:
                return hs_commandQuit(sock,in);
            case 7:
                return hs_commandKick(sock,in);
            case 8:
            case 9:
                return hs_commandHelp(sock);
            case 10:
                return hs_commandVersion(sock);
            default:
                return hs_commandUnknown(sock,in);
        }
    }
    return 0;
}

void hs_setup(std::string configFile, std::string ip, int port)
{
    hs_data.allow.push_back("127.0.0.1");
    std::ifstream rFile(configFile);
    std::string line;
	if (rFile.is_open())
	{
		while (std::getline(rFile,line))
		{
		    if ((line.size() > 0) && (line.at(0) != '#'))
		    {
		        line = hs_lower(line);
		        int pos = line.find('=');
		        std::string item = line.substr(0,pos);
		        if (item == "ip")
		            ip = line.substr(pos+1);
	            if (item == "port")
	                port = std::stoi(line.substr(pos+1));
                if (item == "allowed-ip")
                    hs_data.allow.push_back(line.substr(pos+1));
	        }
		}
		rFile.close();
	}
	else
		std::cerr<<configFile<<" file missing. Using defaults."<<std::endl;
	switch (hs_serverInit(ip,port))
	{
	    case 0:
        {
            std::cout<<"Listening for connections on "<<ip<<':'<<port<<" . . ."<<std::endl;
            break;
        }
	    case 1:
	        std::cerr<<"Failure creating master socket . . ."<<std::endl;
        case 2:
            std::cerr<<"Failure enabling simultaneous socket connections . . ."<<std::endl;
        case 3:
            std::cerr<<"Unable to begin listening for incoming connections . . ."<<std::endl;
        case 4:
            std::cerr<<"Unable to bind listening server to "<<ip<<':'<<port<<" . . ."<<std::endl;
        std::exit(1);
        default:
            std::cerr<<"An unknown error has occurred . . ."<<std::endl;
        std::exit(1);
    }
}

int hs_serverInit(std::string &bindIP, int &bindPort)
{
    if ((hs_data.master_socket = socket(AF_INET,SOCK_STREAM,0)) == 0)
        return 1;
    int opt = 1;
    if (setsockopt(hs_data.master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
        return 2;
    hs_data.address.sin_family = AF_INET;
    hs_data.address.sin_addr.s_addr = inet_addr(bindIP.c_str());
    hs_data.address.sin_port = htons(bindPort);
    if (bind(hs_data.master_socket, (struct sockaddr *)&hs_data.address, sizeof(hs_data.address)) < 0) 
        return 4;
    if (listen(hs_data.master_socket,3) < 0)
        return 3;
    hs_data.addrlen = sizeof(hs_data.address);
    hs_data.sockets = new nigSock[hs_data.MAXCLIENTS];
    return 0;
}

void hs_prepSockets(int &max_sd)
{
	FD_ZERO(&hs_data.readfds);
	FD_SET(0,&hs_data.readfds);
	FD_SET(hs_data.master_socket, &hs_data.readfds);
	max_sd = hs_data.master_socket;
	for (int i = 0;i < hs_data.MAXCLIENTS;i++) 
	{
		if (hs_data.sockets[i].sock > 0)
			FD_SET(hs_data.sockets[i].sock,&hs_data.readfds);
		if (hs_data.sockets[i].sock > max_sd)
			max_sd = hs_data.sockets[i].sock;
	}
}

void hs_acceptNewClient()
{
    ssize_t r = -1;
    int new_socket = 0;
    if ((new_socket = accept(hs_data.master_socket, (struct sockaddr *)&hs_data.address, (socklen_t*)&hs_data.addrlen)) > 0)
	{
	    std::string ip = inet_ntoa(hs_data.address.sin_addr);
	    bool allowed = false;
	    for (auto it = hs_data.allow.begin(), ite = hs_data.allow.end();it != ite;++it)
	    {
	        if (ip == *it)
	        {
                allowed = true;
                break;
            }
        }
        if (!allowed)
        {
            close(new_socket);
		    std::cout<<"Denied incoming connection from "<<ip<<". Not in the allowed-ips list."<<std::endl;
	    }
	    else
	    {
            for (int i = 0;i < hs_data.MAXCLIENTS;i++)
            {
	            if (!hs_data.sockets[i].isOpen())
	            {
		            hs_data.sockets[i].assign(new_socket,hs_data.address);
		            hs_data.connectedClients++;
		            std::cout<<"New connection <"<<i<<"> established with "<<hs_data.sockets[i].ip<<':'<<hs_data.sockets[i].port<<std::endl;
	                std::cout<<"Total open connections: "<<hs_data.connectedClients<<std::endl;
	                hs_data.sockets[i].nWrite(std::string(HS_VERSION) + '\t' + hs_data.mcver + '\t' + hs_data.world + '\n');
	                break;
	            }
	            else if (i == hs_data.MAXCLIENTS-1)
	            {
		            std::cerr<<"Too many open connections. . ."<<std::endl;
		            close(new_socket);
	            }
            }
        }
    }
}

void hs_sendToAllClients(std::string buffer)
{
    for (int i = 0;i < hs_data.MAXCLIENTS;i++)
        if (hs_data.sockets[i].mark.size() > 0)
            hs_data.sockets[i].nWrite(buffer);
}

void hs_closeAllSockets()
{
    for (int i = 0; i < hs_data.MAXCLIENTS; i++)
	{
		if (hs_data.sockets[i].isOpen())
			hs_data.sockets[i].nClose();
	}
	close(hs_data.master_socket);
}

ssize_t hs_handleClients(char *buf)
{
    ssize_t r = -1;
    int valread;
    std::string buffer;
    for (int i = 0;i < hs_data.MAXCLIENTS;i++)
	{
		if ((hs_data.sockets[i].sock == 0) || (!FD_ISSET(hs_data.sockets[i].sock,&hs_data.readfds)))
			continue;
		if ((valread = hs_data.sockets[i].nRead(buffer)) == 0)
		{       //socket is closing
			if (hs_data.sockets[i].mark.size() > 0)
				std::cout<<"Host disconnect <"<<hs_data.sockets[i].mark<<">: "<<hs_data.sockets[i].ip<<':'<<hs_data.sockets[i].port<<" Broken Pipe"<<std::endl;
			else
				std::cout<<"Host disconnect <"<<i<<">: "<<hs_data.sockets[i].ip<<':'<<hs_data.sockets[i].port<<" Broken Pipe"<<std::endl;
			hs_data.sockets[i].nClose();
			hs_data.connectedClients--;
			std::cout<<"Total open connections: "<<hs_data.connectedClients<<std::endl;
		}
		else if (valread > 0)// read
		{
			if (hs_data.sockets[i].mark.size() < 1)
			{		// handshake
			    hs_data.sockets[i].mark = buffer;
				std::cout<<"Client <"<<i<<"> is identifying as: "<<hs_data.sockets[i].mark<<std::endl;
				if (hs_data.world.size() > 0)
                {
	                strncpy(buf,"list\n",5);
                    r = 5;
                    break;
                }
			}
			else if (!hs_commandOverride(buffer,&hs_data.sockets[i]))
	        {
    	        buffer += '\n';
                strncpy(buf,buffer.c_str(),buffer.size());
	            r = buffer.size();
	            break;
            }
		}
	}
	return r;
}

int hs_commandVersion(nigSock *socket)
{
    if (socket == nullptr)
        std::cout<<"[HS] "<<HS_VERSION<<" is running Minecraft version \""<<hs_data.mcver<<"\"."<<std::endl;
    else
        socket->nWrite("[HS] " + std::string(HS_VERSION) + " is running Minecraft version \"" + hs_data.mcver + "\".\n");
    return 1;
}

int hs_commandRelay(nigSock *socket, const std::string &buffer)
{
    if (buffer.size() < 10)
    {
        if (socket == nullptr)
            std::cout<<"[HS] Usage: hs relay <command ...>"<<std::endl;
        else
            socket->nWrite("[HS] Usage: hs relay <command ...>\n");
    }
    else
    {
        if (socket == nullptr)
            std::cout<<"[HS] hs relay can only be used from a mod."<<std::endl;
        else
            socket->nWrite("[99:99:99] [Server thread/INFO]: <#SERVER> !rcon " + buffer.substr(9) + '\n');
    }
    return 1;
}

int hs_commandRelayTo(nigSock *socket, const std::string &buffer)
{
    if (buffer.size() < 13)
    {
        if (socket == nullptr)
            std::cout<<"[HS] Usage: hs relayto <N> <command ...>"<<std::endl;
        else
            socket->nWrite("[HS] Usage: hs relayto <N> <command ...>\n");
    }
    else
    {
        std::string temp = buffer.substr(11,buffer.find(' ',11)-11);
        if ((!isdigit(temp[0])) || ((13 + temp.size()) > buffer.size()))
        {
            if (socket == nullptr)
                std::cout<<"[HS] Usage: hs relayto <N> <command ...>"<<std::endl;
            else
                socket->nWrite("[HS] Usage: hs relayto <N> <command ...>\n");
        }
        else
        {
            int sockTo = std::stoi(temp);
            if ((sockTo < 0) || (sockTo > hs_data.MAXCLIENTS) || (hs_data.sockets[sockTo].mark.size() < 1))
            {
                if (socket == nullptr)
                    std::cout<<"[HS] Invalid mod: "<<temp<<std::endl;
                else
                    socket->nWrite("[HS] Invalid mod: " + temp + '\n');
            }
            else
            {
                temp = "[99:99:99] [Server thread/INFO]: <#SERVER> !rcon " + buffer.substr(12 + temp.size()) + '\n';
                hs_data.sockets[sockTo].nWrite(temp);
            }
        }
    }
    return 1;
}

int hs_commandRelayAll(nigSock *socket, const std::string &buffer)
{
    if (buffer.size() < 13)
    {
        if (socket == nullptr)
            std::cout<<"[HS] Usage: hs relayall <command ...>"<<std::endl;
        else
            socket->nWrite("[HS] Usage: hs relayall <command ...>\n");
    }
    else
        hs_sendToAllClients("[99:99:99] [Server thread/INFO]: <#SERVER> !rcon " + buffer.substr(12) + '\n');
    return 1;
}

int hs_commandWall(nigSock *socket, std::string buffer)
{
    if (buffer.size() < 9)
    {
        if (socket == nullptr)
            std::cout<<"[HS] Usage: hs wall <message ...>"<<std::endl;
        else
            socket->nWrite("[HS] Usage: hs wall <message ...>\n");
    }
    else
    {
        buffer = buffer.substr(8);
        std::string client = "#SERVER";
        if (socket != nullptr)
            client = socket->mark;
        std::cout<<"[HS] WALL <"<<client<<"> "<<buffer<<std::endl;
        hs_sendToAllClients("[HS] WALL <" + client + "> " + buffer + '\n');
    }
    return 1;
}

int hs_commandList(nigSock *socket)
{
    std::string temp = "[HS] All currently connected mods (" + std::to_string(hs_data.connectedClients) + '/' + std::to_string(hs_data.MAXCLIENTS) + "):\n";
    for (int i = 0;i < hs_data.MAXCLIENTS;i++)
        if (hs_data.sockets[i].mark.size() > 0)
            temp = temp + "[HS] " + std::to_string(i) + ":\t" + hs_data.sockets[i].mark + '\n';
    if (socket == nullptr)
        std::cout<<temp.erase(temp.size()-1)<<std::endl;
    else
        socket->nWrite(temp);
    return 1;
}

int hs_commandRaw(nigSock *socket, const std::string &buffer)
{
    if (socket == nullptr)
        std::cout<<"[HS] hs raw can only be used from a mod."<<std::endl;
    else if (buffer.size() > 7)
        socket->nWrite(buffer.substr(7) + '\n');
    else
        socket->nWrite("[HS] Usage: hs raw <raw ...>\n");
    return 1;
}

int hs_commandUnknown(nigSock *socket, const std::string &buffer)
{
    if (socket == nullptr)
        std::cout<<"[HS] Unknown command sequence: "<<buffer<<std::endl;
    else
        socket->nWrite("[HS] Unknown command sequence: " + buffer + '\n');
    return 1;
}

int hs_commandQuit(nigSock *socket, std::string buffer)
{
    if (socket == nullptr)
        std::cout<<"[HS] hs quit can only be used from a mod."<<std::endl;
    else
    {
        if (buffer.size() < 9)
            buffer = "Quit";
        else
            buffer = buffer.substr(8);
        std::cout<<"Host disconnect <"<<socket->mark<<">: "<<socket->ip<<':'<<socket->port<<" ("<<buffer<<')'<<std::endl;
        socket->nClose();
        hs_data.connectedClients--;
        std::cout<<"Total open connections: "<<hs_data.connectedClients<<std::endl;
    }
    return 1;
}

int hs_commandKick(nigSock *socket, std::string buffer)
{
    if (socket != nullptr)
        socket->nWrite("[HS] Unknown command sequence: " + buffer + '\n');
    else if (buffer.size() < 9)
        std::cout<<"[HS] Usage: hs kick <N> [reason ...]"<<std::endl;
    else
    {
        size_t pos = buffer.find(' ',8);
        std::string temp = buffer.substr(8,pos-8);
        if (!isdigit(temp[0]))
            std::cout<<"[HS] Usage: hs kick <N> [reason ...]"<<std::endl;
        else
        {
            int sockTo = std::stoi(temp);
            if ((sockTo < 0) || (sockTo > hs_data.MAXCLIENTS) || (hs_data.sockets[sockTo].mark.size() < 1))
                std::cout<<"[HS] Invalid mod: "<<temp<<std::endl;
            else
            {
                if (((int)buffer.size()-9) > temp.size())
                    buffer = buffer.substr(pos+1);
                else
                    buffer = "Kicked";
                std::cout<<"Client kicked <"<<hs_data.sockets[sockTo].mark<<">: "<<hs_data.sockets[sockTo].ip<<':'<<hs_data.sockets[sockTo].port<<" ("<<buffer<<")"<<std::endl;
                hs_data.sockets[sockTo].nWrite("[HS] You have been kicked from the halfShell server (" + buffer + ")\n");
                hs_data.sockets[sockTo].nClose();
                hs_data.connectedClients--;
                std::cout<<"Total open connections: "<<hs_data.connectedClients<<std::endl;
            }
        }
    }
    return 1;
}

int hs_commandHelp(nigSock *socket)
{
    std::string out = "[HS] hs - Display version info.\n[HS] hs help|? - Displays this help message.\n[HS] hs kick <N> [reason ...] - Kick the Nth connected client.\n[HS] hs list - List all connected clients.\n[HS] hs quit [message ...] - Disconnect from halfShell.\n[HS] hs raw <raw ...> - Sends raw text to the requesting mod.\n[HS] hs relay <command ...> - Relay a command back to the requesting mod as the server.\n[HS] hs relayall <command ...> - Relay a command back to all mods as the server.\n[HS] hs relayto <N> <command ...> - Relay a command back to the Nth mod as the server.\n[HS] hs wall <message ...> - Send a message to all mods, note: can be intercepted as a trigger by the mods.";
    if (socket == nullptr)
        std::cout<<out<<std::endl;
    else
        socket->nWrite(out + '\n');
    return 1;
}

