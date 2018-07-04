#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.0.3"

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Session Flags",
                      "nigel",
                      "Dynamically set users flags during the current session.",
                      VERSION,
                      "http://admin.flags.justca.me/to/your/session");
    handle.regAdminCmd("hm_flags","flagCmd",FLAG_ROOT,"Changes clients admin flags on the fly.");
    return 0;
}

int flagCmd(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 3)
    {
        hmReplyToClient(client, "Usage: " + args[0] + " <targets> [+/add|-/remove|toggle] <flags> - The second argument defaults to toggle if left empty.");
        return 1;
    }
    int FLAGS[26] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554431 };
    int mode = 0;
    string flags;
    args[2] = lower(args[2]);
    if (argc > 3)
    {
        flags = lower(args[3]);
        if ((args[2] == "+") || (args[2] == "add"))
            mode = 1;
        else if ((args[2] == "-") || (args[2] == "remove") || (args[2] == "rem"))
            mode = 2;
    }
    else
        flags = args[2];
    vector<hmPlayer*> targs;
    int targn = hmProcessTargetsPtr(client,args[1],targs,FILTER_NAME|FILTER_UUID|FILTER_IP);
    if (targn < 1)
        hmReplyToClient(client,"No matching players found.");
    else
    {
        for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
        {
            for (auto flag = flags.begin(), flage = flags.end();flag != flage;++flag)
            {
                if (mode == 1)
                    (*it)->flags |= FLAGS[*flag-97];
                else if (((mode == 2) || (!mode)) && (((*it)->flags & FLAGS[*flag-97]) == FLAGS[*flag-97]))
                    (*it)->flags -= FLAGS[*flag-97];
                else if (!mode)
                    (*it)->flags |= FLAGS[*flag-97];
            }
            if (mode == 1)
                hmSendCommandFeedback(client,"Gave '" + flags + "' flags to " + (*it)->name);
            else if (mode == 2)
                hmSendCommandFeedback(client,"Removed '" + flags + "' flags from " + (*it)->name);
            else
                hmSendCommandFeedback(client,"Toggled '" + flags + "' flags on " + (*it)->name);
        }
    }
    return 0;
}

}

