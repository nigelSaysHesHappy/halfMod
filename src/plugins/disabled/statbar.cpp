#include <iostream>
#include <ctime>
#include <cmath>
#include "halfmod.h"
#include "str_tok.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <sys/vtimes.h>
using namespace std;

#define VERSION "v0.1.2"

//static clock_t lastCPU, lastSysCPU, lastUserCPU;
//static int numProcessors;

void popGraph(int value);
string amtTime2(/*love you*/long times);
/*
void init(){
    FILE* file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while(fgets(line, 128, file) != NULL){
        if (strncmp(line, "processor", 9) == 0) numProcessors++;
    }
    fclose(file);
    if (numProcessors == 0)
        numProcessors = 1;
}

double getCurrentValue(){
    struct tms timeSample;
    clock_t now;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        percent = (timeSample.tms_stime - lastSysCPU) +
            (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        //percent /= numProcessors;
        percent *= 100;
    }
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return percent;
}

// returns the average total CPU usage since the last call or since boot if the first call
double getCPUSince()
{
    FILE* file;
    char line[128];
    static long pastCPU = 0;
    long presentCPU;
    static time_t pastTime = 0;
    time_t curTime = time(NULL);
    double percent;
    file = fopen("/proc/stat", "r");
    smatch ml;
    regex ptrn ("cpu\\s+[0-9]+ [0-9]+ [0-9]+ ([0-9]+) ");
    string buf;
    while (fgets(line,128,file) != NULL)
    {
        buf = line;
        if (regex_search(buf,ml,ptrn))
        {
            presentCPU = stol(ml[1].str());
            if (pastTime == 0)
            {
                FILE* file2;
                file2 = fopen("/proc/uptime", "r");
                fgets(line,128,file2);
                fclose(file2);
                pastTime = curTime;
                pastTime -= stol(string(line));
            }
            break;
        }
    }
    fclose(file);
    if (curTime == pastTime)
        percent = 100.0-(double(presentCPU-pastCPU)/double(numProcessors));
    else
        percent = 100.0-(double(presentCPU-pastCPU)/double(numProcessors*(curTime-pastTime)));
    pastCPU = presentCPU;
    pastTime = curTime;
    return percent;
}

long getTotalMemoryFree(long &totalMem)
{
    FILE* file;
    long freeMem;
    char line[128];
    smatch ml;
    regex ptrnA ("MemTotal:\\s+([0-9]+) .*");
    regex ptrnB ("MemFree:\\s+([0-9]+) .*");
    file = fopen("/proc/meminfo", "r");
    string buf;
    while (fgets(line,128,file) != NULL)
    {
        buf = line;
        if (regex_search(buf,ml,ptrnA))
            totalMem = stol(ml[1].str());
        else if (regex_search(buf,ml,ptrnB))
            freeMem = stol(ml[1].str());
    }
    fclose(file);
    return freeMem;
}

long getUptime()
{
    FILE* file;
    char line[64];
    file = fopen("/proc/uptime", "r");
    fgets(line,64,file);
    fclose(file);
    return stol(string(line));
}
*/
struct graphTable
{
    time_t cTime;
    int value;
    int x;
    int y;
};

vector<graphTable> graph;

#define DISPLAY_GRAPH       1
#define DISPLAY_LIST        2
#define STAT_TOTALMEM       1
#define STAT_USEDMEM        2
#define STAT_FREEMEM        4
#define STAT_HMMEM          8
#define STAT_MCMEM          16
#define STAT_TOTALCPU       32
#define STAT_HMCPU          64
#define STAT_MCCPU          128
#define STAT_UPTIME         256

static bool enabled = false;
static int updateTime = 5, oldUpdateTime = 5;
static int displayStats = STAT_TOTALCPU|STAT_TOTALMEM|STAT_FREEMEM;
static int displayMode = DISPLAY_LIST;
static string displaySlot = "sidebar";
static int MCPID = -1;

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(  "Graphs",
                        "nigel",
                        "Line graphs in the sidebar.",
                        VERSION,
                        "http://data.justca.me/on/a/graph" );
    //handle.createTimer("update",updateTime,"popStatBar","");
    //handle.regAdminCmd("hm_graphtime","setUpdateTime",FLAG_CHAT);
    handle.regAdminCmd("hm_statbar","comStatBar",FLAG_CHAT);
    //handle.regConsoleCmd("hm_graphpoint","addPoint");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
        if (it->getExtension() == "sysinfo")
            return 0;
    return 1;
}

int onHShellConnect(hmHandle &handle, rens::smatch args)
{
    static int (*getMCPID)();
    static bool loadfuncs = true;
    if (loadfuncs)
    {
        hmGlobal *global = recallGlobal(NULL);
        for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
        {
            if (it->getExtension() == "sysinfo")
            {
                *(void **) (&getMCPID) = it->getFunc("getMCPID");
                break;
            }
        }
        loadfuncs = false;
    }
    MCPID = (*getMCPID)();
    hmOutDebug("MC PID: " + to_string(MCPID));
    return 0;
}

int onHShellDisconnect(hmHandle &handle)
{
    MCPID = -1;
    return 0;
}

void onPluginStop(hmHandle &handle)
{
    graph.clear();
}

int onWorldInit(hmHandle &handle, rens::smatch args)
{
    hmSendRaw("scoreboard objectives remove hmStatBar\nscoreboard objectives add hmStatBar dummy \"Server Statistics\"");
    return 0;
}

int comStatBar(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " on|off - Enable or disable the statbar.");
        hmReplyToClient(client,"Usage: " + args[0] + " timer <N> - How many seconds between updating the information.");
        hmReplyToClient(client,"Usage: " + args[0] + " displayteam [color] - Define a team to display the statbar for. Empty team will display to everyone.");
        hmReplyToClient(client,"Usage: " + args[0] + " displaytype <graph|list> - Graph can only display one stat while list can have many.");
        hmReplyToClient(client,"Usage: " + args[0] + " addstat <totalmemory|usedmemory|freememory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime>");
        hmReplyToClient(client,"Usage: " + args[0] + " removestat <totalmemory|usedmemory|freememory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime>");
        return 1;
    }
    static int STATS[] = {1,2,4,8,16,32,64,128,256};
    args[1] = lower(args[1]);
    if (args[1] == "on")
    {
        enabled = true;
        hmTimer foo;
        if (handle.findTimer("update",foo))
            handle.triggerTimer("update");
        else
            handle.createTimer("update",updateTime,"popStatBar","");
        hmSendRaw("scoreboard objectives setdisplay " + displaySlot + " hmStatBar");
        hmReplyToClient(client,"The statbar is now enabled.");
        return 0;
    }
    if (args[1] == "off")
    {
        enabled = false;
        handle.killTimer("update");
        hmSendRaw("scoreboard objectives setdisplay " + displaySlot);
        hmReplyToClient(client,"The statbar is now disabled.");
        return 0;
    }
    if (args[1] == "timer")
    {
        if ((argc < 3) || (!stringisnum(args[2],1)))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " timer <N> - How many seconds between updating the information.");
            return 1;
        }
        updateTime = stoi(args[2]);
        hmReplyToClient(client,"The statbar will update every " + data2str("%i",updateTime) + " second(s).");
        return 0;
    }
    if (args[1] == "displayteam")
    {
        if ((argc > 2) && (!istok("black|dark_blue|dark_green|dark_aqua|dark_red|dark_purple|gold|gray|dark_gray|blue|green|aqua|red|light_purple|yellow|white",args[2],"|")))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " displayteam [black|dark_blue|dark_green|dark_aqua|dark_red|dark_purple|gold|gray|dark_gray|blue|green|aqua|red|light_purple|yellow|white] - Empty team will display to everyone.");
            return 1;
        }
        if (argc < 3)
        {
            if (enabled)
                hmSendRaw("scoreboard objectives setdisplay " + displaySlot + "\nscoreboard objectives setdisplay sidebar hmStatBar");
            displaySlot = "sidebar";
        }
        else
        {
            if (enabled)
                hmSendRaw("scoreboard objectives setdisplay " + displaySlot);
            displaySlot = "sidebar.team." + lower(args[2]);
            if (enabled)
                hmSendRaw("scoreboard objectives setdisplay " + displaySlot + " hmStatBar");
        }
        hmReplyToClient(client,"The statbar is now set to display on the " + displaySlot + " slot.");
        return 0;
    }
    if (args[1] == "displaytype")
    {
        if ((argc < 3) || (!istok("graph|list",args[2],"|")))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " displaytype <graph|list> - Graph can only display one stat while list can have many.");
            return 1;
        }
        args[2] = lower(args[2]);
        if (args[2] == "graph")
        {
            displayMode = DISPLAY_GRAPH;
            for (int i = 0;i < 7;i++)
                if ((displayStats & STATS[i]) > 0)
                    displayStats = STATS[i];
            int stat = displayStats;
            if (displayStats == 256) stat = 9;
            else if (displayStats == 128) stat = 8;
            else if (displayStats == 64) stat = 7;
            else if (displayStats == 32) stat = 6;
            else if (displayStats == 16) stat = 5;
            else if (displayStats == 8) stat = 4;
            else if (displayStats == 4) stat = 3;
            hmSendRaw("scoreboard objectives remove hmStatBar\nscoreboard objectives add hmStatBar dummy " + gettok("Total Memory|Used Memory %|Free Memory %|halfMod Memory % (Used)|Minecraft Memory % (Used)|Total CPU Utilization|halfMod CPU Utilization|Minecraft CPU Utilization|Server Uptime",stat,"|"));
            if (enabled)
                hmSendRaw("scoreboard objectives setdisplay " + displaySlot + " hmStatBar");
            hmReplyToClient(client,"The statbar is now set to display the stat as a graph.");
        }
        if (args[2] == "list")
        {
            displayMode = DISPLAY_LIST;
            hmSendRaw("scoreboard objectives remove hmStatBar\nscoreboard objectives add hmStatBar dummy Server Statistics");
            if (enabled)
                hmSendRaw("scoreboard objectives setdisplay " + displaySlot + " hmStatBar");
            hmReplyToClient(client,"The statbar is now set to display the stats as a list.");
        }
    }
    if (args[1] == "addstat")
    {
        if ((argc < 3) || (!istok("totalmemory|usedmemory|freememory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime",args[2],"|")))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " addstat <totalmemory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime>");
            return 1;
        }
        args[2] = lower(args[2]);
        int stat = findtok("totalmemory|usedmemory|freememory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime",args[2],1,"|");
        if (displayMode == DISPLAY_GRAPH)
        {
            displayStats = STATS[stat-1];
            hmSendRaw("scoreboard objectives remove hmStatBar\nscoreboard objectives add hmStatBar dummy " + gettok("Total Memory|Used Memory %|Free Memory %|halfMod Memory % (Used)|Minecraft Memory % (Used)|Total CPU Utilization|halfMod CPU Utilization|Minecraft CPU Utilization|Server Uptime",stat,"|") + "\nscoreboard objectives setdisplay " + displaySlot + " hmStatBar");
            hmReplyToClient(client,"The statbar is now set to display the " + args[2] + ".");
        }
        else
        {
            displayStats |= STATS[stat-1];
            hmReplyToClient(client,"The statbar will now also display the " + args[2] + ".");
        }
    }
    if (args[1] == "removestat")
    {
        if (displayMode == DISPLAY_GRAPH)
        {
            hmReplyToClient(client,args[0] + " removestat can only be used when displaying in 'list' mode.");
            return 1;
        }
        if ((argc < 3) || (!istok("totalmemory|usedmemory|freememory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime",args[2],"|")))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " removestat <totalmemory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime>");
            return 1;
        }
        args[2] = lower(args[2]);
        int stat = STATS[findtok("totalmemory|usedmemory|freememory|halfmodmemory|minecraftmemory|totalcpu|halfmodcpu|minecraftcpu|uptime",args[2],1,"|")-1];
        if ((displayStats & stat) > 0)
        {
            displayStats -= stat;
            hmReplyToClient(client,"The statbar will no longer display the " + args[2] + ".");
        }
        else
        {
            hmReplyToClient(client,"The statbar is already not set to display the " + args[2] + ".");
            return 1;
        }
    }
    return 0;
}

/*
int addPoint(hmHandle &handle, string client, string args[], int argc)
{
    graphTable temp = { time(NULL), stoi(args[1]), -1, 0 };
    graph.push_back(temp);
    return 0;
}*/
/*
int setUpdateTime(hmHandle &handle, string client, string args[], int argc)
{
    if ((argc < 2) || (!stringisnum(args[1],0)))
    {
        hmReplyToClient(client,"Usage: " + args[0] + "<N> - N is a valid, positive integer");
        return 1;
    }
    updateTime = stoi(args[1]);
    hmReplyToClient(client,"The graph will populate every " + data2str("%i",updateTime) + " seconds.");
    return 0;
}*/

int popStatBar(hmHandle &handle, string args)
{
    if (!enabled)
        return 1;
    static double (*getCurrentValue)();
    static double (*getCPUSince)();
    static long (*getTotalMemoryFree)(long &totalMem);
    static long (*getUptime)();
    static long (*getCurrentMemUsage)();
    static long (*getPIDMemUsage)(int pid);
    static bool loadfuncs = true;
    if (loadfuncs)
    {
        hmGlobal *global = recallGlobal(NULL);
        for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
        {
            if (it->getExtension() == "sysinfo")
            {
                *(void **) (&getCurrentValue) = it->getFunc("getCurrentValue");
                *(void **) (&getCPUSince) = it->getFunc("getCPUSince");
                *(void **) (&getTotalMemoryFree) = it->getFunc("getTotalMemoryFree");
                *(void **) (&getUptime) = it->getFunc("getUptime");
                *(void **) (&getCurrentMemUsage) = it->getFunc("getCurrentMemUsage");
                *(void **) (&getPIDMemUsage) = it->getFunc("getPIDMemUsage");
                break;
            }
        }
        loadfuncs = false;
    }
    if (displayMode == DISPLAY_GRAPH)
    {
        int stat = 0;
        switch (displayStats)
        {
            case STAT_TOTALMEM:
            {
                long free, total;
                free = (*getTotalMemoryFree)(total);
                stat = int(total);
                break;
            }
            case STAT_USEDMEM:
            {
                long free, total;
                free = (*getTotalMemoryFree)(total);
                stat = int(100.0-((double(free)/double(total))*100.0));
                break;
            }
            case STAT_FREEMEM:
            {
                long free, total;
                free = (*getTotalMemoryFree)(total);
                stat = int(double(free)/double(total)*100.0);
                break;
            }
            case STAT_HMMEM:
            {
                stat = int((*getCurrentMemUsage)());
                break;
            }
            case STAT_MCMEM:
            {
                stat = int((*getPIDMemUsage)(MCPID));
                break;
            }
            case STAT_TOTALCPU:
            {
                stat = int((*getCPUSince)());
                break;
            }
            case STAT_HMCPU:
            {
                stat = int((*getCurrentValue)());
                break;
            }
            case STAT_MCCPU:
            {
                // nothing yet
                break;
            }
            case STAT_UPTIME:
            {
                stat = int((*getUptime)());
                break;
            }
        }
        popGraph(stat);
    }
    else
    {
        hmSendRaw("scoreboard players reset * hmStatBar");
        if ((displayStats & STAT_TOTALMEM) > 0)
        {
            long free, total;
            free = (*getTotalMemoryFree)(total);
            //hmSendRaw("scoreboard players set Memory␣(Used)␣" + data2str("%lu/%lu␣%% hmStatBar %i",total-free,total,int(100.0-((double(free)/double(total))*100.0))));
            //hmSendRaw("scoreboard players set Total§0␣§rMemory hmStatBar " + data2str("%i",int(total)));
            hmSendRaw("scoreboard players set Total␣Memory hmStatBar " + data2str("%i",int(total)));
        }
        if ((displayStats & STAT_USEDMEM) > 0)
        {
            long free, total;
            free = (*getTotalMemoryFree)(total);
            //hmSendRaw("scoreboard players set Memory␣(Used)␣" + data2str("%lu/%lu␣%% hmStatBar %i",total-free,total,int(100.0-((double(free)/double(total))*100.0))));
            //hmSendRaw("scoreboard players set Used§0␣§rMemory§0␣§2" + data2str("%.2f§r%% hmStatBar %i",100.0-((double(free)/double(total))*100.0),int(total-free)));
            hmSendRaw("scoreboard players set Used␣Memory␣" + data2str("%.2f%% hmStatBar %i",100.0-((double(free)/double(total))*100.0),int(total-free)));
        }
        if ((displayStats & STAT_FREEMEM) > 0)
        {
            long free, total;
            free = (*getTotalMemoryFree)(total);
            //hmSendRaw("scoreboard players set Memory␣(Used)␣" + data2str("%lu/%lu␣%% hmStatBar %i",total-free,total,int(100.0-((double(free)/double(total))*100.0))));
            //hmSendRaw("scoreboard players set Free§0␣§rMemory§0␣§2" + data2str("%.2f§r%% hmStatBar %i",double(free)/double(total)*100.0,int(free)));
            hmSendRaw("scoreboard players set Free␣Memory␣" + data2str("%.2f%% hmStatBar %i",double(free)/double(total)*100.0,int(free)));
        }
        if ((displayStats & STAT_HMMEM) > 0)
            hmSendRaw("scoreboard players set halfMod␣Memory␣Usage hmStatBar " + to_string((*getCurrentMemUsage)()));
        if ((displayStats & STAT_MCMEM) > 0)
            hmSendRaw("scoreboard players set Minecraft␣Memory␣Usage hmStatBar " + to_string((*getPIDMemUsage)(MCPID)));
        if ((displayStats & STAT_TOTALCPU) > 0)
            //hmSendRaw("scoreboard players set Total§0␣§rCPU§0␣§r% hmStatBar " + data2str("%i",int((*getCPUSince)())));
            hmSendRaw("scoreboard players set Total␣CPU␣% hmStatBar " + data2str("%i",int((*getCPUSince)())));
        if ((displayStats & STAT_HMCPU) > 0)
            //hmSendRaw("scoreboard players set halfMod§0␣§rCPU§0␣§r% hmStatBar " + data2str("%i",int((*getCurrentValue)())));
            hmSendRaw("scoreboard players set halfMod␣CPU␣% hmStatBar " + data2str("%i",int((*getCurrentValue)())));
        if ((displayStats & STAT_UPTIME) > 0)
            //hmSendRaw("scoreboard players set Uptime§0␣§r" + amtTime2((*getUptime)()) + " hmStatBar -1");
            hmSendRaw("scoreboard players set Uptime␣" + amtTime2((*getUptime)()) + " hmStatBar -1");
    }
    if (oldUpdateTime != updateTime)
    {
        oldUpdateTime = updateTime;
        handle.createTimer("update",updateTime,"popStatBar","");
        return 1;
    }
    else
        return 0;
}

}

void popGraph(int value)
{
    time_t now = time(NULL);
    graph.push_back({now,value,-1,0});
    int max = 0, scale = 0;
    for (auto it = graph.begin();it != graph.end();)
    {
        if ((max == 0) || (it->value > max))
            max = it->value;
        //it->x++;
        it->x = ((now-it->cTime)/updateTime);
        if (it->x > 38)
            it = graph.erase(it);
        else
            ++it;
    }
    if (max > 14)//112
        max += (14-(max % 14));
    else
        max = 14;
    scale = max/14;
    bool points[39][15] = {false};
    int last[2] = {0};
    float slope, round;
    int x;
    bool first = true;
    for (auto it = graph.rbegin(), ite = graph.rend();it != ite;++it)
    {
        it->y = it->value/scale;
        round = float(it->value)/float(scale);
        if (round-it->y >= 0.5)
            it->y++;
        //points[it->x][it->y] = true;
        if (first)
        {
            last[0] = it->x;
            last[1] = it->y;
            first = false;
        }
        slope = float(it->y-last[1])/(float(it->x-last[0]) == 0.0 ? 1.0 : float(it->x-last[0]));
        x = it->x;
        for (float y = it->y;x >= last[0];x--)
        {
            points[x][int(y)] = true;
            for (int step = int(y), end = int(y-slope);;)
            {
                if (step < end)
                    step++;
                else if (step != end)
                    step--;
                if ((step == end) || (step > 14) || (step < 0))
                    break;
                points[x][step] = true;
            }
            y-=slope;
        }
        last[0] = it->x;
        last[1] = it->y;
    }
    string lines[15];
    char STARTC[15] = { 'P', 'O', 'N', 'M', 'L', 'K', 'J', 'H', 'G', 'F', 'E', 'D', 'C', 'B', 'A' };
    hmSendRaw("scoreboard players reset * hmStatBar");
    for (int y = 0;y < 15;y++)
    {
        lines[y] = STARTC[y];
        for (int x = 0;x < 39;x++)
        {
            if (points[x][y])
                lines[y] += "▓";
            else
                lines[y] += "▒";
        }
        hmSendRaw("scoreboard players set " + lines[y] + " hmStatBar " + data2str("%i",y*scale));
    }
}

string amtTime2(long times)
{
    if (times == 0)
        return "0secs";
    string out = "";
    if (times >= 31536000)
    {
        out = data2str("%liyr",times/31536000);
        if (times >= 63072000)
            out += "s";
        times %= 31536000;
    }
    if (times >= 86400)
    {
        out += data2str("%liday",times/86400);
        if (times >= 172800)
            out += "s";
        times %= 86400;
    }
    if (times >= 3600)
    {
        out += data2str("%lihr",times/3600);
        if (times >= 7200)
            out += "s";
        times %= 3600;
    }
    if (times >= 60)
    {
        out += data2str("%limin",times/60);
        if (times >= 120)
            out += "s";
        times %= 60;
    }
    if (times > 0)
    {
        out += data2str("%lisec",times);
        if (times > 1)
            out += "s";
    }
    return out;
}

