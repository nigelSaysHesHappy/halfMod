#include <fstream>
#include "str_tok.h"
#include "halfmod.h"
#include "nbtmap.h"
using namespace std;

#define VERSION "v0.0.5"

int setPearlTarget(hmHandle &handle, const hmPlayer &client, string args[], int argc);
int uuidLookup(hmHandle &handle, hmHook hook, rens::smatch args);
int catchPearl(hmHandle &handle, hmHook hook, rens::smatch args);

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Almighty Telepearl",
                      "nigel",
                      "Summon others at will with your ender pearls.",
                      VERSION,
                      "http://telepearl.made.you.justca.me/");
    handle.regAdminCmd("hm_telepearl",&setPearlTarget,FLAG_SLAY,"Set your telepearl target");
    handle.hookPattern("pearlTagged","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(.+): Added tag 'hmPearlObj' to Thrown Ender Pearl\\]$",&catchPearl);
    return 0;
}

extern "C" int onWorldInit(hmHandle &handle, rens::smatch args)
{
    hmGlobal *global = recallGlobal(NULL);
    hmSendRaw("scoreboard objectives add hmPearl minecraft.used:minecraft.ender_pearl");
    string path = "./" + strremove(global->world,"\"") + "/datapacks/";
    mkdirIf(path.c_str());
    path += "halfMod/";
    mkdirIf(path.c_str());
    path += "data/";
    mkdirIf(path.c_str());
    mkdirIf(string(path + "minecraft/").c_str());
    mkdirIf(string(path + "minecraft/tags/").c_str());
    mkdirIf(string(path + "minecraft/tags/functions/").c_str());
    mkdirIf(string(path + "halfmod/").c_str());
    mkdirIf(string(path + "halfmod/functions/").c_str());
    fstream file (path + "minecraft/tags/functions/tick.json",ios_base::in);
    string line, lines, str = "{\n        \"values\":[\n                \"halfmod:tick\"        ]}";
    bool reload = false;
    if (file.is_open())
    {
        while (getline(file,line))
            lines = appendtok(lines,line,"\n");
        file.close();
    }
    if (lines != str)
    {
        file.open(path + "minecraft/tags/functions/tick.json",ios_base::out|ios_base::trunc);
        if (file.is_open())
        {
            reload = true;
            file<<str;
            file.close();
        }
    }
    lines.clear();
    //str = "execute if entity @e[type=minecraft:ender_pearl,limit=1,tag=!handled,tag=!hmPearlObj] run function halfmod:telepearl";
    //str = "execute if entity @e[type=minecraft:ender_pearl,limit=1,tag=!handled,tag=!hmPearlObj] run setblock ~12 255 ~12 minecraft:command_block{auto:1b,Command:\"execute as @a[tag=hmPearl,scores={hmPearl=1..}] run tag @e[type=minecraft:ender_pearl,limit=1,sort=nearest,tag=!handled,tag=!hmPearlObj] add hmPearlObj\"} destroy";
    str = "execute unless block ~12 0 ~12 minecraft:repeating_command_block run setblock ~12 0 ~12 minecraft:repeating_command_block{auto:1b,Command:\"execute as @a[tag=hmPearl,scores={hmPearl=1..}] run tag @e[type=minecraft:ender_pearl,limit=1,sort=nearest,tag=!handled,tag=!hmPearlObj] add hmPearlObj\"} replace";
    file.open(path + "halfmod/functions/tick.mcfunction",ios_base::in);
    bool found = false;
    if (file.is_open())
    {
        while (getline(file,line))
        {
            if (line == str)
            {
                found = true;
                break;
            }
        }
        file.close();
    }
    if (!found)
    {
        file.open(path + "halfmod/functions/tick.mcfunction",ios_base::out|ios_base::app);
        if (file.is_open())
        {
            reload = true;
            file<<"\n"<<str;
            file.close();
        }
    }
    if (reload)
        hmSendRaw("reload");
    return 0;
}

int setPearlTarget(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    hmPlayer *dat = hmGetPlayerPtr(client.name);
    string custom;
    for (int i = 0, j = numtok(dat->custom,"\n");i < j;i++)
    {
        string tag = gettok(dat->custom,i+1,"\n");
        if (gettok(tag,1,"=") != "Telepearl Target")
            custom = addtok(custom,tag,"\n");
    }
    dat->custom = custom;
    if (argc > 1)
    {
        vector<hmPlayer> targs;
        int targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
        if (targn == 1)
        {
            string target = targs.begin()->name;
            handle.hookPattern("telepearlLookup " + client.name + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + target + ") has the following entity data: (\\{.*\\})$",&uuidLookup);
            hmSendRaw("data get entity " + target);
        }
        else
        {
            hmSendRaw("tag " + client.name + " remove hmPearl");
            if (targn > 1)
                hmReplyToClient(client,"You can only select one player!");
            else
                hmReplyToClient(client,"Player must be online!");
        }
    }
    else
    {
        hmSendRaw("tag " + client.name + " remove hmPearl");
        hmReplyToClient(client,"Your Telepearl target has been reset.");
    }
    return 0;
}

int uuidLookup(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    string target = args[1].str(), client = gettok(hook.name,2," ");
    NBTCompound nbt = args[2].str();
    string owner = "{L:" + nbt.get("UUIDLeast") + ", M:" + nbt.get("UUIDMost") + "}";
    hmPlayer *dat = hmGetPlayerPtr(client);
    dat->custom = addtok(dat->custom,"Telepearl Target=" + owner,"\n");
    hmSendRaw("tag " + client + " add hmPearl");
    hmReplyToClient(client,"Your ender pearls will now summon " + target);
    return 1;
}

int catchPearl(hmHandle &handle, hmHook hook, rens::smatch args)
{
    string client = args[1].str(), target;
    hmPlayer *dat = hmGetPlayerPtr(client);
    for (int i = 0, j = numtok(dat->custom,"\n");i < j;i++)
    {
        string tag = gettok(dat->custom,i+1,"\n");
        if (gettok(tag,1,"=") == "Telepearl Target")
            target = gettok(tag,2,"=");
    }
    if (target.size() > 0)
        hmSendRaw("execute at " + client + " run data merge entity @e[limit=1,sort=nearest,tag=hmPearlObj,type=minecraft:ender_pearl] {owner:" + target + ",Tags:[\"handled\"]}\nscoreboard players reset " + client + " hmPearl");//\nsetblock ~12 255 ~12 minecraft:air replace");
    else
        hmSendRaw("tag " + client + " remove hmPearl");
    return 0;
}

