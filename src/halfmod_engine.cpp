#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <netdb.h>
#include <unistd.h>
#include "str_tok.h"
#include "halfmod.h"
#include "halfmod_func.h"
#include ".hmEngineBuild.h"
using namespace std;

vector<hmConsoleFilter> filters;

int main(int argc, char *argv[])
{
	int port, sockfd;
	string host[2];
	struct sockaddr_in serv_addr;
	struct hostent *server;
	fd_set readfds;
	struct timeval timeout;
	int activity;
	string line;
	int valread;
	bool connected;
	hmGlobal serverInfo;
	serverInfo.conFilter = &filters;
	recallGlobal(&serverInfo);
	serverInfo.hmVer = VERSION;
	
	if (argc > 1)
	{
		int pos = 1;
		for (;pos < argc;pos++)
		{
			if (string(argv[pos]).compare(0,2,"--") == 0)
			{
				if (argv[pos] == "--")
				{
					pos++;
					break;
				}
				else if (strcmp(argv[pos],"--version") == 0)
				{
					cout<<VERSION<<" built with halfMod API "<<API_VERSION<<" written by nigel."<<endl;
					return 0;
				}
				else if (strcmp(argv[pos],"--verbose") == 0)
					serverInfo.verbose = true;
				else if (strcmp(argv[pos],"--quiet") == 0)
					serverInfo.quiet = true;
				else if (strcmp(argv[pos],"--debug") == 0)
					serverInfo.debug = true;
				//else if (iswm(argv[pos],"--mc-version=*"))
				else if (string(argv[pos]).compare(2,11,"mc-version=") == 0)
					serverInfo.mcVer = deltok(argv[pos],1,"=");
				//else if (iswm(argv[pos],"--mc-world=*"))
				else if (string(argv[pos]).compare(2,9,"mc-world=") == 0)
					serverInfo.world = deltok(argv[pos],1,"=");
				else if (strcmp(argv[pos],"--log-bans") == 0)
					serverInfo.logMethod |= LOG_BAN;
				else if (strcmp(argv[pos],"--log-kicks") == 0)
					serverInfo.logMethod |= LOG_KICK;
				else if (strcmp(argv[pos],"--log-op") == 0)
					serverInfo.logMethod |= LOG_OP;
				else if (strcmp(argv[pos],"--log-whitelist") == 0)
					serverInfo.logMethod |= LOG_WHITELIST;
			}
			else
				break;
		}
		if (argc-pos < 2)
		{
			ifstream rFile(LASTRESORT);
			if (rFile.is_open())
			{
				while (getline(rFile,line))
				{
					if (iswm(nospace(line),"#*")) continue;
					line = lower(line);
					if (gettok(line,1,"=") == "ip") host[0] = gettok(line,2,"=");
					if (gettok(line,1,"=") == "port") host[1] = stoi(gettok(line,2,"="));
				}
				line = "";
				rFile.close();
				if ((host[0] == "") || (host[1] == ""))
				{
					cerr<<"No input provided and fallback \""<<LASTRESORT<<"\" file is incomplete . . ."<<endl;
					return 1;
				}
			}
			else
			{
				cerr<<"No input provided and no fallback \""<<LASTRESORT<<"\" file . . ."<<endl;
				return 1;
			}
		}
		else
		{
			host[0] = argv[pos];
			host[1] = argv[pos+1];
		}
	}
	
	mkdirIf("./halfMod/");
	mkdirIf("./halfMod/plugins/");
	mkdirIf("./halfMod/config/");
	mkdirIf("./halfMod/userdata/");
	mkdirIf("./halfMod/logs/");
	
	vector<string> pluginPaths;
	vector<hmHandle> plugins;
	hashAdmins(serverInfo,ADMINCONF);
	findPlugins("./halfMod/extensions/",pluginPaths);
	for (int i = 0, j = pluginPaths.size();i < j;i++)
	    loadExtension(serverInfo,pluginPaths[i]);
    pluginPaths.clear();
//	plugins.resize(findPlugins("./halfMod/plugins/",pluginPaths));
	findPlugins("./halfMod/plugins/",pluginPaths);
	for (int i = 0, j = pluginPaths.size();i < j;i++)
		loadPlugin(serverInfo,plugins,pluginPaths[i]);
	hashConsoleFilters(plugins,CONFILTERPATH);
	// eventually get around to using fstream for this... lol
	system("echo todo>listo.nada");
	regex hsPtrn ("^(.*?)\t(.*?)\t(.*)$");
	smatch hsMl;
	while (1)
	{
		port = stoi(host[1]);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		while (sockfd < 0)
		{
			cerr<<"Error opening socket . . . Retrying in 5 seconds . . ."<<endl;
			sleep(5);
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
		}
		server = gethostbyname(host[0].c_str());
		if (server == NULL)
		{
			cerr<<"Error, no such host "<<argv[1]<<" . . .";
			if (host[0] != "127.0.0.1")
			{
				cerr<<" Trying 127.0.0.1 . . .";
				if ((server = gethostbyname("127.0.0.1")) == NULL)
				{
					cerr<<" Something is very wrong . . .\nTry recompiling?"<<endl;
					return 1;
				}
			}
			cerr<<endl;
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, 
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);
		serv_addr.sin_port = htons(port);
		serverInfo.hsSocket = -1;
		serverInfo.mcScreen.clear();
		while (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
		{
			cerr<<"No connection to halfShell . . . Retrying in 1 second . . ."<<endl;
			sleep(1);
		}
		serverInfo.hsSocket = sockfd;
		line = VERSION;
		line += "\r";
		send(sockfd,line.c_str(),line.size(),0); // send handshake
		line.clear();
		connected = false;
		//send(sockfd,"list\r",5,0); // request player list upon connecting
		while (1)
		{
			//clear the socket set
			FD_ZERO(&readfds);
			//add socket to set
			FD_SET(sockfd, &readfds);
			
			FD_SET(0, &readfds);
			timeout.tv_sec = 0;
			timeout.tv_usec = 10;
			activity = select( sockfd + 1 , &readfds , NULL , NULL , &timeout);
			if ((activity < 0) && (errno!=EINTR)) 
			{
				cerr<<"Select error.\n";
			}
			else if ((activity == 0) && (errno!=EINTR))
			{
				processTimers(plugins);
			}
			else
			{
				if (FD_ISSET(0, &readfds))
				{
					// input from stdin
					getline(cin,line);
					if (line != "")
					{
						processCmd(serverInfo,plugins,gettok(line,1," "),"#SERVER",deltok(line,1," "),false,true);
//						cout<<"stdin: "<<line<<endl;
						line.clear();
					}
				}
				if (FD_ISSET(sockfd, &readfds))
				{
					if ((valread = readSock(sockfd,line)) == 0)
					{
						// host disconnected
						close(sockfd);
						processEvent(plugins,HM_ONHSDISCONNECT);
						sockfd = 0;
						cerr<<"Disconnected from host . . . Attempting to reconnect . . ."<<endl;
						break;
					}
					// input from minecraft
					if (!connected)
					{	// receive handshake
					    regex_match(line,hsMl,hsPtrn);
						serverInfo.hsVer = hsMl[1];
						serverInfo.mcScreen = hsMl[2];
						if (serverInfo.mcVer == "")
						    serverInfo.mcVer = hsMl[3];
						cout<<"Link established with "<<serverInfo.hsVer<<endl;
						connected = true;
						processEvent(plugins,HM_ONHSCONNECT,hsMl);
					}
					else
						processThread(serverInfo,plugins,line);
					line.clear();
				}
				processTimers(plugins);
			}
		}
	}
	return 0;
}





