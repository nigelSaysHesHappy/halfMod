#include <iostream>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION		"v0.0.8"

bool skullDrop = true;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string toggleButton(string name, int socket, string ip, string client)
{
    if (skullDrop)
    {
        hmFindConVar("drop_head")->setBool(false);
        return "Players will no longer drop their skull on death . . .";
    }
    hmFindConVar("drop_head")->setBool(true);
    return "Players will now drop their skull on death . . .";
}

int headCmd(hmHandle &handle, string client, string args[], int argc);

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(	"Skull Drop",
                        "nigel",
                        "Players drop their head when they die.",
                        VERSION,
                        "http://you.justca.me/from/head"    );
    handle.hookConVarChange(handle.createConVar("drop_head","true","Enable or disable dropping heads on death.",FCVAR_NOTIFY,true,0.0,true,1.0),"cChange");
    handle.regAdminCmd("hm_givemehead",&headCmd,FLAG_CHEATS,"Get the head of a player");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            (*addConfigButtonCallback)("toggleSkull","Toggle Skull Drops",FLAG_CVAR,&toggleButton);
        }
    }
    return 0;
}

int cChange(hmConVar &cvar, string oldVar, string newVar)
{
    skullDrop = cvar.getAsBool();
    return 0;
}

int onPlayerDeath(hmHandle &handle, smatch args)
{
    if (skullDrop)
    {
        string client = args[1].str();
        hmSendRaw("execute at " + client + " run summon minecraft:item ~ ~ ~ {Item:{id:\"minecraft:player_head\",Count:1,tag:{SkullOwner:\"" + client + "\"}}}");
    }
    return 0;
}

}

int headCmd(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <player>");
        return 1;
    }
    hmSendRaw("give " + client + " minecraft:player_head{SkullOwner:\"" + args[1] + "\"}");
    return 0;
}

