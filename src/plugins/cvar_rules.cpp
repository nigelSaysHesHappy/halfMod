#include <fstream>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION		"v0.0.2"

void loadCvars(hmHandle &handle);

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
        loadCvars(handle);
    return 0;
}

int onWorldInit(hmHandle &handle, smatch args)
{
    loadCvars(handle);
    return 0;
}

}

void loadCvars(hmHandle &handle)
{
    ifstream file ("./halfMod/config/gamerules.conf");
    if (file.is_open())
    {
        string line, name, value, desc, temp, temp1;
        short flags;
        bool hMin, hMax;
        float min, max;
        int i, j;
        while (getline(file,line))
        {
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
                    hMin = true;
                    min = stof(getqtok(temp,2,"="));
                }
                else if (temp1 == "max")
                {
                    hMax = true;
                    max = stof(getqtok(temp,2,"="));
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
            hmSendRaw("gamerule " + name);
        }
        file.close();
    }
    else
        hmOutDebug("Error: Missing `./halfMod/config/gamerules.conf` file.");
}

