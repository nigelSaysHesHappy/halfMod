#ifndef halfmod
#define halfmod

#include <vector>
#include <unordered_map>
#include <regex>
#include <string>
#include <chrono>
#include <ratio>
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
#define HM_ONGAMERULE       168
#define HM_ONGLOBALMSG      176
#define HM_ONPRINTMSG       184
#define HM_ONCONSOLERECV    192
#define HM_ONPLUGINSLOADED  200
#define HM_ONPLUGINEND      208
#define HM_ONCUSTOM_1       216
#define HM_ONCUSTOM_2       224
#define HM_ONCUSTOM_3       232
#define HM_ONCUSTOM_4       240
#define HM_ONCUSTOM_5       248
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
#define HM_ONGAMERULE_FUNC      "onGamerule"          //    1 = gamerule, 2 = "now" (has changed) or "currently" (query), 3 = value
#define HM_ONGLOBALMSG_FUNC     "onGlobalMessage"     //    1 = message
#define HM_ONPRINTMSG_FUNC      "onPrintMessage"      //    1 = message
#define HM_ONCONSOLERECV_FUNC   "onConsoleReceive"    //    0 = message // Only triggers if the message will be displayed
#define HM_ONPLUGINSLOADED_FUNC "onPluginsLoaded"     //    Called when all plugins are finished loading
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

#define SECONDS         0
#define MILLISECONDS    1
#define MICROSECONDS    2
#define NANOSECONDS     3

#define FCVAR_NOTIFY    1   // Clients are notified of changes
#define FCVAR_CONSTANT  2   // The cvar is assigned to the default value and cannot be changed
#define FCVAR_READONLY  4   // Only internal methods will be able to change the value, gamerule will be able to change it, but hm_cvar will not
#define FCVAR_GAMERULE  8   // The cvar is tied to a gamerule of the same name

class hmHandle;
class hmConVar;

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

struct hmCommand
{
    std::string cmd;
    int (*func)(hmHandle&,const hmPlayer&,std::string[],int);
    int flag;
    std::string desc;
};

struct hmHook
{
    std::string name;
    std::regex rptrn;
    std::string ptrn;
    int (*func)(hmHandle&,hmHook,std::smatch);
};

struct hmExtHook
{
    std::string name;
    std::regex rptrn;
    std::string ptrn;
    int (*func)(hmExtension&,hmExtHook,std::smatch);
};

struct hmEvent
{
    int event;
    int (*func)(hmHandle&);
    int (*func_with)(hmHandle&,std::smatch);
};

struct hmAdmin
{
    std::string client;
    int flags;
};

struct hmTimer
{
    std::string name;
    long interval;
    std::chrono::high_resolution_clock::time_point last;
    int (*func)(hmHandle&,std::string);
    std::string args;
    short type;
};

struct hmExtTimer
{
    std::string name;
    long interval;
    std::chrono::high_resolution_clock::time_point last;
    int (*func)(hmExtension&,std::string);
    std::string args;
    short type;
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

struct hmConVarHook
{
    int (*func)(hmConVar&,std::string,std::string);
};

class hmConVar
{
    public:
        hmConVar();
        hmConVar(const std::string &cname, const std::string &defVal, const std::string &description = "", short flag = 0, bool hasMinimum = false, float minimum = 0.0, bool hasMaximum = false, float maximum = 0.0);
        short flags;
        bool hasMin;
        bool hasMax;
        float min;
        float max;
        std::string getName();
        std::string getDesc();
        std::string getDefault();
        void reset();
        std::string getAsString();
        bool getAsBool();
        int getAsInt();
        float getAsFloat();
        void setString(std::string newValue, bool wasSet = false);
        void setBool(bool newValue);
        void setInt(int newValue);
        void setFloat(float newValue);
        std::vector<hmConVarHook> hooks;
    private:
        std::string name;
        std::string desc;
        std::string defaultValue;
        std::string value;
};

struct hmGlobal
{
    //std::vector<hmPlayer> players;
    std::unordered_map<std::string,hmPlayer> players;
    //std::vector<hmAdmin> admins;
    std::unordered_map<std::string,hmAdmin> admins;
    std::vector<hmPlugin> pluginList;
    //std::vector<hmConVar> conVars;
    std::unordered_map<std::string,hmConVar> conVars;
    std::string mcVer;
    std::string hmVer;
    std::string hsVer;
    std::string world;
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
        //std::vector<hmCommand> cmds;
        std::unordered_map<std::string,hmCommand> cmds;
        std::string vars;
        
        // create a new object
        hmHandle();
        
        // create a new object and call load(pluginPath)
        hmHandle(const std::string &pluginPath, hmGlobal *global);
        //~hmHandle();
        void unload();
        
        // load plugin, returns success
        bool load(const std::string &pluginPath, hmGlobal *global);
        
        // return the API version used to compile the plugin
        std::string getAPI();
        
        // return loaded
        bool isLoaded();
        
        // fill info struct with plugin info
        void pluginInfo(const std::string &name, const std::string &author, const std::string &description, const std::string &version, const std::string &url);
        
        // returns a copy of the hmInfo struct
        hmInfo getInfo();
        
        // register an event hook
        // returns total number of events hooked, -1 on error
        int hookEvent(int event, const std::string &function, bool withSmatch = true);
        int hookEvent(int event, int (*func)(hmHandle&));
        int hookEvent(int event, int (*func_with)(hmHandle&,std::smatch));
        
        // register an admin command
        // returns total number of commands registered by plugin, -1 on error
        int regAdminCmd(const std::string &command, const std::string &function, int flags, const std::string &description = "");
        int regAdminCmd(const std::string &command, int (*func)(hmHandle&,const hmPlayer&,std::string[],int), int flags, const std::string &description = "");
        
        // register a user command
        // returns total number of commands registered by plugin, -1 on error
        int regConsoleCmd(const std::string &command, const std::string &function, const std::string &description = "");
        int regConsoleCmd(const std::string &command, int (*func)(hmHandle&,const hmPlayer&,std::string[],int), const std::string &description = "");
        
        // remove all user or admin commands that are 'command'
        // returns total number of commands registered by plugin
        int unregCmd(const std::string &command);
        
        // hook a regex pattern to a function name
        // returns total number of hooks registered by plugin, -1 on error
        int hookPattern(const std::string &name, const std::string &pattern, const std::string &function);
        int hookPattern(const std::string &name, const std::string &pattern, int (*func)(hmHandle&,hmHook,std::smatch));
        
        // unhook all patterns with 'name'
        // returns total number of hooks registered by plugin
        int unhookPattern(const std::string &name);
        
        // returns a copy of the hmCommand struct containing 'command'
        bool findCmd(const std::string &command, hmCommand &cmd);
        
        // returns a copy of the hmHook struct by name
        bool findHook(const std::string &name, hmHook &hook);
        
        // returns a copy of the hmEvent struct by event
        bool findEvent(int event, hmEvent &evnt);
        
        std::vector<hmHook> hooks;
        
        // returns the path to the plugin
        std::string getPath();
        
        // starts a timer
        int createTimer(const std::string &name, long interval, const std::string &function, const std::string &args = "", short type = SECONDS);
        int createTimer(const std::string &name, long interval, int (*func)(hmHandle&,std::string), const std::string &args = "", short type = SECONDS);
        
        // kills a timer by name
        int killTimer(const std::string &name);
        
        // set all timers with 'name' to be triggered on the following tick - returns total number of triggered timers
        int triggerTimer(const std::string &name);
        
        // returns a timer by name
        bool findTimer(const std::string &name, hmTimer &timer);
        
        std::vector<hmTimer> timers;
        
        int totalCmds();
        int totalEvents();
        
        std::string getPlugin();
        
        bool invalidTimers;
        
        hmConVar* createConVar(const std::string &name, const std::string &defaultValue, const std::string &description = "", short flags = 0, bool hasMin = false, float min = 0.0, bool hasMax = false, float max = 0.0);
        
        int hookConVarChange(hmConVar *cvar, const std::string &function);
        int hookConVarChange(hmConVar *cvar, int (*func)(hmConVar&,std::string,std::string));
        
        int totalCvars();
    
    private:
        std::vector<std::string> conVarNames;
        bool loaded;
        hmInfo info;
        //std::vector<hmEvent> events;
        std::unordered_map<int,hmEvent> events;
        void *module;
        std::string API_VER;
        std::string modulePath;
        std::string pluginName;
};

class hmExtension
{
    public:
        hmExtension();
        hmExtension(const std::string &extensionPath, hmGlobal *global);
        void unload();
        bool load(const std::string &extensionPath, hmGlobal *global);
        std::string getAPI();
        hmInfo getInfo();
        bool isLoaded();
        void extensionInfo(const std::string &name, const std::string &author, const std::string &description, const std::string &version, const std::string &url);
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
        void *getFunc(const std::string &func);
        std::vector<hmExtHook> hooks;
        int hookPattern(const std::string &name, const std::string &pattern, const std::string &function);
        int hookPattern(const std::string &name, const std::string &pattern, int (*func)(hmExtension&,hmExtHook,std::smatch));
        int unhookPattern(const std::string &name);
        hmConVar* createConVar(const std::string &name, const std::string &defaultValue, const std::string &description = "", short flags = 0, bool hasMin = false, float min = 0.0, bool hasMax = false, float max = 0.0);
        int hookConVarChange(hmConVar *cvar, const std::string &function);
        int hookConVarChange(hmConVar *cvar, int (*func)(hmConVar&,std::string,std::string));
        int totalCvars();
        // starts a timer
        int createTimer(const std::string &name, long interval, const std::string &function, const std::string &args = "", short type = SECONDS);
        int createTimer(const std::string &name, long interval, int (*func)(hmExtension&,std::string), const std::string &args = "", short type = SECONDS);
        
        // kills a timer by name
        int killTimer(const std::string &name);
        
        // set all timers with 'name' to be triggered on the following tick - returns total number of triggered timers
        int triggerTimer(const std::string &name);
        
        // returns a timer by name
        bool findTimer(const std::string &name, hmExtTimer &timer);
        
        std::vector<hmExtTimer> timers;
        bool invalidTimers;
    private:
        std::vector<std::string> conVarNames;
        bool loaded;
        hmInfo info;
        void *module;
        std::string API_VER;
        std::string modulePath;
        std::string extensionName;
};


int mcVerInt(std::string version);
void mkdirIf(const char *path);
std::string stripFormat(const std::string &str);
hmGlobal *recallGlobal(hmGlobal *global);
void hmSendRaw(std::string raw, bool output = true);
void hmServerCommand(std::string raw, bool output = true);
void hmReplyToClient(const std::string &client, const std::string &message);
void hmReplyToClient(const hmPlayer &client, const std::string &message);
void hmSendCommandFeedback(const std::string &client, const std::string &message);
void hmSendCommandFeedback(const hmPlayer &client, const std::string &message);
void hmSendMessageAll(const std::string &message);
bool hmIsPlayerOnline(std::string client);
hmPlayer hmGetPlayerInfo(std::string client);
std::unordered_map<std::string,hmPlayer>::iterator hmGetPlayerIterator(std::string client);
hmPlayer* hmGetPlayerPtr(std::string client);
std::string hmGetPlayerUUID(std::string client);
std::string hmGetPlayerIP(std::string client);
int hmGetPlayerFlags(std::string client);
void hmOutQuiet(const std::string &text);
void hmOutVerbose(const std::string &text);
void hmOutDebug(const std::string &text);
hmPlayer hmGetPlayerData(std::string client);
void hmLog(std::string data, int logType, std::string logFile);
int hmProcessTargets(std::string client, std::string target, std::vector<hmPlayer> &targList, int filter);
int hmProcessTargetsPtr(std::string client, std::string target, std::vector<hmPlayer*> &targList, int filter);
int hmProcessTargets(const hmPlayer &client, const std::string &target, std::vector<hmPlayer> &targList, int filter);
int hmProcessTargetsPtr(const hmPlayer &client, const std::string &target, std::vector<hmPlayer*> &targList, int filter);
bool hmIsPluginLoaded(const std::string &nameOrPath, const std::string &version = "");
std::string hmGetPluginVersion(const std::string &nameOrPath);
int hmAddConsoleFilter(const std::string &name, const std::string &ptrn, short blocking = HM_BLOCK_OUTPUT, int event = 0);
int hmRemoveConsoleFilter(const std::string &name);
int hmWritePlayerDat(std::string client, const std::string &data, const std::string &ignore, bool ifNotPresent = false);
hmConVar *hmFindConVar(const std::string &name);
std::unordered_map<std::string,hmConVar>::iterator hmFindConVarIt(const std::string &name);
int hmResolveFlag(char flag);
int hmResolveFlags(const std::string &flags);

#endif

