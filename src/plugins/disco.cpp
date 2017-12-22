#include "halfmod.h"
#include "str_tok.h"
#include <fstream>
using namespace std;

#define VERSION "v0.1.0"

bool enabled = false;
int interval = 1;
vector<string> readyPlayers;

void handlePlayer(hmHandle &handle, vector<hmPlayer>::iterator it);

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Disco",
                      "nigel",
                      "Boogie on down to disco town!",
                      VERSION,
                      "http://this.disco.train.justca.me/to/town");
    handle.regAdminCmd("hm_disco","toggleDisco",FLAG_INVENTORY,"Toggle disco mode");
    handle.regAdminCmd("hm_disco_time","setDiscoTime",FLAG_INVENTORY,"Change the tempo of the disco! Default is 1");
    handle.regAdminCmd("hm_votedisco","voteDisco",FLAG_VOTE,"All aboard the disco train!");
    ifstream file("./halfMod/config/disco.cfg");
    if (file.is_open())
    { // If this file exists; halfMod, or this plugin, ended while disco was enabled
        file.close();
        enabled = true;
        handle.createTimer("discoTime",interval,"discoTime");
    }
}

int updatePlayers(hmHandle &handle, string args)
{ // This will trigger in the event of a late load if there are already players on the server
    hmGlobal *global = recallGlobal(NULL);
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;it++)
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
        string client = stripFormat(args[1].str());
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

int setDiscoTime(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
        hmReplyToClient(client,"The current tempo of the disco is: " + to_string(interval));
    else
    {
        if (stringisnum(args[1],1))
        {
            interval = stoi(args[1]);
            hmSendCommandFeedback(client,"Set the tempo of the disco to " + to_string(interval));
        }
        else
            hmReplyToClient(client,"Usage: " + args[0] + " <N> - N must be at least 1");
    }
    return 0;
}

int toggleDisco(hmHandle &handle, string client, string args[], int argc)
{
    hmGlobal *global = recallGlobal(NULL);
    string stripClient, lines, line;
    if (enabled)
        hmSendRaw("replaceitem entity @a armor.feet minecraft:air\nreplaceitem entity @a armor.legs minecraft:air\nreplaceitem entity @a armor.chest minecraft:air\nreplaceitem entity @a armor.head minecraft:air");
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;it++)
    {
        stripClient = stripFormat(it->name);
        if (!enabled)
        {
            handle.hookPattern("getArmor:" + stripClient,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + stripClient + " has the following entity data: \\{.*Inventory: \\[(.*)\\].*\\}$","getArmor");
            hmSendRaw("data get entity " + stripClient);
        }
        else
        {
            for (int i = 1;(line = gettok(it->custom,i,"\n")) != "";i++)
            {
                if (gettok(line,1,"=") == "disco")
                {
                    if (line != "disco=null")
                        hmSendRaw("execute at " + stripClient + " run summon minecraft:zombie ~ ~ ~ {ArmorItems:[" + deltok(line,1,"=") + "],ArmorDropChances:[2.0f,2.0f,2.0f,2.0f],NoAI:1b,Health:0.1f,Fire:300s,Silent:1}");
                    it->custom = deltok(it->custom,i,"\n");
                    hmWritePlayerDat(stripClient,"","disco");
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
        handle.createTimer("discoTime",interval,"discoTime");
        hmAddConsoleFilter("disco","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Replaced a slot on [^ ]+ with \\[Leather (Boots|Pants|Tunic|Cap)\\]$");
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
    string inv = args[1].str(), stripClient = stripFormat(gettok(hook.name,2,":"));
    regex ptrn ("\\{Slot: (100b|101b|102b|103b)");
    smatch ml;
    if (regex_search(inv,ml,ptrn))
    {
        hmSendRaw("replaceitem entity " + stripClient + " armor.feet minecraft:air\nreplaceitem entity " + stripClient + " armor.legs minecraft:air\nreplaceitem entity " + stripClient + " armor.chest minecraft:air\nreplaceitem entity " + stripClient + " armor.head minecraft:air");
        inv = "{Slot: " + ml[1].str() + ml.suffix().str();
        ptrn = ", \\{Slot: -106b";
        if (regex_search(inv,ml,ptrn))
            inv = ml.prefix().str();
        ptrn = "\\{Slot: [0-9]+b, ?";
        inv = "disco=" + regex_replace(inv,ptrn,"{");
        auto it = hmGetPlayerIterator(stripClient);
        if (hmWritePlayerDat(stripClient,inv,"disco",true) > -1)
            it->custom = appendtok(it->custom,inv,"\n");
    }
    else
        hmWritePlayerDat(stripClient,"disco=null","disco",true);
    readyPlayers.push_back(stripClient);
    handle.unhookPattern(hook.name);
    return 1;
}

int discoTime(hmHandle &handle, string args)
{
    static int oldInt = interval;
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
        handle.createTimer("discoTime",interval,"discoTime");
        return 1;
    }
    return 0;
}

int voteDisco(hmHandle &handle, string client, string args[], int argc)
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

void handlePlayer(hmHandle &handle, vector<hmPlayer>::iterator it)
{
    string line, stripClient = stripFormat(it->name);
    bool exists = false;
    for (int i = 1;(line = gettok(it->custom,i,"\n")) != "";i++)
    {
        if (gettok(line,1,"=") == "disco")
        {
            exists = true;
            if (!enabled)
            {
                hmSendRaw("replaceitem entity " + stripClient + " armor.feet minecraft:air\nreplaceitem entity " + stripClient + " armor.legs minecraft:air\nreplaceitem entity " + stripClient + " armor.chest minecraft:air\nreplaceitem entity " + stripClient + " armor.head minecraft:air");
                if (line != "disco=null")
                    hmSendRaw("execute at " + stripClient + " run summon minecraft:zombie ~ ~ ~ {ArmorItems:[" + deltok(line,1,"=") + "],ArmorDropChances:[2.0f,2.0f,2.0f,2.0f],NoAI:1b,Health:0.1f,Fire:300s,Silent:1}");
                it->custom = deltok(it->custom,i,"\n");
                hmWritePlayerDat(stripClient,"","disco");
            }
            else readyPlayers.push_back(stripClient);
            break;
        }
    }
    if (!exists)
    {
        handle.hookPattern("getArmor:" + stripClient,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + stripClient + " has the following entity data: \\{.*Inventory: \\[(.*)\\].*\\}$","getArmor");
        hmSendRaw("data get entity " + stripClient);
    }
}

