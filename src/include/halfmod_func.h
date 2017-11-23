#ifndef HM_ENGINE
#define HM_ENGINE

#define LASTRESORT	"./halfMod/config/failsafe.conf"
#define CONFILTERPATH	"./halfMod/config/consolefilter.txt"
#define ADMINCONF	"./halfMod/config/admins.conf"

struct hmConsoleFilter
{
	regex filter;	// pattern to match
	bool blocking;	// if true, the matching lines will not be processed at all, false will simply not display any lines matching
};

int readSock(int sock, string &buffer);
int findPlugins(const char *dir, vector<string> &paths);
int loadPlugin(hmGlobal &info, vector<hmHandle> &p, string path);
void loadPlayerData(hmGlobal &info, string name);
int hashAdmins(hmGlobal &info, string path);
int hashConsoleFilters(vector<hmConsoleFilter> &filters, string path);

int processThread(hmGlobal &info, vector<hmHandle> &plugins, vector<hmConsoleFilter> &filters, string thread);
int processFunc(hmHandle &plugin,int retVal);
int processEvent(vector<hmHandle> &plugins, int event, smatch thread);
int processCmd(hmGlobal &global, vector<hmHandle> &plugins, vector<hmConsoleFilter> &filters, string cmd, string caller, string args = "", bool console = false);
int processHooks(vector<hmHandle> &plugins, string thread);
void processTimers(vector<hmHandle> &plugins);

int isInternalCmd(string cmd);
int internalHM(hmGlobal &global, vector<hmHandle> &plugins, string caller, string args[], int argc);
int internalReload(hmGlobal &global, string caller);
int internalInfo(vector<hmHandle> &plugins, string caller, string args[], int argc);
int internalExec(hmGlobal &global, vector<hmHandle> &plugins, vector<hmConsoleFilter> &filters, string cmd, string caller, string args);
int internalVersion(hmGlobal &global, string caller);
int internalCredits(hmGlobal &global, string caller);
int internalRcon(hmGlobal &global, vector<hmHandle> &plugins, vector<hmConsoleFilter> &filters, string cmd, string caller, string args);
int internalRehash(vector<hmConsoleFilter> &filters, string caller);

#endif

