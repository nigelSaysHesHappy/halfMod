#include "halfmod.h"
#include "str_tok.h"
#include <fstream>
using namespace std;

#define VERSION "v0.2.5"

bool enabled = false;
long interval = 1000;
hmConVar *cDiscoTime;
vector<string> readyPlayers;
#define MAXMUSIC    11
const string music[MAXMUSIC] = {
    "record.blocks",
    "record.cat",
    "record.chirp",
    "record.far",
    "record.mall",
    "record.mellohi",
    "record.stal",
    "record.strad",
    "record.wait",
    "record.ward",
    "music.credits"
};

void handlePlayer(hmHandle &handle, unordered_map<std::string,hmPlayer>::iterator it);

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string toggleButton(string name, int socket, string ip, string client)
{
    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> !disco");
    if (!enabled)
        return "Disco mode is now enabled . . .";
    return "Disco mode is now disabled . . .";
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    int ver = mcVerInt(global->mcVer);
    if ((ver < 113000) || ((ver >= 10000000) && (ver < 17005001)))
    {
        hmOutDebug("Error: Minimum Minecraft version for \"disco\" plugin is 1.13 official or snapshot 17w50a");
        return 1;
    }
    handle.pluginInfo("Disco",
                      "nigel",
                      "Boogie on down to disco town!",
                      VERSION,
                      "http://this.disco.train.justca.me/to/town");
    handle.regAdminCmd("hm_disco","toggleDisco",FLAG_INVENTORY,"Toggle disco mode");
    cDiscoTime = handle.createConVar("disco_time","1.0","Change the tempo of the disco! In fractional seconds.",0,true,0.0001);
    handle.hookConVarChange(cDiscoTime,"setDiscoTime");
    handle.regAdminCmd("hm_votedisco","voteDisco",FLAG_VOTE,"All aboard the disco train!");
    ifstream file("./halfMod/config/disco.cfg");
    if (file.is_open())
    { // If this file exists; halfMod, or this plugin, ended while disco was enabled
        file.close();
        enabled = true;
        handle.createTimer("discoTime",interval,"discoTime","",MILLISECONDS);
    }
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            (*addConfigButtonCallback)("toggleDisco","Toggle Disco",FLAG_INVENTORY,&toggleButton);
        }
    }
    return 0;
}

int updatePlayers(hmHandle &handle, string args)
{ // This will trigger in the event of a late load if there are already players on the server
    hmGlobal *global = recallGlobal(NULL);
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
        handlePlayer(handle,it);
    return 1;
}

int onHShellConnect(hmHandle &handle, smatch args)
{ // when connecting to halfShell on either startup or a late launch, the `list` command is automatically run.
    if (readyPlayers.size() > 0)
        readyPlayers.clear();
    if (recallGlobal(NULL)->players.size() > 0) // Late plugin load
        updatePlayers(handle,"");
    else                                        // Late halfMod load
        handle.hookPattern("listCheck","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: There are ([0-9]{1,}) of a max [0-9]{1,} players online:\\s*(.*)$","listCheck");
    return 0;
}

int listCheck(hmHandle &handle, hmHook hook, smatch args)
{
    int online = stoi(args[1].str());
    if (online > 0)                                            // Hooks are processed before internal triggers and events, timers are processed at the very end of each tick
        handle.createTimer("updatePlayers",0,"updatePlayers"); // By creating a timer with an interval of 0, we allow halfMod to process the output from `list` and populate
    handle.unhookPattern(hook.name);                           // the player list before continuing.
    return 0;
}

int onRehashFilter(hmHandle &handle)
{
    if (enabled)
        hmAddConsoleFilter("disco","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Replaced a slot on [^ ]+ with \\[Leather (Boots|Pants|Tunic|Cap)\\]$");
    return 0;
}

int onPlayerJoin(hmHandle &handle, smatch args)
{
    handlePlayer(handle,hmGetPlayerIterator(args[1].str()));
    return 0;
}

int onPlayerDisconnect(hmHandle &handle, smatch args)
{
    if (enabled)
    {
        string client = stripFormat(lower(args[1].str()));
        for (auto it = readyPlayers.begin(), ite = readyPlayers.end();it != ite;++it)
        {
            if (*it == client)
            {
                readyPlayers.erase(it);
                break;
            }
        }
    }
    return 0;
}

int setDiscoTime(hmConVar &cvar, string oldVal, string newVal)
{
    interval = cvar.getAsFloat()*1000.0;
    return 0;
}

int toggleDisco(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    hmGlobal *global = recallGlobal(NULL);
    string lines, line;
    if (enabled)
        hmSendRaw("replaceitem entity @a armor.feet minecraft:air\nreplaceitem entity @a armor.legs minecraft:air\nreplaceitem entity @a armor.chest minecraft:air\nreplaceitem entity @a armor.head minecraft:air");
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
    {
        if (!enabled)
        {
            handle.hookPattern("getArmor:" + it->first,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + it->second.name + " has the following entity data: \\{.*Inventory: \\[(.*)\\].*\\}$","getArmor");
            hmSendRaw("data get entity " + it->first);
        }
        else
        {
            for (int i = 1;(line = gettok(it->second.custom,i,"\n")) != "";i++)
            {
                if (gettok(line,1,"=") == "disco")
                {
                    if (line != "disco=null")
                        hmSendRaw("execute at " + it->first + " run summon minecraft:zombie ~ ~ ~ {ArmorItems:[" + deltok(line,1,"=") + "],ArmorDropChances:[2.0f,2.0f,2.0f,2.0f],NoAI:1b,Health:0.1f,Fire:300s,Silent:1}");
                    it->second.custom = deltok(it->second.custom,i,"\n");
                    hmWritePlayerDat(it->first,"","disco");
                    break;
                }
            }
        }
    }
    enabled=!enabled;
    if (enabled)
    {
        readyPlayers.clear();
        ofstream file("./halfMod/config/disco.cfg",iostream::trunc);
        if (file.is_open())
        {
            file<<"enabled";
            file.close();
        }
        handle.createTimer("discoTime",interval,"discoTime","",MILLISECONDS);
        hmAddConsoleFilter("disco","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Replaced a slot on [^ ]+ with \\[Leather (Boots|Pants|Tunic|Cap)\\]$");
        hmSendRaw("execute as @a at @s run playsound minecraft:" + music[randint(0,MAXMUSIC)] + " record @s ~ ~ ~ 100");
        hmSendMessageAll("All aboard the disco train!");
    }
    else
    {
        readyPlayers.clear();
        remove("./halfMod/config/disco.cfg");
        handle.killTimer("discoTime");
        hmServerCommand("hm_rehashfilter");
        hmSendMessageAll("Time to shut this disco down :(");
    }
    return 0;
}

int getArmor(hmHandle &handle, hmHook hook, smatch args)
{
    string inv = args[1].str(), stripClient = gettok(hook.name,2,":");
    regex ptrn ("\\{Slot: (100b|101b|102b|103b)");
    smatch ml;
    hmPlayer *ptr = hmGetPlayerPtr(stripClient);
    if (regex_search(inv,ml,ptrn))
    {
        hmSendRaw("replaceitem entity " + stripClient + " armor.feet minecraft:air\nreplaceitem entity " + stripClient + " armor.legs minecraft:air\nreplaceitem entity " + stripClient + " armor.chest minecraft:air\nreplaceitem entity " + stripClient + " armor.head minecraft:air");
        inv = "{Slot: " + ml[1].str() + ml.suffix().str();
        ptrn = ", \\{Slot: -106b";
        if (regex_search(inv,ml,ptrn))
            inv = ml.prefix().str();
        ptrn = "\\{Slot: [0-9]+b, ?";
        inv = "disco=" + regex_replace(inv,ptrn,"{");
        if (hmWritePlayerDat(stripClient,inv,"disco",true) > -1)
            ptr->custom = appendtok(ptr->custom,inv,"\n");
    }
    else
    {
        hmWritePlayerDat(stripClient,"disco=null","disco",true);
        ptr->custom = appendtok(ptr->custom,"disco=null","\n");
    }
    readyPlayers.push_back(stripClient);
    handle.unhookPattern(hook.name);
    return 1;
}

int discoTime(hmHandle &handle, string args)
{
    static long oldInt = interval;
    if (!enabled)
        return 1;
    string randColor;
    for (auto it = readyPlayers.begin(), ite = readyPlayers.end();it != ite;++it)
    {
        randColor = to_string(int(randint(0,1932928) * 1111 % 16777215));
        hmSendRaw("replaceitem entity " + *it + " armor.feet minecraft:leather_boots{ench:[{lvl:1s,id:10s},{lvl:1s,id:71s}],display:{Lore:[\"Disco!\"],color:" + randColor + "}}");
        hmSendRaw("replaceitem entity " + *it + " armor.legs minecraft:leather_leggings{ench:[{lvl:1s,id:10s},{lvl:1s,id:71s}],display:{Lore:[\"Disco!\"],color:" + randColor + "}}");
        hmSendRaw("replaceitem entity " + *it + " armor.chest minecraft:leather_chestplate{ench:[{lvl:1s,id:10s},{lvl:1s,id:71s}],display:{Lore:[\"Disco!\"],color:" + randColor + "}}");
        hmSendRaw("replaceitem entity " + *it + " armor.head minecraft:leather_helmet{ench:[{lvl:1s,id:10s},{lvl:1s,id:71s}],display:{Lore:[\"Disco!\"],color:" + randColor + "}}");
    }
    if (oldInt != interval)
    {
        oldInt = interval;
        handle.createTimer("discoTime",interval,"discoTime","",MILLISECONDS);
        return 1;
    }
    return 0;
}

int voteDisco(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (!hmIsPluginLoaded("vote"))
        hmReplyToClient(client,"Error: `vote' plugin is not loaded!");
    else
    {
        if (enabled)
            hmServerCommand("hm_voterun hm_disco \"Have we had enough of the disco?\" \"Yes\" \"Never.\"");
        else
            hmServerCommand("hm_voterun hm_disco \"What time is it?\" \"Disco time!\" \"Not disco time :(\"");
    }
    return 0;
}

}

void handlePlayer(hmHandle &handle, unordered_map<std::string,hmPlayer>::iterator it)
{
    string line;
    bool exists = false;
    for (int i = 1;(line = gettok(it->second.custom,i,"\n")) != "";i++)
    {
        if (gettok(line,1,"=") == "disco")
        {
            exists = true;
            if (!enabled)
            {
                hmSendRaw("replaceitem entity " + it->first + " armor.feet minecraft:air\nreplaceitem entity " + it->first + " armor.legs minecraft:air\nreplaceitem entity " + it->first + " armor.chest minecraft:air\nreplaceitem entity " + it->first + " armor.head minecraft:air");
                if (line != "disco=null")
                    hmSendRaw("execute at " + it->first + " run summon minecraft:zombie ~ ~ ~ {ArmorItems:[" + deltok(line,1,"=") + "],ArmorDropChances:[2.0f,2.0f,2.0f,2.0f],NoAI:1b,Health:0.1f,Fire:300s,Silent:1}");
                it->second.custom = deltok(it->second.custom,i,"\n");
                hmWritePlayerDat(it->first,"","disco");
            }
            else readyPlayers.push_back(it->first);
            break;
        }
    }
    if ((!exists) && (enabled))
    {
        handle.hookPattern("getArmor:" + it->first,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + it->second.name + " has the following entity data: \\{.*Inventory: \\[(.*)\\].*\\}$","getArmor");
        hmSendRaw("data get entity " + it->first);
    }
}

