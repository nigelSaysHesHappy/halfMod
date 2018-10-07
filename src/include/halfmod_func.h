#ifndef HM_ENGINE
#define HM_ENGINE

#define LASTRESORT	"./halfMod/config/failsafe.conf"
#define CONFILTERPATH	"./halfMod/config/consolefilter.txt"
#define ADMINCONF	"./halfMod/config/admins.conf"

//extern vector<hmConsoleFilter> filters;

int handleCommandLineSwitches(hmGlobal &serverInfo, int argc, char *argv[]);
void loadConfig(string *host);
void enterLoadedState();
void sendHandshake(int sockfd);
hostent *resolveHost(const string &host);
void createDirs();
void loadAllExtensions(hmGlobal &info);
void loadAllPlugins(hmGlobal &info, vector<hmHandle> &plugins);
void loadAssets(hmGlobal &info, vector<hmHandle> &plugins, vector<hmConsoleFilter> &filters);
int initSocket();
bool receiveHandshake(int sockfd, hmGlobal &info, vector<hmHandle> &plugins);
void handleDisconnect(int &sockfd);
void handleDisconnect(int &sockfd, vector<hmHandle> &plugins);
int handleConnection(int &sockfd, fd_set &readfds, hmGlobal &serverInfo, vector<hmHandle> &plugins, hostent *server, int port);
void tryConnect(hmGlobal &serverInfo, int &sockfd, hostent *server, int port);
int resetSocketSelect(vector<hmHandle> &plugins, fd_set &readfds, int sockfd);

int readSock(int sock, string &buffer);
size_t findPlugins(const char *dir, vector<string> &paths);
int loadPlugin(hmGlobal &info, vector<hmHandle> &p, const string &path);
int loadExtension(hmGlobal &info, const string &path);
void loadPlayerData(hmGlobal &info, const string &name);
int hashAdmins(hmGlobal &info, const string &path);
int hashConsoleFilters(vector<hmConsoleFilter> &filters, vector<hmHandle> &plugins, const string &path);

int processThread(hmGlobal &info, vector<hmHandle> &plugins, string &thread);
int processFunc(hmHandle &plugin,int retVal);
int processEvent(vector<hmHandle> &plugins, int event);
int processEvent(vector<hmHandle> &plugins, int event, rens::smatch thread);
int processCmd(hmGlobal &global, vector<hmHandle> &plugins, const string &cmd, const string &caller, const string &args = "", bool visible = true, bool console = false);
int processHooks(vector<hmHandle> &plugins, const string &thread);
timeval processTimers(vector<hmHandle> &plugins);

int isInternalCmd(const string &cmd, int flags);
int internalHM(hmGlobal &global, vector<hmHandle> &plugins, const string &caller, string args[], int argc);
int internalReload(hmGlobal &global, const string &caller);
int internalInfo(vector<hmHandle> &plugins, const string &caller, string args[], int argc);
int internalExec(hmGlobal &global, vector<hmHandle> &plugins, const string &cmd, const string &caller, const string &args);
int internalVersion(hmGlobal &global, const string &caller);
int internalCredits(hmGlobal &global, const string &caller);
int internalRcon(hmGlobal &global, vector<hmHandle> &plugins, const string &cmd, const string &caller, const string &args);
int internalRehash(hmGlobal &info, vector<hmHandle> &plugins, const string &caller);
int internalCvar(hmGlobal &global, const string &caller, string args[], int argc);
int internalCvarInfo(hmGlobal &global, const string &caller, string args[], int argc);
int internalCvarReset(hmGlobal &global, const string &caller, string args[], int argc);

struct internalCom
{
    string cmd;
    int flags;
    string desc;
};

#endif

