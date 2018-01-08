#ifndef halfmod
#define halfmod

#include <vector>
#include <regex>
#include <string>
#include ".hmAPIBuild.h"

#define HM_NON_BLOCK        0
#define HM_BLOCK_OUTPUT     1
#define HM_BLOCK_EVENT      2
#define HM_BLOCK_HOOK       4
#define HM_BLOCK_ALL        7
#define HM_ONAUTH           8
#define HM_ONJOIN           16
#define HM_ONWARN           24
#define HM_ONINFO           32
#define HM_ONSHUTDOWN       40
#define HM_ONDISCONNECT     48
#define HM_ONPART           56
#define HM_ONADVANCE        64
#define HM_ONGOAL           72
#define HM_ONCHALLENGE      80
#define HM_ONACTION         88
#define HM_ONTEXT           96
#define HM_ONFAKETEXT       104
#define HM_ONDEATH          112
#define HM_ONWORLDINIT      120
#define HM_ONCONNECT        128
#define HM_ONHSCONNECT      136
#define HM_ONHSDISCONNECT   144
#define HM_ONSHUTDOWNPOST   152
#define HM_ONREHASHFILTER   160
#define HM_ONCUSTOM_1       168
#define HM_ONCUSTOM_2       176
#define HM_ONCUSTOM_3       184
#define HM_ONCUSTOM_4       192
#define HM_ONCUSTOM_5       200
// The limit of custom event id's is only bound by the max size of an int
// As long as the id cannot be & by 1, 2, or 4, it is valid
// These 5 are simply a base to use as an idea
// In plugin practice; only very large, unique numbers should be used

// for all events, args[0] will be the full thread line.
// the following entries will be pieces of useful info in the order of appearance on the line.

#define HM_ONCONNECT_FUNC       "onPlayerConnect"     //    1 = player, 2 = ip, 3 = port
#define HM_ONAUTH_FUNC          "onPlayerAuth"        //    1 = Auth #, 2 = player, 3 = uuid
#define HM_ONJOIN_FUNC          "onPlayerJoin"        //    1 = player
#define HM_ONWARN_FUNC          "onWarnThread"        //    1 = thread message
#define HM_ONINFO_FUNC          "onInfoThread"        //    1 = thread message
#define HM_ONSHUTDOWN_FUNC      "onServerShutdown"    //    1 = message
#define HM_ONDISCONNECT_FUNC    "onPlayerDisconnect"  //    1 = player, 2 = reason
#define HM_ONPART_FUNC          "onPlayerLeave"       //    1 = player
#define HM_ONADVANCE_FUNC       "onPlayerAdvancement" //    1 = player, 2 = advancement
#define HM_ONGOAL_FUNC          "onPlayerGoal"        //    1 = player, 2 = goal
#define HM_ONCHALLENGE_FUNC     "onPlayerChallenge"   //    1 = player, 2 = challenge
#define HM_ONACTION_FUNC        "onPlayerAction"      //    1 = player, 2 = message
#define HM_ONTEXT_FUNC          "onPlayerText"        //    1 = player, 2 = message
#define HM_ONFAKETEXT_FUNC      "onFakePlayerText"    //    1 = player, 2 = message
#define HM_ONDEATH_FUNC         "onPlayerDeath"       //    1 = player, 2 = death message
#define HM_ONWORLDINIT_FUNC     "onWorldInit"         //    1 = world prep time
#define HM_ONPLUGINSTART_FUNC   "onPluginStart"       //    
#define HM_ONPLUGINEND_FUNC     "onPluginEnd"         //    
#define HM_ONHSCONNECT_FUNC     "onHShellConnect"     //    1 = hS version, 2 = mc screen, 3 = mc version
#define HM_ONHSDISCONNECT_FUNC  "onHShellDisconnect"  //    
#define HM_ONSHUTDOWNPOST_FUNC  "onServerShutdownPost"//    
#define HM_ONREHASHFILTER_FUNC  "onRehashFilter"      //
#define HM_ONCUSTOM_1_FUNC      "onCustom1"           //    Values are defined by the filter
#define HM_ONCUSTOM_2_FUNC      "onCustom2"           //    Values are defined by the filter
#define HM_ONCUSTOM_3_FUNC      "onCustom3"           //    Values are defined by the filter
#define HM_ONCUSTOM_4_FUNC      "onCustom4"           //    Values are defined by the filter
#define HM_ONCUSTOM_5_FUNC      "onCustom5"           //    Values are defined by the filter

#define FLAG_ADMIN                1    //    a    Generic admin
#define FLAG_BAN                  2    //    b    Can ban/unban
#define FLAG_CHAT                 4    //    c    Special chat access
#define FLAG_CUSTOM1              8    //    d    Custom1
#define FLAG_EFFECT              16    //    e    Grant/revoke effects
#define FLAG_CONFIG              32    //    f    Execute server config files
#define FLAG_CHANGEWORLD         64    //    g    Can change world
#define FLAG_CVAR               128    //    h    Can change gamerules and plugin variables
#define FLAG_INVENTORY          256    //    i    Can access inventory commands
#define FLAG_CUSTOM2            512    //    j    Custom2
#define FLAG_KICK              1024    //    k    Can kick players
#define FLAG_CUSTOM3           2048    //    l    Custom3
#define FLAG_GAMEMODE          4096    //    m    Can change players' gamemode
#define FLAG_CUSTOM4           8192    //    n    Custom4
#define FLAG_OPER             16384    //    o    Grant/revoke operator status
#define FLAG_CUSTOM5          32768    //    p    Custom5
#define FLAG_RSVP             65536    //    q    Reserved slot access
#define FLAG_RCON            131072    //    r    Access to server RCON
#define FLAG_SLAY            262144    //    s    Slay/harm players
#define FLAG_TIME            524288    //    t    Can modify the world time
#define FLAG_UNBAN          1048576    //    u    Can unban, but not ban
#define FLAG_VOTE           2097152    //    v    Can initiate votes
#define FLAG_WHITELIST      4194304    //    w    Access to whitelist commands
#define FLAG_CHEATS         8388608    //    x    Commands like give, xp, enchant, etc...
#define FLAG_WEATHER       16777216    //    y    Can change the weather
#define FLAG_ROOT          33554431    //    z    Magic flag which grants all flags

#define LOG_KICK            1
#define LOG_BAN             2
#define LOG_OP              4
#define LOG_WHITELIST       8
#define LOG_ALWAYS          16

#define FILTER_NAME         1
#define FILTER_NO_SELECTOR  2
#define FILTER_IP           4
#define FILTER_FLAG         8
#define FILTER_UUID         16
#define FILTER_NO_EVAL      32

class hmHandle;

class hmExtension;

struct hmInfo
{
    std::string name;
    std::string author;
    std::string description;
    std::string version;
    std::string url;
};

struct hmPlugin
{
    std::string path;
    std::string name;
    std::string version;
};

struct hmCommand
{
    std::string cmd;
    int (*func)(hmHandle&,std::string,std::string[],int);
    int flag;
    std::string desc;
};

struct hmHook
{
    std::string name;
    //std::regex ptrn;
    std::string ptrn;
    int (*func)(hmHandle&,hmHook,std::smatch);
};

struct hmEvent
{
    int event;
    int (*func)(hmHandle&);
    int (*func_with)(hmHandle&,std::smatch);
};

struct hmPlayer
{
    std::string name;
    int flags;
    std::string uuid;
    std::string ip;
    int join;
    int quit;
    std::string quitmsg;
    int death;
    std::string deathmsg;
    std::string custom;
};

struct hmAdmin
{
    std::string client;
    int flags;
};

struct hmTimer
{
    std::string name;
    std::time_t interval;
    std::time_t last;
    int (*func)(hmHandle&,std::string);
    std::string args;
};

struct hmConsoleFilter
{
	std::regex filter;	// pattern to match
	bool blockOut = false;
	bool blockEvent = false;
	bool blockHook = false;
	int event;
	std::string name;
};

struct hmGlobal
{
    std::vector<hmPlayer> players;
    std::vector<hmAdmin> admins;
    std::vector<hmPlugin> pluginList;
    std::string mcVer;
    std::string hmVer;
    std::string hsVer;
    std::string world;
    std::string mcScreen;
    bool quiet = false;
    bool verbose = false;
    bool debug = false;
    int hsSocket;
    int logMethod;
    int maxPlayers;
    std::vector<hmConsoleFilter>* conFilter;
    std::vector<hmExtension> extensions;
};

class hmHandle
{
    public:
        std::vector<hmCommand> cmds;
        std::string vars;
        
        // create a new object
        hmHandle();
        
        // create a new object and call load(pluginPath)
        hmHandle(std::string pluginPath, hmGlobal *global);
        //~hmHandle();
        void unload();
        
        // load plugin, returns success
        bool load(std::string pluginPath, hmGlobal *global);
        
        // return the API version used to compile the plugin
        std::string getAPI();
        
        // return loaded
        bool isLoaded();
        
        // fill info struct with plugin info
        void pluginInfo(std::string name, std::string author, std::string description, std::string version, std::string url);
        
        // returns a copy of the hmInfo struct
        hmInfo getInfo();
        
        // register an event hook
        // returns total number of events hooked, -1 on error
        int hookEvent(int event, std::string function, bool withSmatch = true);
        
        // register an admin command
        // returns total number of commands registered by plugin, -1 on error
        int regAdminCmd(std::string command, std::string function, int flags, std::string description = "");
        
        // register a user command
        // returns total number of commands registered by plugin, -1 on error
        int regConsoleCmd(std::string command, std::string function, std::string description = "");
        
        // remove all user or admin commands that are 'command'
        // returns total number of commands registered by plugin
        int unregCmd(std::string command);
        
        // hook a regex pattern to a function name
        // returns total number of hooks registered by plugin, -1 on error
        int hookPattern(std::string name, std::string pattern, std::string function);
        
        // unhook all patterns with 'name'
        // returns total number of hooks registered by plugin
        int unhookPattern(std::string name);
        
        // returns a copy of the hmCommand struct containing 'command'
        hmCommand findCmd(std::string command);
        
        // returns a copy of the hmHook struct by name
        hmHook findHook(std::string name);
        
        // returns a copy of the hmEvent struct by event
        hmEvent findEvent(int event);
        
        std::vector<hmHook> hooks;
        
        // returns the path to the plugin
        std::string getPath();
        
        // starts a timer
        int createTimer(std::string name, std::time_t interval, std::string function, std::string args = "");
        
        // kills a timer by name
        int killTimer(std::string name);
        
        // set all timers with 'name' to be triggered on the following tick - returns total number of triggered timers
        int triggerTimer(std::string name);
        
        // returns a timer by name
        hmTimer findTimer(std::string name);
        
        std::vector<hmTimer> timers;
        
        int totalCmds();
        int totalEvents();
        
        std::string getPlugin();
        
        bool invalidTimers;
    
    private:
        bool loaded;
        hmInfo info;
        std::vector<hmEvent> events;
        void *module;
        std::string API_VER;
        std::string modulePath;
        std::string pluginName;
};

class hmExtension
{
    public:
        hmExtension();
        hmExtension(std::string extensionPath, hmGlobal *global);
        void unload();
        bool load(std::string extensionPath, hmGlobal *global);
        std::string getAPI();
        hmInfo getInfo();
        bool isLoaded();
        void extensionInfo(std::string name, std::string author, std::string description, std::string version, std::string url);
        std::string getPath();
        std::string getExtension();
        /*int getFunc(void *ptr, std::string func);
        int getFunc(int *ptr, std::string func);
        int getFunc(long *ptr, std::string func);
        int getFunc(short *ptr, std::string func);
        int getFunc(double *ptr, std::string func);
        int getFunc(float *ptr, std::string func);
        int getFunc(bool *ptr, std::string func);
        int getFunc(std::string *ptr, std::string func);*/
        void *getFunc(std::string func);
    private:
        bool loaded;
        hmInfo info;
        void *module;
        std::string API_VER;
        std::string modulePath;
        std::string extensionName;
};


int mcVerInt(std::string version);
void mkdirIf(const char *path);
std::string stripFormat(std::string str);
hmGlobal *recallGlobal(hmGlobal *global);
void hmSendRaw(std::string raw, bool output = true);
void hmServerCommand(std::string raw, bool output = true);
void hmReplyToClient(std::string client, std::string message);
void hmSendCommandFeedback(std::string client, std::string message);
void hmSendMessageAll(std::string message);
bool hmIsPlayerOnline(std::string client);
hmPlayer hmGetPlayerInfo(std::string client);
std::vector<hmPlayer>::iterator hmGetPlayerIterator(std::string client);
std::string hmGetPlayerUUID(std::string client);
std::string hmGetPlayerIP(std::string client);
int hmGetPlayerFlags(std::string client);
void hmOutQuiet(std::string text);
void hmOutVerbose(std::string text);
void hmOutDebug(std::string text);
hmPlayer hmGetPlayerData(std::string client);
void hmLog(std::string data, int logType, std::string logFile);
int hmProcessTargets(std::string client, std::string target, std::vector<hmPlayer> &targList, int filter);
bool hmIsPluginLoaded(std::string nameOrPath, std::string version = "");
std::string hmGetPluginVersion(std::string nameOrPath);
int hmAddConsoleFilter(std::string name, std::string ptrn, short blocking = HM_BLOCK_OUTPUT, int event = 0);
int hmRemoveConsoleFilter(std::string name);
int hmWritePlayerDat(std::string client, std::string data, std::string ignore, bool ifNotPresent = false);

#endif

