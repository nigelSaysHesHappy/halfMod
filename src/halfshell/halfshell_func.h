#ifndef HALFSHELL
#define HALFSHELL

#define TRUE		1
#define FALSE		0
#define DEFPORT		9422
#define LOCALHOST	"127.0.0.1"
#define CONF		"halfshell.conf"
#define MAXCLIENTS	30
#define MAXSCREENLEN	400

#define MAX_INTERNAL        7
#define INTERNAL_RELAY      0
#define INTERNAL_RELAYTO    1
#define INTERNAL_RELAYALL   2
#define INTERNAL_WALL       3
#define INTERNAL_LIST       4
#define INTERNAL_RAW        5
#define INTERNAL_VER        6

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

extern std::string mcver;

void loadConfig(std::string configFile, std::string &ip, int &port, std::string &allow);
nigSock *serverInit(int &master_socket, int &addrlen, sockaddr_in &address, std::string bindIP, int bindPort, int max, int &err);
void sendToAllClients(nigSock *sockets, std::string buffer);
void closeAllSockets(int &master_socket, nigSock *sockets);
int prepSockets(fd_set &readfds, int &master_socket, nigSock *sockets);
void acceptNewClient(sockaddr_in &address, int &addrlen, int &master_socket, nigSock *sockets, int &connectedClients, std::string &allow, std::string screen);
void handleClients(fd_set &readfds, nigSock *sockets, int &connectedClients, std::string screen);
void handleHandshake(nigSock &socket, std::string identity, std::string screen);
void sendList(std::string screen);
bool wasLateLoad();
void commandVersion(nigSock &socket, std::string screen);
void commandRelay(nigSock &socket, std::string buffer);
void commandRelayTo(nigSock &socket, nigSock *sockets, std::string buffer);
void commandRelayAll(nigSock &socket, nigSock *sockets, std::string buffer);
void commandWall(nigSock &socket, nigSock *sockets, std::string buffer);
void commandList(nigSock &socket, nigSock *sockets, int connectedClients);
void commandRaw(nigSock &socket, std::string buffer);
void commandUnknown(nigSock &socket, std::string buffer);
void sendToScreen(std::string screen, std::string buffer);

#endif

