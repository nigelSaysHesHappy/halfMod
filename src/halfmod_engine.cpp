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

int main(int argc, char *argv[])
{
    hmGlobal serverInfo;
    recallGlobal(&serverInfo);
    serverInfo.hmVer = VERSION;
    serverInfo.logMethod = LOG_BAN|LOG_KICK|LOG_OP|LOG_WHITELIST;
    
    int pos = handleCommandLineSwitches(serverInfo,argc,argv);
    if (pos < 1)
        return pos;
    
    string line, host[2];
    if (argc-pos < 2)
        loadConfig(host);
    for (int i = 0;argc-pos > 0;i++,pos++)
    {
        host[i] = argv[pos];
        if (i > 0)
            break;
    }
    if ((host[0] == "") || (!stringisnum(host[1],0)))
    {
        cerr<<"No input provided and fallback \""<<LASTRESORT<<"\" file is missing or corrupt . . ."<<endl;
        return 1;
    }
    int port = stoi(host[1]);
    hostent *server = resolveHost(host[0]);
    if (server == NULL)
    {
        cerr<<"Something is very wrong . . .\nTry recompiling?"<<endl;
        return 1;
    }
    
    vector<hmHandle> plugins;
    vector<hmConsoleFilter> filters;
    serverInfo.conFilter = &filters;
    loadAssets(serverInfo,plugins,filters);
    
    fd_set readfds;
    int valread, sockfd = 0;
    while (1)
    {
        if (handleConnection(sockfd,readfds,serverInfo,plugins,server,port) > 0)
        {
            if (FD_ISSET(0,&readfds))
            {       // input from stdin
                getline(cin,line);
                if (line.size() > 0)
                    processCmd(serverInfo,plugins,gettok(line,1," "),"#SERVER",deltok(line,1," "),false,true);
            }
            if (FD_ISSET(sockfd,&readfds))
            {       //input from minecraft
                if ((valread = readSock(sockfd,line)) == 0)
                    handleDisconnect(sockfd,plugins);
                else
                    processThread(serverInfo,plugins,line);
            }
        }
    }
    return 0;
}

