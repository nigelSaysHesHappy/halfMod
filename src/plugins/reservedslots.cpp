#include <iostream>
#include <string>
#include <regex>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION		"v0.0.1"

int reservedSlots = 1;

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(	"Reserved Slots",
                        "nigel",
                        "Always keeps at least one player slot available for players with the RSVP flag.",
                        VERSION,
                        "http://reservations.justca.me/in/your/slot"    );
    handle.regAdminCmd("hm_reservedslots","slotCom",FLAG_CVAR,"Set the number of reserved slots.");
}

int onPlayerConnect(hmHandle &handle, smatch args)
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

int slotCom(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
        hmReplyToClient(client,"Current number of slot(s) to reserve: " + data2str("%i",reservedSlots));
    else
    {
        if (!stringisnum(args[1],0))
            hmReplyToClient(client,"Usage: " + args[1] + " [N] - How many slots to keep reserved. Empty to see current value.");
        else
        {
            reservedSlots = stoi(args[1]);
            hmReplyToClient(client,"Current number of slot(s) to reserve set to " + data2str("%i",reservedSlots));
        }
    }
    return 0;
}

}

