#include <iostream>
#include <string>
#include <regex>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION		"v0.0.7"

int reservedSlots = 1;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string increaseButton(string name, int socket, string ip, string client)
{
    if (reservedSlots >= recallGlobal(NULL)->maxPlayers)
    {
        return "The number of reserved slots is already at the max player limit . . .";
    }
    hmFindConVar("reserved_slots")->setInt(++reservedSlots);
    return "Set the number of reserved slots to " + to_string(reservedSlots) + " . . .";
}

string decreaseButton(string name, int socket, string ip, string client)
{
    if (reservedSlots < 1)
    {
        return "The number of reserved slots is already 0 . . .";
    }
    hmFindConVar("reserved_slots")->setInt(--reservedSlots);
    return "Set the number of reserved slots to " + to_string(reservedSlots) + " . . .";
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(	"Reserved Slots",
                        "nigel",
                        "Always keeps at least one player slot available for players with the RSVP flag.",
                        VERSION,
                        "http://reservations.justca.me/in/your/slot"    );
    handle.hookConVarChange(handle.createConVar("reserved_slots","1","Set the number of reserved slots.",FCVAR_NOTIFY,true,0.0),"slotCom");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            (*addConfigButtonCallback)("increaseSlots","Increase Rsvp",FLAG_CVAR,&increaseButton);
            (*addConfigButtonCallback)("decreaseSlots","Decrease Rsvp",FLAG_CVAR,&decreaseButton);
        }
    }
    //cout<<"poop\n";
    return 0;
}

int onPlayerJoin(hmHandle &handle, smatch args)
{
    hmGlobal *global;
    global = recallGlobal(global);
    string client = args[1].str();
    if (global->maxPlayers - global->players.size() <= reservedSlots)
    {
        if ((hmGetPlayerFlags(client) & FLAG_RSVP) > 0)
        {
            for (auto it = global->players.rbegin(), ite = global->players.rend();it != ite;++it)
            {
                if ((it->flags & FLAG_RSVP) == 0)
                {
                    hmSendRaw("kick " + stripFormat(it->name) + " Kicked for slot reservation.");
                    break;
                }
            }
        }
        else
            hmSendRaw("kick " + client + " This slot is reserved.");
    }
    return 0;
}

int slotCom(hmConVar &cvar, string oldVar, string newVar)
{
    reservedSlots = cvar.getAsInt();
    return 0;
}

}

