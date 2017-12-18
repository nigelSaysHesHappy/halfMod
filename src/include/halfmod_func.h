#ifndef HM_ENGINE
#define HM_ENGINE

#define LASTRESORT	"./halfMod/config/failsafe.conf"
#define CONFILTERPATH	"./halfMod/config/consolefilter.txt"
#define ADMINCONF	"./halfMod/config/admins.conf"

extern vector<hmConsoleFilter> filters;

int readSock(int sock, string &buffer);
int findPlugins(const char *dir, vector<string> &paths);
int loadPlugin(hmGlobal &info, vector<hmHandle> &p, string path);
void loadPlayerData(hmGlobal &info, string name);
int hashAdmins(hmGlobal &info, string path);
int hashConsoleFilters(vector<hmHandle> &plugins, string path);

int writePlayerDat(string client, string data, string ignore, bool ifNotPresent = false);
int processThread(hmGlobal &info, vector<hmHandle> &plugins, string thread);
int processFunc(hmHandle &plugin,int retVal);
int processEvent(vector<hmHandle> &plugins, int event);
int processEvent(vector<hmHandle> &plugins, int event, smatch thread);
int processCmd(hmGlobal &global, vector<hmHandle> &plugins, string cmd, string caller, string args = "", bool visible = true, bool console = false);
int processHooks(vector<hmHandle> &plugins, string thread);
void processTimers(vector<hmHandle> &plugins);

int isInternalCmd(string cmd);
int internalHM(hmGlobal &global, vector<hmHandle> &plugins, string caller, string args[], int argc);
int internalReload(hmGlobal &global, string caller);
int internalInfo(vector<hmHandle> &plugins, string caller, string args[], int argc);
int internalExec(hmGlobal &global, vector<hmHandle> &plugins, string cmd, string caller, string args);
int internalVersion(hmGlobal &global, string caller);
int internalCredits(hmGlobal &global, string caller);
int internalRcon(hmGlobal &global, vector<hmHandle> &plugins, string cmd, string caller, string args);
int internalRehash(vector<hmHandle> &plugins, string caller);

#endif

