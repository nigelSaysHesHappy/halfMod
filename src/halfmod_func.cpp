#include <iostream>
#include <fstream>
#include <ctime>
#include <dirent.h>
#include <netdb.h>
#include <unistd.h>
#include "str_tok.h"
#include "halfmod.h"

#ifndef HM_USE_PCRE2
#include <array>
#endif

#include "halfmod_func.h"
#include ".hmEngineBuild.h"
using namespace std;

int handleCommandLineSwitches(hmGlobal &serverInfo, int argc, char *argv[])
{
    int pos = 1;
    if (argc > 1)
    {
        for (;pos < argc;pos++)
        {
            if (string(argv[pos]).compare(0,2,"--") == 0)
            {
                if (strcmp(argv[pos],"--") == 0)
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
                else if (strcmp(argv[pos],"--log-off") == 0)
                    serverInfo.logMethod = 0;
                else if (strcmp(argv[pos],"--log-bans-off") == 0)
                    serverInfo.logMethod &= ~LOG_BAN;
                else if (strcmp(argv[pos],"--log-kicks-off") == 0)
                    serverInfo.logMethod &= ~LOG_KICK;
                else if (strcmp(argv[pos],"--log-op-off") == 0)
                    serverInfo.logMethod &= ~LOG_OP;
                else if (strcmp(argv[pos],"--log-whitelist-off") == 0)
                    serverInfo.logMethod &= ~LOG_WHITELIST;
                else if (strcmp(argv[pos],"--log-bans") == 0)
                    serverInfo.logMethod |= LOG_BAN;
                else if (strcmp(argv[pos],"--log-kicks") == 0)
                    serverInfo.logMethod |= LOG_KICK;
                else if (strcmp(argv[pos],"--log-op") == 0)
                    serverInfo.logMethod |= LOG_OP;
                else if (strcmp(argv[pos],"--log-whitelist") == 0)
                    serverInfo.logMethod |= LOG_WHITELIST;
                else
                    cout<<"Ignoring unknown command switch: "<<argv[pos]<<endl;
            }
            else
                break;
        }
    }
    return pos;
}

void loadConfig(string *host)
{
    string line;
    ifstream rFile(LASTRESORT);
    if (rFile.is_open())
    {
        while (getline(rFile,line))
        {
            if (iswm(nospace(line),"#*")) continue;
            line = lower(line);
            if (gettok(line,1,"=") == "ip") host[0] = gettok(line,2,"=");
            if (gettok(line,1,"=") == "port") host[1] = gettok(line,2,"=");
        }
        rFile.close();
    }
}

void enterLoadedState()
{
    ofstream file ("listo.nada");
    if (file.is_open())
    {
        file<<"todo";
        file.close();
    }
}

void sendHandshake(int sockfd)
{
    string shake = string(VERSION) + "\r";
    send(sockfd,shake.c_str(),shake.size(),0);
}

hostent *resolveHost(const string &host)
{
    hostent *server = gethostbyname(host.c_str());
    if (server == NULL)
    {
        cerr<<"Error, no such host "<<host<<" . . .";
        if (host != "127.0.0.1")
        {
            cerr<<" Trying 127.0.0.1 . . .\n";
            if ((server = gethostbyname("127.0.0.1")) == NULL)
                return NULL;
        }
        else
            return NULL;
    }
    return server;
}

void createDirs()
{
    mkdirIf("./halfMod/");
    mkdirIf("./halfMod/plugins/");
    mkdirIf("./halfMod/config/");
    mkdirIf("./halfMod/userdata/");
    mkdirIf("./halfMod/logs/");
}

void loadAllExtensions(hmGlobal &info)
{
    vector<string> paths;
    info.extensions.reserve(findPlugins("./halfMod/extensions/",paths));
    for (auto it = paths.begin(), ite = paths.end();it != ite;++it)
        loadExtension(info,*it);
}

void loadAllPlugins(hmGlobal &info, vector<hmHandle> &plugins)
{
    vector<string> paths;
    size_t foo = findPlugins("./halfMod/plugins/",paths);
    plugins.reserve(foo);
    std::cout<<"Loading "<<foo<<" plugins . . ."<<std::endl;
    for (auto it = paths.begin(), ite = paths.end();it != ite;++it)
        loadPlugin(info,plugins,*it);
    processEvent(plugins,HM_ONPLUGINSLOADED);
}

void loadAssets(hmGlobal &info, vector<hmHandle> &plugins, vector<hmConsoleFilter> &filters)
{
    createDirs();
    hashAdmins(info,ADMINCONF);
    loadAllExtensions(info);
    loadAllPlugins(info,plugins);
    hashConsoleFilters(filters,plugins,CONFILTERPATH);
    enterLoadedState();
    cout<<"Assets loaded . . .\n";
}

bool receiveHandshake(int sockfd, hmGlobal &info, vector<hmHandle> &plugins)
{
    string line;
    if (readSock(sockfd,line) > 1)
    {
        size_t pos[2];
        if ((pos[0] = line.find('\t')) != std::string::npos)
        {
            if ((pos[1] = line.find('\t',pos[0]+1)) != std::string::npos)
            {
                info.hsVer = line.substr(0,pos[0]);
                if (pos[1]-pos[0] > 1)
                {
                    info.mcVer = line.substr(pos[0]+1,pos[1]);
                    if (line.size() > pos[1])
                        info.world = line.substr(pos[1]+1);
                }
                cout<<"Link established with "<<info.hsVer<<endl;
                rens::smatch hsMl;
                #ifdef HM_USE_PCRE2
                hsMl.capture.push_back({line,0});
                hsMl.capture.push_back({info.hsVer,0});
                hsMl.capture.push_back({info.mcVer,pos[0]});
                hsMl.capture.push_back({info.world,pos[1]});
                #endif
                processEvent(plugins,HM_ONHSCONNECT,hsMl);
                return true;
            }
        }
    }
    cerr<<"Invalid handshake received. \""<<line<<"\" Disconnecting . . ."<<endl;
    return false;
}

void handleDisconnect(int &sockfd)
{
    close(sockfd);
    sockfd = 0;
}

void handleDisconnect(int &sockfd, vector<hmHandle> &plugins)
{
    handleDisconnect(sockfd);
    processEvent(plugins,HM_ONHSDISCONNECT);
    cerr<<"Disconnected from host . . . Attempting to reconnect . . ."<<endl;
}

int initSocket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    while (sockfd < 1)
    {
        cerr<<"Error opening socket . . . Retrying in 5 seconds . . ."<<endl;
        sleep(5);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }
    return sockfd;
}

int handleConnection(int &sockfd, fd_set &readfds, hmGlobal &serverInfo, vector<hmHandle> &plugins, hostent *server, int port)
{
    if (sockfd < 1)
    {
        sockfd = initSocket();
        tryConnect(serverInfo,sockfd,server,port);
        sendHandshake(sockfd);
        if (receiveHandshake(sockfd,serverInfo,plugins) == false)
            handleDisconnect(sockfd);
    }
    if (sockfd > 0)
        return resetSocketSelect(plugins,readfds,sockfd);
    return 0;
}

void tryConnect(hmGlobal &serverInfo, int &sockfd, hostent *server, int port)
{
    static sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = htons(port);
    serverInfo.hsSocket = -1;
    while (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
        cerr<<"No connection to halfShell . . . Retrying in 1 second . . ."<<endl;
        sleep(1);
    }
    serverInfo.hsSocket = sockfd;
}

int resetSocketSelect(vector<hmHandle> &plugins, fd_set &readfds, int sockfd)
{
    static timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(sockfd,&readfds);
    FD_SET(0,&readfds);
    //timeout.tv_sec = 0;
    //timeout.tv_usec = 10;
    timeout = processTimers(plugins);
    //cout<<"wtf "<<timeout.tv_sec<<"."<<timeout.tv_usec<<endl;
    return select(sockfd+1,&readfds,NULL,NULL,&timeout);
}

int loadPlugin(hmGlobal &info, vector<hmHandle> &plugins, const string &path)
{
    std::cout<<"[HM] Loading plugin \""<<path<<"\" . . ."<<std::endl;
    for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
    {
        if (it->getPath() == path)
        {
            cerr<<"[HM] Plugin \""<<path<<"\" is already loaded."<<endl;
            return 3;
        }
    }
    hmHandle temp;
    if (!temp.load(path,&info))
    {
        cerr<<"[HM] Error loading plugin \""<<path<<"\" . . .\n";
        return 1;
    }
    else
    {
        if (temp.getAPI() != API_VERSION)
        {
            cout<<"[HM] Error plugin \""<<path<<"\" was compiled with a different version of the API ("<<temp.getAPI()<<") . . . Recompile with "<<API_VERSION<<".\n";
            return 2;
        }
        else
        {
            plugins.push_back(temp);
            //cout<<"[HM] Plugin \""<<plugins[plugins.size()-1].getInfo().name<<"\" loaded . . .\n";
        }
    }
    return 0;
}

int loadExtension(hmGlobal &info, const string &path)
{
    std::cout<<"[HM] Loading extension \""<<path<<"\" . . ."<<std::endl;
    for (auto it = info.extensions.begin(), ite = info.extensions.end();it != ite;++it)
    {
        if (it->getPath() == path)
        {
            cerr<<"[HM] Extension \""<<path<<"\" is already loaded."<<endl;
            return 3;
        }
    }
    hmExtension temp;
    if (!temp.load(path,&info))
    {
        cerr<<"[HM] Error loading extension \""<<path<<"\" . . .\n";
        return 1;
    }
    else
    {
        if (temp.getAPI() != API_VERSION)
        {
            cout<<"[HM] Error extension \""<<path<<"\" was compiled with a different version of the API ("<<temp.getAPI()<<") . . . Recompile with "<<API_VERSION<<".\n";
            return 2;
        }
        else
        {
            info.extensions.push_back(temp);
            //cout<<"[HM] Extension \""<<info.extensions[info.extensions.size()-1].getInfo().name<<"\" loaded . . .\n";
        }
    }
    return 0;
}

int readSock(int sock, string &buffer)
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

size_t findPlugins(const char *dir, vector<string> &paths)
{
    DIR *wd;
    struct dirent *entry;
    if ((wd = opendir(dir)))
    {
        while ((entry = readdir(wd)))
            if (strright(entry->d_name,4) == ".hmo")
                paths.push_back(dir + string(entry->d_name));
        closedir(wd);
    }
    return paths.size();
}

#define REGPTRN_ALL          0
#define REGPTRN_TIME         1
#define REGPTRN_INFO         2
#define REGPTRN_IGNORE       3
#define REGPTRN_SAY          4
#define REGPTRN_CHATCMD      5
#define REGPTRN_FAKESAY      6
#define REGPTRN_ACTION       7
#define REGPTRN_CONNECT      8
#define REGPTRN_GAMERULE     9
#define REGPTRN_SERVERSTART  10
#define REGPTRN_LEVELPREP    11
#define REGPTRN_LEVELINIT    12
#define REGPTRN_LIST         13
#define REGPTRN_JOIN         14
#define REGPTRN_DISCONNECT   15
#define REGPTRN_LEFT         16
#define REGPTRN_ADVANCE      17
#define REGPTRN_GOAL         18
#define REGPTRN_CHALLENGE    19
#define REGPTRN_DEATH        20
#define REGPTRN_WARN         21
#define REGPTRN_SHUTDOWN     22
#define REGPTRN_AUTH         23
#define REGPTRN_NOEXEC       24
#define REGPTRN_GLOBAL       25
#define REGPTRN_PRINT        26
#define REGPTRN_SHUTDPOST    27
#define REGPTRN_MAX          28

// in charge of populating the hmGlobal struct and controlling console output
// I said f it and made it handle everything.
int processThread(hmGlobal &info, vector<hmHandle> &plugins, string &thread)
{
    #ifdef HM_USE_PCRE2
    static rens::regex ptrns[REGPTRN_MAX] =
    {
        ".*",
        "\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] (.+)",
        "\\[Server thread/INFO\\]: (.*)",
        "[^\\s\\[\\]<>]+ (did not match|has the following entity data:|has [0-9]+ experience|has [0-9]+ \\[.*?\\]|moved too quickly!|moved wrongly!).*",
        "<(\\S+?)> (.*)",
        "<(\\S+?)> !(\\S+) ?(.*)",
        "\\[(\\S+?)\\] (.*)",
        "\\* (\\S+) (.+)",
        "([^\\s\\[]+)\\[/([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}):([0-9]{1,})\\] logged in.*",
        "\\[?[^ ]* ?Gamerule ([^ ]+) is (now|currently) set to: (.+?)\\]?$",
        "Starting minecraft server version (.+)",
        "Preparing level (.+)",
        "Done \\((.*)\\)!.*",
        "There are ([0-9]{1,})(/| of a max )([0-9]{1,}) players online:\\s*(.*)",
        "(\\S+) joined the game",
        "(\\S+) lost connection: (.*)",
        "(\\S+) left the game",
        "(\\S+) has made the advancement (.*)",
        "(\\S+) has reached the goal (.*)",
        "(\\S+) has completed the challenge (.*)",
        "(\\S+) (.+)",
        "\\[Server thread/WARN\\]: (.*)",
        "\\[Server Shutdown Thread/INFO\\]: (.*)",
        "\\[User Authenticator #([0-9]{1,})/INFO\\]: UUID of player (\\S+) is (.*)",
        "\\[Server thread/ERROR\\]: Couldn't execute command for (\\S+): (\\S+) ?(.*)",
        "::\\[GLOBAL\\] (.+)",
        "::\\[PRINT\\] (.+)",
        "\\[HS\\] Server closed with exit status: (.*)"
    };
    #else
    static const array<regex,REGPTRN_MAX> ptrns =
    {
        regex(".*"),
        regex("\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] (.+)"),
        regex("\\[Server thread/INFO\\]: (.*)"),
        regex("[^\\s\\[\\]<>]+ (did not match|has the following entity data:|has [0-9]+ experience|has [0-9]+ \\[.*?\\]|moved too quickly!|moved wrongly!).*"),
        regex("<(\\S+?)> (.*)"),
        regex("<(\\S+?)> !(\\S+) ?(.*)"),
        regex("\\[(\\S+?)\\] (.*)"),
        regex("\\* (\\S+) (.+)"),
        regex("([^\\s\\[]+)\\[/([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}):([0-9]{1,})\\] logged in.*"),
        regex("\\[?[^ ]* ?Gamerule ([^ ]+) is (now|currently) set to: (.+?)\\]?$"),
        regex("Starting minecraft server version (.+)"),
        regex("Preparing level (.+)"),
        regex("Done \\((.*)\\)!.*"),
        regex("There are ([0-9]{1,})(/| of a max )([0-9]{1,}) players online:\\s*(.*)"),
        regex("(\\S+) joined the game"),
        regex("(\\S+) lost connection: (.*)"),
        regex("(\\S+) left the game"),
        regex("(\\S+) has made the advancement (.*)"),
        regex("(\\S+) has reached the goal (.*)"),
        regex("(\\S+) has completed the challenge (.*)"),
        regex("(\\S+) (.+)"),
        regex("\\[Server thread/WARN\\]: (.*)"),
        regex("\\[Server Shutdown Thread/INFO\\]: (.*)"),
        regex("\\[User Authenticator #([0-9]{1,})/INFO\\]: UUID of player (\\S+) is (.*)"),
        regex("\\[Server thread/ERROR\\]: Couldn't execute command for (\\S+): (\\S+) ?(.*)"),
        regex("::\\[GLOBAL\\] (.+)"),
        regex("::\\[PRINT\\] (.+)"),
        regex("\\[HS\\] Server closed with exit status: (.*)")
    };
    #endif
        
    static bool catchLine = false;
    if (catchLine == true)
    {
        catchLine = false;
        string nextLine = strremove(thread,",");
        size_t pos[2] = {0};
        for (int i = 3;i;i--)
            pos[0] = nextLine.find(' ',pos[0])+1;
        while (pos[1] < string::npos)
        {
            pos[1] = nextLine.find(' ',pos[0]);
            loadPlayerData(info,nextLine.substr(pos[0],pos[1]));
            pos[0] = pos[1]+1;
        }
    }
    short blocking = 0;
    rens::smatch ml;
    vector<int> events;
    bool newEvent;
    for (auto it = info.conFilter->begin(), ite = info.conFilter->end();it != ite;++it)
    {
        if (rens::regex_search(thread,ml,it->filter))
        {
            if (it->blockOut)
                blocking |= HM_BLOCK_OUTPUT;
            if (it->blockEvent)
                blocking |= HM_BLOCK_EVENT;
            if (it->blockHook)
                blocking |= HM_BLOCK_HOOK;
            if (it->event)
            {
                newEvent = true;
                for (auto iti = events.begin(), itie = events.end();iti != itie;++iti)
                {
                    if (*iti == it->event)
                    {
                        newEvent = false;
                        break;
                    }
                }
                if (newEvent)
                {
                    events.push_back(it->event);
                    if (processEvent(plugins,it->event,ml))
                        blocking |= HM_BLOCK_EVENT;
                }
            }
        }
    }
    if ((blocking & HM_BLOCK_OUTPUT) == 0)
        cout<<thread<<endl;
    if ((blocking & HM_BLOCK_HOOK) == 0)
        if (processHooks(plugins,thread))
            blocking |= HM_BLOCK_EVENT;
    if ((blocking & HM_BLOCK_EVENT) == 0)
    {
        rens::regex_match(thread,ml,ptrns[REGPTRN_ALL]);
        if (!processEvent(plugins,HM_ONCONSOLERECV,ml))
        {
            bool isRemote = false;
            if (thread.compare(0,10,"[99:99:99]") == 0)
                isRemote = true;
            if (rens::regex_match(thread,ml,ptrns[REGPTRN_TIME]))
            {
                thread = ml[1].str();
                if (rens::regex_match(thread,ml,ptrns[REGPTRN_INFO]))
                {
                    if (!processEvent(plugins,HM_ONINFO,ml))
                    {
                        thread = ml[1].str();
                        if (!rens::regex_match(thread,ptrns[REGPTRN_IGNORE]))
                        {
                            if (rens::regex_match(thread,ml,ptrns[REGPTRN_SAY]))
                            {
                                if ((isRemote) || (hmIsPlayerOnline(ml[1].str())))
                                {
                                    rens::smatch ml1 = ml;
                                    bool blocked = false;
                                    if ((rens::regex_match(thread,ml,ptrns[REGPTRN_CHATCMD])) && (processCmd(info,plugins,"hm_" + ml[2].str(),ml[1].str(),ml[3].str())))
                                        blocked = true;
                                    if (!blocked)
                                    {
                                        if (isRemote)
                                            processEvent(plugins,HM_ONFAKETEXT,ml);
                                        else
                                            processEvent(plugins,HM_ONTEXT,ml1);
                                    }
                                }
                                else
                                    processEvent(plugins,HM_ONFAKETEXT,ml);
                            }
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_FAKESAY]))
                                processEvent(plugins,HM_ONFAKETEXT,ml);
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_ACTION]))
                                processEvent(plugins,HM_ONACTION,ml);
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_CONNECT]))
                            {
                                hmWritePlayerDat(ml[1].str(),"ip=" + ml[2].str() + "\n" + data2str("join=%li",time(NULL)),"ip=join");
                                processEvent(plugins,HM_ONCONNECT,ml);
                            }
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_GAMERULE]))
                            {
                                hmConVar *cvar = hmFindConVar(ml[1].str());
                                if ((cvar != nullptr) && ((cvar->flags & FCVAR_GAMERULE) > 0))
                                    cvar->setString(ml[3].str(),true);
                                processEvent(plugins,HM_ONGAMERULE,ml);
                            }
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_SERVERSTART]))
                            {
                                info.mcVer = ml[1].str();
                                info.players.clear();
                            }
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_LEVELPREP]))
                                info.world = ml[1].str();
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_LEVELINIT]))
                            {
                                hmSendRaw("list");
                                if (info.mcVer == "")
                                    cout<<"Minecraft version undefined."<<endl;
                                else
                                    cout<<"Minecraft version found: "<<info.mcVer<<endl;
                                if (info.world == "")
                                    cout<<"World undefined."<<endl;
                                else
                                    cout<<"World defined as: "<<info.world<<endl;
                                internalExec(info,plugins,"","#SERVER","autoexec");    // on world load setup
                                internalExec(info,plugins,"","#SERVER","startup");    // this will contain timed unbans if the server was offline when they expired
                                remove("./halfMod/configs/startup.cfg");    // delete startup
                                processEvent(plugins,HM_ONWORLDINIT,ml);
                            }
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_LIST]))
                            {
                                info.players.clear();
                                if (stoi(ml[1].str()) > 0)
                                {
                                    if (ml[2].str().size() < 2)
                                        catchLine = true;
                                    else
                                    {
                                        string users = strremove(ml[4].str(),",");
                                        size_t pos[2] = {0};
                                        while (pos[1] < string::npos)
                                        {
                                            pos[1] = users.find(' ',pos[0]);
                                            loadPlayerData(info,users.substr(pos[0],pos[1]));
                                            pos[0] = pos[1]+1;
                                        }
                                    }
                                }
                                info.maxPlayers = stoi(ml[3].str());
                            }
                            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_JOIN]))
                            {
                                loadPlayerData(info,ml[1].str());
                                processEvent(plugins,HM_ONJOIN,ml);
                            }
                            else if (hmIsPlayerOnline(thread.substr(0,thread.find(' '))))
                            {
                                if (rens::regex_match(thread,ml,ptrns[REGPTRN_DISCONNECT]))
                                {
                                    hmWritePlayerDat(ml[1].str(),data2str("quit=%li",time(NULL)) + "\nquitmsg=" + ml[2].str(),"quit=quitmsg");
                                    processEvent(plugins,HM_ONDISCONNECT,ml);
                                }
                                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_LEFT]))
                                {
                                    info.players.erase(stripFormat(lower(ml[1].str())));
                                    processEvent(plugins,HM_ONPART,ml);
                                }
                                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_ADVANCE]))
                                    processEvent(plugins,HM_ONADVANCE,ml);
                                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_GOAL]))
                                    processEvent(plugins,HM_ONGOAL,ml);
                                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_CHALLENGE]))
                                    processEvent(plugins,HM_ONCHALLENGE,ml);
                                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_DEATH]))
                                {
                                    string client = stripFormat(lower(ml[1].str())), msg = ml[2].str();
                                    time_t cTime = time(NULL);
                                    hmWritePlayerDat(client,data2str("death=%li",cTime) + "\ndeathmsg=" + msg,"death=deathmsg");
                                    auto c = info.players.find(client);
                                    if (c != info.players.end())
                                    {
                                        c->second.death = cTime;
                                        c->second.deathmsg = msg;
                                    }
                                    processEvent(plugins,HM_ONDEATH,ml);
                                }
                            }
                        }
                    }
                }
                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_WARN]))
                    processEvent(plugins,HM_ONWARN,ml);
                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_SHUTDOWN]))
                    processEvent(plugins,HM_ONSHUTDOWN,ml);
                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_AUTH]))
                {
                    hmWritePlayerDat(ml[2].str(),"uuid=" + ml[3].str(),"uuid",true);
                    processEvent(plugins,HM_ONAUTH,ml);
                }
                else if (rens::regex_match(thread,ml,ptrns[REGPTRN_NOEXEC]))
                    processCmd(info,plugins,"hm_" + ml[2].str(),ml[1].str(),ml[3].str(),false);
            }
            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_GLOBAL]))
                processEvent(plugins,HM_ONGLOBALMSG,ml);
            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_PRINT]))
                processEvent(plugins,HM_ONPRINTMSG,ml);
            else if (rens::regex_match(thread,ml,ptrns[REGPTRN_SHUTDPOST]))
                processEvent(plugins,HM_ONSHUTDOWNPOST,ml);
        }
    }
    return 0;
}

int processEvent(vector<hmHandle> &plugins, int event)
{
    hmEvent evnt;
    for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
    {
        if (it->findEvent(event,evnt))
        {
            if ((*evnt.func)(*it))
            {
                return 1;
            }
        }
    }
    return 0;
}

int processEvent(vector<hmHandle> &plugins, int event, rens::smatch thread)
{
    hmEvent evnt;
    for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
    {
        if (it->findEvent(event,evnt))
        {
            if ((*evnt.func_with)(*it,thread))
            {
                return 1;
            }
        }
    }
    return 0;
}

#define MAXINTERNALS    11
static internalCom cmdlist[MAXINTERNALS] =
{
    {"hm",0,""},
    {"hm_reloadadmins",FLAG_CONFIG,"Reload the admin config file."},
    {"hm_info",0,"View and search commands you have access to."},
    {"hm_version",0,"Display halfShell and halfMod version info."},
    {"hm_credits",0,"Display halfMod credits."},
    {"hm_exec",FLAG_CONFIG,"Execute a config file."},
    {"hm_rcon",FLAG_RCON,"Run commands as the server console."},
    {"hm_rehashfilter",FLAG_CONFIG,"Reload the console filter file."},
    {"hm_cvar",FLAG_CVAR,"View and set the values of ConVars."},
    {"hm_cvarinfo",FLAG_CVAR,"View and search all ConVars."},
    {"hm_resetcvar",FLAG_CVAR,"Reset a ConVar to its default value."}
};

int isInternalCmd(const string &cmd, int flags)
{
    for (int i = 0;i < MAXINTERNALS;i++)
    {
        if (cmdlist[i].cmd == cmd)
        {
            if ((cmdlist[i].flags == 0) || ((flags & cmdlist[i].flags) == cmdlist[i].flags))
                return i;
            else
                return -2;
        }
    }
    return -1;
}

int processCmd(hmGlobal &global, vector<hmHandle> &plugins, const string &cmd, const string &caller, const string &args, bool visible, bool console)
{
    if (caller == "Server")
        return 0;
    hmCommand com;
    hmPlayer player;
    player.flags = 0;
    player.name = caller;
    if ((console) || (caller == "#SERVER"))
        player.flags = FLAG_ROOT;
    else if (hmIsPlayerOnline(caller))
        player = hmGetPlayerInfo(caller);
    else// if (remote)
    {
        visible = false;
        string client = stripFormat(lower(caller));
        /*for (vector<hmAdmin>::iterator it = global.admins.begin(), ite = global.admins.end();it != ite;++it)
        {
            if (lower(it->client) == client)
            {
                flags = it->flags;
                break;
            }
        }*/
        auto c = global.admins.find(client);
        if (c != global.admins.end())
            player.flags = c->second.flags;
    }
    string *arga;
    int argc = numqtok(args," ")+1;
    arga = new string[argc];
    arga[0] = cmd;
    for (int i = 1;i < argc;i++)
        arga[i] = getqtok(args,i," ");
    int ret = 0, internal;
    if ((internal = isInternalCmd(cmd,player.flags)) > -1)
    {
        if (caller != "#SERVER")
            cout<<"[HM] "<<caller<<" is issuing the command "<<cmd<<endl;
        switch (internal)
        {
            case 0:
            {
                ret = internalHM(global,plugins,caller,arga,argc);
                break;
            }
            case 1:
            {
                ret = internalReload(global,caller);
                break;
            }
            case 2:
            {
                ret = internalInfo(plugins,caller,arga,argc);
                break;
            }
            case 3:
            {
                ret = internalVersion(global,caller);
                break;
            }
            case 4:
            {
                ret = internalCredits(global,caller);
                break;
            }
            case 5:
            {
                ret = internalExec(global,plugins,cmd,caller,args);
                break;
            }
            case 6:
            {
                ret = internalRcon(global,plugins,cmd,caller,args);
                break;
            }
            case 7:
            {
                ret = internalRehash(global,plugins,caller);
                break;
            }
            case 8:
            {
                ret = internalCvar(global,caller,arga,argc);
                break;
            }
            case 9:
            {
                ret = internalCvarInfo(global,caller,arga,argc);
                break;
            }
            case 10:
            {
                ret = internalCvarReset(global,caller,arga,argc);
                break;
            }
        }
    }
    else if (internal == -2)
    {
        hmReplyToClient(caller,"You do not have access to this command.");
        ret = 9;
    }
    else
    {
        for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
        {
            if (it->findCmd(cmd,com))
            {
                if ((com.flag == 0) || ((player.flags & com.flag) == com.flag))
                {
                    if (caller != "#SERVER")
                        cout<<"[HM] "<<caller<<" is issuing the command "<<cmd<<endl;
                    ret = (*com.func)(*it,player,arga,argc);
                    internal = 1;
                    break;
                }
                else
                {
                    hmReplyToClient(caller,"You do not have access to this command.");
                    internal = 1;
                    ret = 1;
                    break;
                }
            }
        }
        if ((console) && (internal != 1))
        {
            if (args.size() > 0)
                hmSendRaw(cmd + " " + args,false);
            else
                hmSendRaw(cmd,false);
            ret = 1;
        }
        else if ((internal < 0) && (!visible))
        {
            hmReplyToClient(caller,"Unknown command: " + cmd);
        }
    }
    delete[] arga;
    return ret;
}

int processHooks(vector<hmHandle> &plugins, const string &thread)
{
    rens::smatch ml;
    for (auto ita = recallGlobal(NULL)->extensions.begin(), itb = recallGlobal(NULL)->extensions.end();ita != itb;++ita)
    {
        for (auto it = ita->hooks.begin(), ite = ita->hooks.end();it != ite;++it)
        {
            if (it->name != "")
                if (rens::regex_search(thread,ml,it->rptrn))
                    if ((*it->func)(*ita,*it,ml))
                        return 1;
        }
    }
    for (auto ita = plugins.begin(), itb = plugins.end();ita != itb;++ita)
    {
        for (auto it = ita->hooks.begin(), ite = ita->hooks.end();it != ite;++it)
        {
            if (it->name != "")
                if (rens::regex_search(thread,ml,it->rptrn))
                    if ((*it->func)(*ita,*it,ml))
                        return 1;
        }
    }
    return 0;
}

int hashAdmins(hmGlobal &info, const string &path)
{
    // faster than looping through a mult table for each and every value.
    int FLAGS[26] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554431 };
    ifstream file (path);
    string line, flagstr;
    int flags;
    //regex comment ("#.*");
    //regex white ("\\s");
    for (auto it = info.players.begin(), ite = info.players.end();it != ite;++it)
        it->second.flags = 0;
    if (file.is_open())
    {
        info.admins.clear();
        while (getline(file,line))
        {
            //line = regex_replace(line,white,"");
            line = strremove(strremove(line,"\t")," ");
            //if (regex_match(line,comment))
            //    continue;
            if ((line.size() < 1) || (line.at(0) == '#'))
                continue;
            //line = regex_replace(line,comment,"");
            size_t p = line.find('#');
            if (p < std::string::npos)
                line.erase(p);
            if (numtok(line,"=") > 1)
            {
                flags = 0;
                flagstr = lower(deltok(line,1,"="));
                //for (int i = 0;i < flagstr.size();i++)
                for (auto i = flagstr.begin(), j = flagstr.end();i < j;++i)
                {
                    //if (!isalpha(flagstr[i]))
                    if (!isalpha(*i))
                        continue;
                    //flags |= FLAGS[flagstr[i]-97];
                    flags |= FLAGS[*i-97];
                }
                //info.admins.push_back({ gettok(line,1,"="), flags });
                string client = lower(gettok(line,1,"="));
                info.admins.emplace(client,hmAdmin({client,flags}));
                /*for (auto it = info.players.begin(), ite = info.players.end();it != ite;++it)
                {
                    if (lower(gettok(line,1,"=")) == stripFormat(lower(it->name)))
                        it->flags = flags;
                }*/
                if (hmIsPlayerOnline(client))
                    info.players.at(client).flags = flags;
            }
        }
        file.close();
        return 0;
    }
    return 1;
}

int hashConsoleFilters(vector<hmConsoleFilter> &filters, vector<hmHandle> &plugins, const string &path)
{
    ifstream file(path);
    if (file.is_open())
    {
        filters.clear();
        string line;
        while (getline(file,line))
        {
            hmConsoleFilter temp;
            if ((line.size() < 3) || (line.at(0) == '#') || (!stringisnum(gettok(line,1," "))))
                continue;
            temp.event = stoi(gettok(line,1," "));
            if (temp.event == 0)
                continue;
            if ((temp.event & HM_BLOCK_OUTPUT) != 0)
            {
                temp.event -= HM_BLOCK_OUTPUT;
                temp.blockOut = true;
            }
            if ((temp.event) && ((temp.event & HM_BLOCK_EVENT) != 0))
            {
                temp.event -= HM_BLOCK_EVENT;
                temp.blockEvent = true;
            }
            if ((temp.event) && ((temp.event & HM_BLOCK_HOOK) != 0))
            {
                temp.event -= HM_BLOCK_HOOK;
                temp.blockHook = true;
            }
            temp.name = "config";
            temp.filter = deltok(line,1," ");
            filters.push_back(temp);
        }
        file.close();
        processEvent(plugins,HM_ONREHASHFILTER);
        return 0;
    }
    return 1;
}

void loadPlayerData(hmGlobal &info, const string &name)
{
    /*for (auto it = info.players.begin();it != info.players.end();)
    {
        if (stripFormat(lower(it->name)) == stripFormat(lower(name)))
            it = info.players.erase(it);
        else
            ++it;
    }
    info.players.push_back(hmGetPlayerData(name));*/
    //auto c = info.players.find(name);
    //if (c != c.
    hmPlayer data = hmGetPlayerData(name);
    auto c = info.players.emplace(stripFormat(lower(name)),data);
    if (!c.second)
        c.first->second = data;
}

int internalHM(hmGlobal &global, vector<hmHandle> &plugins, const string &caller, string args[], int argc)
{
    if (argc < 2)
        internalVersion(global,caller);
    else if (argc < 3)
    {
        hmReplyToClient(caller,"Usage: " + args[0] + " plugins list");
        hmReplyToClient(caller,"Usage: " + args[0] + " plugins info <plugin>");
        hmReplyToClient(caller,"Usage: " + args[0] + " plugins load <plugin>");
        hmReplyToClient(caller,"Usage: " + args[0] + " plugins unload <plugin>");
        hmReplyToClient(caller,"Usage: " + args[0] + " plugins reload <plugin>");
        hmReplyToClient(caller,"Usage: " + args[0] + " extensions list");
        hmReplyToClient(caller,"Usage: " + args[0] + " extensions info <extension>");
    }
    else
    {
        int n = 0;
        if (args[1] + " " + args[2] == "plugins list")
        {
            hmReplyToClient(caller,"The following plugins are loaded:");
            for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
            {
                hmReplyToClient(caller,data2str("[%i] %s identifies as '%s'",n,it->getPlugin().c_str(),it->getInfo().name.c_str()));
                n++;
            }
        }
        else if (args[1] + " " + args[2] == "plugins info")
        {
            if (argc < 4)
                hmReplyToClient(caller,"Usage: " + args[0] + " plugins info <plugin>");
            else
            {
                bool found = false;
                for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
                {
                    if ((it->getPlugin() == args[3]) || (it->getPath() == args[3]) || ((stringisnum(args[3],0)) && (atoi(args[3].c_str()) == n)))
                    {
                        found = true;
                        hmReplyToClient(caller,data2str("[%i] Plugin %s:",n,it->getPlugin().c_str()));
                        hmReplyToClient(caller,"    Path   : " + it->getPath());
                        hmInfo temp = it->getInfo();
                        hmReplyToClient(caller,"    Name   : " + temp.name);
                        hmReplyToClient(caller,"    Author : " + temp.author);
                        hmReplyToClient(caller,"    Desc   : " + temp.description);
                        hmReplyToClient(caller,"    Version: " + temp.version);
                        if (hmIsPlayerOnline(caller))
                            hmSendRaw("tellraw " + caller + " [{\"text\":\"[HM]     Url    : \"},{\"text\":\"" + temp.url + "\",\"color\":\"blue\",\"clickEvent\":{\"action\":\"open_url\",\"value\":\"" + temp.url + "\"}}]",false);
                        else
                            hmReplyToClient(caller,"    Url    : " + temp.url);
                        hmReplyToClient(caller,data2str("    %i registered command(s).",it->totalCmds()));
                        hmReplyToClient(caller,data2str("    %i registered convar(s).",it->totalCvars()));
                        hmReplyToClient(caller,data2str("    %i registered event(s).",it->totalEvents()));
                        hmReplyToClient(caller,data2str("    %lu registered hook(s).",it->hooks.size()));
                        hmReplyToClient(caller,data2str("    %lu running timer(s).",it->timers.size()));
                    }
                    n++;
                }
                if (!found)
                    hmReplyToClient(caller,"Could not find any loaded plugins for the given criteria.");
            }
        }
        else if (args[1] + " " + args[2] == "plugins load")
        {
            if (argc < 4)
                hmReplyToClient(caller,"Usage: " + args[0] + " plugins load <plugin>");
            else
            {
                string path = "./halfMod/plugins/" + args[3] + ".hmo";
                int err = loadPlugin(global,plugins,path);
                if (caller != "#SERVER")
                {
                    switch (err)
                    {
                        case 0: // success
                        {
                            hmReplyToClient(caller,"Plugin '" + plugins[plugins.size()-1].getInfo().name + "' loaded successfully.");
                            break;
                        }
                        case 1: // not loaded
                        {
                            hmReplyToClient(caller,"Error loading plugin '" + path + "'.");
                            break;
                        }
                        case 2: // api mismatch
                        {
                            hmReplyToClient(caller,"Error plugin '" + path + "' was compiled with a different version of the API . . . Recompile with " + string(API_VERSION));
                            break;
                        }
                        case 3: // already loaded
                        {
                            hmReplyToClient(caller,"Plugin '" + path + "' is already loaded.");
                            break;
                        }
                    }
                }
            }
        }
        else if (args[1] + " " + args[2] == "plugins unload")
        {
            if (argc < 4)
                hmReplyToClient(caller,"Usage: " + args[0] + " plugins unload <plugin>");
            else
            {
                bool found = false;
                for (auto it = plugins.begin();it != plugins.end();)
                {
                    if ((it->getPlugin() == args[3]) || (it->getPath() == args[3]) || ((stringisnum(args[3],0)) && (stoi(args[3]) == n)))
                    {
                        found = true;
                        it->unload();
                        //it->hooks.clear();
                        it = plugins.erase(it);
                        hmReplyToClient(caller,"Plugin " + args[3] + " unloaded.");
                    }
                    else    ++it;
                    n++;
                }
                if (!found)
                    hmReplyToClient(caller,"Could not find any loaded plugins for the given criteria.");
            }
        }
        else if (args[1] + " " + args[2] == "plugins reload")
        {
            if (argc < 4)
                hmReplyToClient(caller,"Usage: " + args[0] + " plugins reload <plugin>");
            else
            {
                bool found = false;
                string path;
                for (auto it = plugins.begin();it != plugins.end();)
                {
                    if ((it->getPlugin() == args[3]) || (it->getPath() == args[3]) || ((stringisnum(args[3],0)) && (atoi(args[3].c_str()) == n)))
                    {
                        found = true;
                        it->unload();
                        path = it->getPath();
                        it = plugins.erase(it);
                        break;
                    }
                    else    ++it;
                    n++;
                }
                if (!found)
                    hmReplyToClient(caller,"Could not find any loaded plugins for the given criteria.");
                else
                {
                    int err = loadPlugin(global,plugins,path);
                    if (caller != "#SERVER")
                    {
                        switch (err)
                        {
                            case 0: // success
                            {
                                hmReplyToClient(caller,"Plugin '" + plugins[plugins.size()-1].getInfo().name + "' reloaded successfully.");
                                break;
                            }
                            case 1: // not loaded
                            {
                                hmReplyToClient(caller,"Error loading plugin '" + path + "'.");
                                break;
                            }
                            case 2: // api mismatch
                            {
                                hmReplyToClient(caller,"Error plugin '" + path + "' was compiled with a different version of the API . . . Recompile with " + string(API_VERSION));
                                break;
                            }
                            case 3: // already loaded
                            {
                                hmReplyToClient(caller,"Plugin '" + path + "' is already loaded. Then how was it loaded in the first place??");
                                break;
                            }
                        }
                    }
                }
            }
        }
        else if (args[1] + " " + args[2] == "extensions list")
        {
            hmReplyToClient(caller,"The following extensions are loaded:");
            for (auto it = global.extensions.begin(), ite = global.extensions.end();it != ite;++it)
            {
                hmReplyToClient(caller,data2str("[%i] %s identifies as '%s'",n,it->getExtension().c_str(),it->getInfo().name.c_str()));
                n++;
            }
        }
        else if (args[1] + " " + args[2] == "extensions info")
        {
            if (argc < 4)
                hmReplyToClient(caller,"Usage: " + args[0] + " extensions info <extension>");
            else
            {
                bool found = false;
                for (auto it = global.extensions.begin(), ite = global.extensions.end();it != ite;++it)
                {
                    if ((it->getExtension() == args[3]) || (it->getPath() == args[3]) || ((stringisnum(args[3],0)) && (atoi(args[3].c_str()) == n)))
                    {
                        found = true;
                        hmReplyToClient(caller,data2str("[%i] Extension %s:",n,it->getExtension().c_str()));
                        hmReplyToClient(caller,"    Path   : " + it->getPath());
                        hmInfo temp = it->getInfo();
                        hmReplyToClient(caller,"    Name   : " + temp.name);
                        hmReplyToClient(caller,"    Author : " + temp.author);
                        hmReplyToClient(caller,"    Desc   : " + temp.description);
                        hmReplyToClient(caller,"    Version: " + temp.version);
                        if (hmIsPlayerOnline(caller))
                            hmSendRaw("tellraw " + caller + " [{\"text\":\"[HM]     Url    : \"},{\"text\":\"" + temp.url + "\",\"color\":\"blue\",\"clickEvent\":{\"action\":\"open_url\",\"value\":\"" + temp.url + "\"}}]",false);
                        else
                            hmReplyToClient(caller,"    Url    : " + temp.url);
                        hmReplyToClient(caller,data2str("    %i registered convar(s).",it->totalCvars()));
                        hmReplyToClient(caller,data2str("    %lu registered hook(s).",it->hooks.size()));
                        hmReplyToClient(caller,data2str("    %lu running timer(s).",it->timers.size()));
                    }
                    n++;
                }
                if (!found)
                    hmReplyToClient(caller,"Could not find any loaded extensions for the given criteria.");
            }
        }
        else
        {
            hmReplyToClient(caller,"Usage: " + args[0] + " plugins list");
            hmReplyToClient(caller,"Usage: " + args[0] + " plugins info <plugin>");
            hmReplyToClient(caller,"Usage: " + args[0] + " plugins load <plugin>");
            hmReplyToClient(caller,"Usage: " + args[0] + " plugins unload <plugin>");
            hmReplyToClient(caller,"Usage: " + args[0] + " plugins reload <plugin>");
            hmReplyToClient(caller,"Usage: " + args[0] + " extensions list");
            hmReplyToClient(caller,"Usage: " + args[0] + " extensions info <extension>");
        }
    }
    return 0;
}

int internalReload(hmGlobal &global, const string &caller)
{
    if (hashAdmins(global,ADMINCONF))
    {
        hmReplyToClient(caller,"Unable to load admin config file . . .");
        return 1;
    }
    hmReplyToClient(caller,"Successfully reloaded the admin config file.");
    return 0;
}

int internalExec(hmGlobal &global, vector<hmHandle> &plugins, const string &cmd, const string &caller, const string &args)
{
    if (args == "")
    {
        hmReplyToClient(caller,"Usage: " + cmd + " <cfg file name>");
        return 8;
    }
    ifstream file ("./halfMod/configs/" + args + ".cfg");
    if (!file.is_open())
    {
        hmReplyToClient(caller,"Error: Cannot open file './halfMod/configs/" + args + ".cfg'");
        return 1;
    }
    string line;
    int lines = 0;
    for (;getline(file,line);lines++)
        processCmd(global,plugins,gettok(line,1," "),"#SERVER",deltok(line,1," "),true,true);
    hmReplyToClient(caller,data2str("Executed %i line(s) from file.",lines));
    file.close();
    return 0;
}

int internalInfo(vector<hmHandle> &plugins, const string &caller, string args[], int argc)
{
    int flags = hmGetPlayerFlags(caller);
    int plug = -1;
    string criteria = "";
    int page = 1;
    if (argc > 1)
    {
        if ((args[1] == "plugin") && (argc > 2))
        {
            bool found = false;
            if (args[2] == "internal")
            {
                hmReplyToClient(caller,"Displaying internal halfMod commands");
                plug = -2;
                found = true;
            }
            else
            {
                for (auto it = plugins.begin();it != plugins.end();++it)
                {
                    plug++;
                    if ((it->getPlugin() == args[2]) || (it->getPath() == args[2]) || ((stringisnum(args[2],0)) && (stoi(args[2]) == plug)))
                    {
                        hmReplyToClient(caller,"Displaying commands from plugin: '" + it->getPlugin() + "'");
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
            {
                hmReplyToClient(caller,"Could not find any loaded plugins for the given criteria.");
                return 3;
            }
            if ((argc > 3) && (stringisnum(args[3],1)))
                page = stoi(args[3]);
        }
        else
        {
            if ((argc > 2) && (stringisnum(args[2],1)))
            {
                criteria = args[1];
                page = stoi(args[2]);
            }
            else if (stringisnum(args[1],1))
                page = stoi(args[1]);
            else
                criteria = args[1];
        }
    }
    int n = -1;
    bool outTellraw = hmIsPlayerOnline(caller);
    vector<string> list;
    string entry;
    if (plug < 0)
    { // list internals
        for (int i = 1;i < MAXINTERNALS;i++)
        {
            if (criteria.size() > 0)
            {
                if ((!isin(cmdlist[i].cmd,criteria)) && (!isin(cmdlist[i].desc,criteria)))
                    continue;
            }
            if ((cmdlist[i].flags == 0) || ((flags & cmdlist[i].flags) > 0))
            {
                if (outTellraw)
                    entry = "[{\"text\":\"[HM] \"},{\"text\":\"" + cmdlist[i].cmd + "\",\"color\":\"gray\"},{\"text\":\" - \",\"color\":\"none\"},{\"text\":\"" + cmdlist[i].desc + "\",\"color\":\"dark_aqua\"}]";
                else
                    entry = cmdlist[i].cmd + " - " + cmdlist[i].desc;
                list.push_back(entry);
            }
        }
    }
    if (plug > -2)
    {
        for (auto it = plugins.begin(), ite = plugins.end();it != ite;++it)
        {
            n++;
            if ((plug > -1) && (n != plug))
                continue;
            for (auto com = it->cmds.begin(), come = it->cmds.end();com != come;++com)
            {
                if (criteria.size() > 0)
                {
                    if ((!isin(com->second.cmd,criteria)) && (!isin(com->second.desc,criteria)))
                        continue;
                }
                if ((com->second.flag == 0) || ((flags & com->second.flag) > 0))
                {
                    if (outTellraw)
                        entry = "[{\"text\":\"[HM] \"},{\"text\":\"" + com->second.cmd + "\",\"color\":\"gray\"},{\"text\":\" - \",\"color\":\"none\"},{\"text\":\"" + com->second.desc + "\",\"color\":\"dark_aqua\"}]";
                    else
                        entry = com->second.cmd + " - " + com->second.desc;
                    list.push_back(entry);
                }
            }
        }
    }
    if (list.size() < 1)
    {
        if (plug > -1)
            hmReplyToClient(caller,"This plugin doesn't seem to have any commands.");
        else if (criteria.size() > 0)
            hmReplyToClient(caller,"No commands found matching the criteria: '" + criteria + "'");
        else
            hmReplyToClient(caller,"No registered commands found.");
    }
    else
    {
        int start, stop;
        page++;
        do
        {
            page--;
            start = (page-1)*10;
            stop = page*10-1;
        }
        while (start >= list.size());
        int pages = list.size()/10;
        if (list.size() % 10)
            pages++;
        if (criteria.size() > 0)
            hmReplyToClient(caller,"Displaying page " + data2str("%i/%i",page,pages) + " matching: '" + criteria + "':");
        else
            hmReplyToClient(caller,"Displaying page " + data2str("%i/%i:",page,pages));
        for (auto it = list.begin()+start, ite = list.end();it != ite;++it)
        {
            if (outTellraw)
                hmSendRaw("tellraw " + caller + " " + *it);
            else
                hmReplyToClient(caller,*it);
            if (start == stop)
                break;
            start++;
        }
    }
    return 0;
}

int internalCvarInfo(hmGlobal &global, const string &caller, string args[], int argc)
{
    string criteria = "";
    int page = 1;
    if (argc > 1)
    {
        if ((argc > 2) && (stringisnum(args[2],1)))
        {
            criteria = args[1];
            page = stoi(args[2]);
        }
        else if (stringisnum(args[1],1))
            page = stoi(args[1]);
        else
            criteria = args[1];
    }
    bool outTellraw = hmIsPlayerOnline(caller);
    vector<string> list;
    string entry;
    for (auto it = global.conVars.begin(), ite = global.conVars.end();it != ite;++it)
    {
        if (criteria.size() > 0)
        {
            if ((!isin(it->second.getName(),criteria)) && (!isin(it->second.getDesc(),criteria)))
                continue;
        }
        if (outTellraw)
            entry = "[{\"text\":\"[HM] \"},{\"text\":\"" + it->second.getName() + "\",\"color\":\"gray\"},{\"text\":\" (Default: " + it->second.getDefault() + ")\",\"color\":\"gold\"},{\"text\":\" - \",\"color\":\"none\"},{\"text\":\"" + it->second.getDesc() + "\",\"color\":\"dark_aqua\"}]";
        else
            entry = it->second.getName() + " (Default: " + it->second.getDefault() + ") - " + it->second.getDesc();
        list.push_back(entry);
    }
    if (list.size() < 1)
    {
        if (criteria.size() > 0)
            hmReplyToClient(caller,"No ConVars found matching the criteria: '" + criteria + "'");
        else
            hmReplyToClient(caller,"No registered ConVars found.");
    }
    else
    {
        int start, stop;
        page++;
        do
        {
            page--;
            start = (page-1)*10;
            stop = page*10-1;
        }
        while (start >= list.size());
        int pages = list.size()/10;
        if (list.size() % 10)
            pages++;
        if (criteria.size() > 0)
            hmReplyToClient(caller,"Displaying page " + data2str("%i/%i",page,pages) + " matching: '" + criteria + "':");
        else
            hmReplyToClient(caller,"Displaying page " + data2str("%i/%i:",page,pages));
        for (auto it = list.begin()+start, ite = list.end();it != ite;++it)
        {
            if (outTellraw)
                hmSendRaw("tellraw " + caller + " " + *it);
            else
                hmReplyToClient(caller,*it);
            if (start == stop)
                break;
            start++;
        }
    }
    return 0;
}

int internalCvarReset(hmGlobal &global, const string &caller, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(caller,"Usage: " + args[0] + " <cvar> - Reset a cvar to default.");
        return 8;
    }
    //bool found = false;
    /*for (auto it = global.conVars.begin(), ite = global.conVars.end();it != ite;++it)
    {
        if (args[1] == it->getName())
        {
            found = true;
            if ((it->flags & FCVAR_READONLY))
                hmReplyToClient(caller,it->getName() + " is read-only.");
            else
            {
                it->reset();
                hmReplyToClient(caller,it->getName() + " has been reset to " + it->getAsString());
            }
            break;
        }
    }*/
    auto c = global.conVars.find(args[1]);
    if (c != global.conVars.end())
    {
        if ((c->second.flags & FCVAR_READONLY))
            hmReplyToClient(caller,c->second.getName() + " is read-only.");
        else
        {
            c->second.reset();
            hmReplyToClient(caller,c->second.getName() + " has been reset to " + c->second.getAsString());
        }
    }
    else
        hmReplyToClient(caller,"Unknown ConVar: " + args[1]);
    return 0;
}

int internalVersion(hmGlobal &global, const string &caller)
{
    hmReplyToClient(caller,string(VERSION) + " written by nigel.");
    hmReplyToClient(caller,global.hsVer + ", the vanilla Minecraft ModLoader written by nigel.");
    return 0;
}

int internalCredits(hmGlobal &global, const string &caller)
{
    hmReplyToClient(caller,string(VERSION) + " written by nigel.");
    hmReplyToClient(caller,global.hsVer + ", the vanilla Minecraft ModLoader written by nigel.");
    hmReplyToClient(caller,"Many thanks to SaintNewts and OSX for conceptual and technical advice.");
    hmReplyToClient(caller,"To highlight a few of the things making halfMod possible:");
    hmReplyToClient(caller,"SaintNewts: Running Minecraft in line buffered mode, allowing an instant file-less pipe directly into halfShell.");
    hmReplyToClient(caller,"OSX: Using dlfcn.h to dynamically load shared libraries, allowing the halfMod API to load and handle C++ plugins.");
    hmReplyToClient(caller,"SourceMod has been a hyuuuge influence over nearly every aspect of halfMod.");
    return 0;
}

int internalRcon(hmGlobal &global, vector<hmHandle> &plugins, const string &cmd, const string &caller, const string &args)
{
    if (args == "")
    {
        hmReplyToClient(caller,"Usage: " + cmd + " <command> [arguments ...]");
        return 8;
    }    
    processCmd(global,plugins,gettok(args,1," "),caller,deltok(args,1," "),true,true);
    return 0;
}

int internalRehash(hmGlobal &info, vector<hmHandle> &plugins, const string &caller)
{
    if (hashConsoleFilters(*info.conFilter,plugins,CONFILTERPATH))
    {
        hmReplyToClient(caller,"Unable to load console filter config file . . .");
        return 1;
    }
    hmReplyToClient(caller,"Successfully reloaded the console filter file.");
    return 0;
}

int internalCvar(hmGlobal &global, const string &caller, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(caller,"Usage: " + args[0] + " <convar> [value]");
        return 1;
    }
    /*bool found = false;
    for (auto it = global.conVars.begin(), ite = global.conVars.end();it != ite;++it)
    {
        if (args[1] == it->getName())
        {
            found = true;
            if (argc < 3)
                hmReplyToClient(caller,it->getName() + " = " + it->getAsString());
            else
            {
                if ((it->flags & FCVAR_READONLY))
                    hmReplyToClient(caller,it->getName() + " is read-only.");
                else
                {
                    it->setString(args[2]);
                    hmReplyToClient(caller,it->getName() + " set to " + it->getAsString());
                }
            }
            break;
        }
    }
    if (!found)*/
    auto it = global.conVars.find(args[1]);
    if (it != global.conVars.end())
    {
        if (argc < 3)
            hmReplyToClient(caller,it->second.getName() + " = " + it->second.getAsString());
        else
        {
            if ((it->second.flags & FCVAR_READONLY))
                hmReplyToClient(caller,it->second.getName() + " is read-only.");
            else
            {
                it->second.setString(args[2]);
                hmReplyToClient(caller,it->second.getName() + " set to " + it->second.getAsString());
            }
        }
    }
    else
        hmReplyToClient(caller,"Unknown ConVar: " + args[1]);
    return 0;
}

timeval processTimers(vector<hmHandle> &plugins)
{
    chrono::high_resolution_clock::time_point curTime = chrono::high_resolution_clock::now();
    timeval ret;
    ret.tv_sec = 60;
    ret.tv_usec = 0;
    int n;
    size_t cap;
    for (vector<hmExtension>::iterator ita = recallGlobal(NULL)->extensions.begin(), ite = recallGlobal(NULL)->extensions.end();ita != ite;++ita)
    {
        n = 0;
        cap = ita->timers.capacity();
        for (vector<hmExtTimer>::iterator it = ita->timers.begin();it != ita->timers.end();)
        {
            chrono::high_resolution_clock::time_point testTime = curTime;
            timeval tv;
            if (ita->invalidTimers)
                ita->invalidTimers = false;
            switch (it->type)
            {
                case MILLISECONDS:
                {
                    tv.tv_sec = it->interval/1000;
                    tv.tv_usec = (it->interval%1000)*1000;
                    testTime -= (chrono::milliseconds)(it->interval);
                    break;
                }
                case MICROSECONDS:
                {
                    tv.tv_sec = it->interval/1000000;
                    tv.tv_usec = (it->interval%1000000);
                    testTime -= (chrono::microseconds)(it->interval);
                    break;
                }
                case NANOSECONDS:
                {
                    tv.tv_sec = it->interval/1000000000;
                    tv.tv_usec = (it->interval%1000000000)/1000;
                    testTime -= (chrono::nanoseconds)(it->interval);
                    break;
                }
                default:
                {
                    tv.tv_sec = it->interval;
                    tv.tv_usec = 0;
                    testTime -= (chrono::seconds)(it->interval);
                }
            }
            if (it->last <= testTime)
            {
                if ((*it->func)(*ita,it->args))
                {
                    if ((ita->invalidTimers) && (cap != ita->timers.capacity()))
                        it = ita->timers.erase(ita->timers.begin()+n);
                    else
                        it = ita->timers.erase(it);
                }
                else
                {
                    if ((tv.tv_sec < ret.tv_sec) || ((tv.tv_sec == ret.tv_sec) && (tv.tv_usec < ret.tv_usec)))
                        ret = tv;
                    if ((ita->invalidTimers) && (cap != ita->timers.capacity()))
                        it = ita->timers.begin()+n;
                    it->last = curTime;
                    it++;
                    n++;
                }
            }
            else
            {
                n++;
                chrono::nanoseconds ns = it->last-testTime;
                tv.tv_sec = chrono::duration_cast<chrono::seconds>(ns).count();
                tv.tv_usec = chrono::duration_cast<chrono::microseconds>(ns).count()%1000000;
                if ((tv.tv_sec < ret.tv_sec) || ((tv.tv_sec == ret.tv_sec) && (tv.tv_usec < ret.tv_usec)))
                    ret = tv;
                it++;
            }
            //n++;
        }
    }
    for (vector<hmHandle>::iterator ita = plugins.begin(), ite = plugins.end();ita != ite;++ita)
    {
        n = 0;
        cap = ita->timers.capacity();
        for (vector<hmTimer>::iterator it = ita->timers.begin();it != ita->timers.end();)
        {
            chrono::high_resolution_clock::time_point testTime = curTime;
            timeval tv;
            if (ita->invalidTimers)
                ita->invalidTimers = false;
            switch (it->type)
            {
                case MILLISECONDS:
                {
                    tv.tv_sec = it->interval/1000;
                    tv.tv_usec = (it->interval%1000)*1000;
                    testTime -= (chrono::milliseconds)(it->interval);
                    break;
                }
                case MICROSECONDS:
                {
                    tv.tv_sec = it->interval/1000000;
                    tv.tv_usec = (it->interval%1000000);
                    testTime -= (chrono::microseconds)(it->interval);
                    break;
                }
                case NANOSECONDS:
                {
                    tv.tv_sec = it->interval/1000000000;
                    tv.tv_usec = (it->interval%1000000000)/1000;
                    testTime -= (chrono::nanoseconds)(it->interval);
                    break;
                }
                default:
                {
                    tv.tv_sec = it->interval;
                    tv.tv_usec = 0;
                    testTime -= (chrono::seconds)(it->interval);
                }
            }
            if (it->last <= testTime)
            {
                if ((*it->func)(*ita,it->args))
                {
                    if ((ita->invalidTimers) && (cap != ita->timers.capacity()))
                        it = ita->timers.erase(ita->timers.begin()+n);
                    else
                        it = ita->timers.erase(it);
                }
                else
                {
                    if ((tv.tv_sec < ret.tv_sec) || ((tv.tv_sec == ret.tv_sec) && (tv.tv_usec < ret.tv_usec)))
                        ret = tv;
                    if ((ita->invalidTimers) && (cap != ita->timers.capacity()))
                        it = ita->timers.begin()+n;
                    it->last = curTime;
                    it++;
                    n++;
                }
            }
            else
            {
                n++;
                chrono::nanoseconds ns = it->last-testTime;
                tv.tv_sec = chrono::duration_cast<chrono::seconds>(ns).count();
                tv.tv_usec = chrono::duration_cast<chrono::microseconds>(ns).count()%1000000;
                if ((tv.tv_sec < ret.tv_sec) || ((tv.tv_sec == ret.tv_sec) && (tv.tv_usec < ret.tv_usec)))
                    ret = tv;
                it++;
            }
            //n++;
        }
    }
    return ret;
}

