#include "halfmod.h"
#include <fstream>
#include <iostream>
#include "str_tok.h"

#define VERSION "v0.0.6"

int benchmarkCvar(hmConVar &cvar, std::string oldVar, std::string newVar);
int debugStart(hmHandle &handle, hmHook hook, rens::smatch args);
int debugStop(hmHandle &handle, hmHook hook, rens::smatch args);

int benchForCmd(hmHandle &handle, const hmPlayer &client, std::string args[], int argc);
int benchForSec(hmHandle &handle, const hmPlayer &client, std::string args[], int argc);

int benchmarkDefault;
bool benchmarkRunning;

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Benchmark",
                      "nigel",
                      "Benchmark Command Performance",
                      VERSION,
                      "http://who.justca.me");
    handle.hookConVarChange(handle.createConVar("benchmark_default","200","Default number of times to run a command with hm_benchfor.",FCVAR_NOTIFY,true,0.0),&benchmarkCvar);
    handle.hookPattern("debugStart","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Started debug profiling$",&debugStart);
    handle.hookPattern("debugStop","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Stopped debug profiling after ([^ ]+) seconds and ([^ ]+) ticks \\(([^ ]+) ticks per second\\)$",&debugStop);
    handle.regAdminCmd("hm_benchfor",&benchForCmd,FLAG_RCON,"Benchmark running command N times.");
    handle.regAdminCmd("hm_benchtime",&benchForSec,FLAG_RCON,"Benchmark for N seconds.");
    benchmarkDefault = 200;
    benchmarkRunning = false;
    return 0;
}

int benchmarkCvar(hmConVar &cvar, std::string oldVar, std::string newVar)
{
    benchmarkDefault = cvar.getAsInt();
    return 0;
}

int stopDebugTPS(hmHandle &handle, std::string args)
{
    hmSendRaw("debug stop");
    return 1;
}

int debugStart(hmHandle &handle, hmHook hook, rens::smatch args)
{
    benchmarkRunning = true;
    if (hook.name.substr(0,9) == "debugTime")
    {
        std::string s = hook.name.substr(9);
        int seconds = std::stoi(s);
        handle.createTimer("debugTimer",seconds,&stopDebugTPS);
    }
    return 0;
}

int debugStop(hmHandle &handle, hmHook hook, rens::smatch args)
{
    benchmarkRunning = false;
    if (hook.name == "debugStop")
        return 0;
    std::string seconds = args[1].str(), ticks = args[2].str(), tps = args[3].str();
    if (hook.name.substr(0,9) == "debugTime")
    {
        std::string s = hook.name.substr(9);
        hmSendRaw("say Requested " + s + " seconds; ran for " + seconds + " seconds and " + ticks + " ticks (" + tps + " tps)");
    }
    else
        hmSendRaw("say It took " + seconds + " seconds and " + ticks + " ticks (" + tps + " tps) to run `" + strreplace(strreplace(gettok(hook.name,2,"\n"),"@","#"),"\\n","\\\\n") + "` " + gettok(hook.name,1,"\n") + " times.");
    handle.unhookPattern(hook.name);
    return 0;
}

int benchForCmd(hmHandle &handle, const hmPlayer &client, std::string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <command> [quantity]");
        return 1;
    }
    int quantity = benchmarkDefault;
    if ((argc > 2) && (isdigit(args[2][0])))
        quantity = std::stoi(args[2]);
    if (benchmarkRunning)
    {
        hmReplyToClient(client,"A benchmark is already running!");
        return 1;
    }
    handle.hookPattern(std::to_string(quantity) + "\n" + args[1],"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Stopped debug profiling after ([^ ]+) seconds and ([^ ]+) ticks \\(([^ ]+) ticks per second\\)$",&debugStop);
    hmSendRaw("debug start");
    std::string command = strreplace(args[1],"\\n","\n");
    for (;quantity;quantity--)
        hmSendRaw(args[1]);
    hmSendRaw("debug stop");
    return 0;
}

int benchForSec(hmHandle &handle, const hmPlayer &client, std::string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <seconds>");
        return 1;
    }
    handle.hookPattern("debugTime" + args[1],"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Started debug profiling$",&debugStart);
    handle.hookPattern("debugTime" + args[1],"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Stopped debug profiling after ([^ ]+) seconds and ([^ ]+) ticks \\(([^ ]+) ticks per second\\)$",&debugStop);
    hmReplyToClient(client,"Beginning benchmark for " + args[1] + " seconds.");
    hmSendRaw("debug start");
    return 1;
}




