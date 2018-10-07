#include <fstream>
#include <regex>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION		"v0.0.4"

int lateLoadCvars(hmHandle &handle, hmHook hook, smatch args);
bool loadCvars(hmHandle &handle);

bool cvarRulesLoaded;

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo(	"Gamerule Cvars",
                        "nigel",
                        "Define ConVars for configured gamerules",
                        VERSION,
                        "http://gamerules.justca.me/in/your/plugins"    );
    if (global->maxPlayers > 0)
        cvarRulesLoaded = loadCvars(handle);
    else
    {
        handle.hookPattern("cvarLoad","\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: There are ([0-9]{1,})(/| of a max )([0-9]{1,}) players online:\\s*(.*)",&lateLoadCvars);
        cvarRulesLoaded = false;
    }
    return 0;
}

int onWorldInit(hmHandle &handle, rens::smatch args)
{
    cvarRulesLoaded = loadCvars(handle);
    return 0;
}

}

int lateLoadCvars(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    if (!cvarRulesLoaded)
        cvarRulesLoaded = loadCvars(handle);
    return 0;
}

bool loadCvars(hmHandle &handle)
{
    ifstream file ("./halfMod/config/gamerules.conf");
    if (file.is_open())
    {
        string line, name, value, desc, temp, temp1;
        short flags;
        bool hMin, hMax;
        float min, max;
        int i, j;
        std::regex comment ("\\s*#.*");
        std::regex white0 ("$\\s+");
        std::regex white1 ("\\s+");
        while (getline(file,line))
        {
            if ((line.size() < 1) || (std::regex_match(line,comment)))
                continue;
            line = std::regex_replace(line,white0,"");
            line = std::regex_replace(line,white1," ");
            name = getqtok(line,1," ");
            value = getqtok(name,2,"=");
            name = getqtok(name,1,"=");
            desc = "";
            flags = FCVAR_GAMERULE;
            hMin = hMax = false;
            min = max = 0.0;
            for (i = 2, j = numqtok(line," ");i <= j;i++)
            {
                temp = getqtok(line,i," ");
                temp1 = getqtok(temp,1,"=");
                if (temp1 == "desc")
                    desc = getqtok(temp,2,"=");
                else if (temp1 == "min")
                {
                    temp1 = getqtok(temp,2,"=");
                    if (stringisnum(temp1))
                    {
                        hMin = true;
                        min = stoi(temp1);
                    }
                }
                else if (temp1 == "max")
                {
                    temp1 = getqtok(temp,2,"=");
                    if (stringisnum(temp1))
                    {
                        hMax = true;
                        max = stoi(getqtok(temp,2,"="));
                    }
                }
                else if (lower(temp) == "notify")
                    flags |= FCVAR_NOTIFY;
                else if (lower(temp) == "constant")
                    flags |= FCVAR_CONSTANT;
                else if (lower(temp) == "readonly")
                    flags |= FCVAR_READONLY;
            }
            handle.createConVar(name,value,desc,flags,hMin,min,hMax,max);
            if ((flags & FCVAR_CONSTANT) > 0)
                hmSendRaw("gamerule " + name + " " + value);
            else
                hmSendRaw("gamerule " + name);
        }
        file.close();
        return true;
    }
    else
        hmOutDebug("Error: Missing `./halfMod/config/gamerules.conf` file.");
    return false;
}

