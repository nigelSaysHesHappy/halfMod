#include <cstdlib>
#include <fstream>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.0.3"

void recompile(string client, string filename, string flags)
{
    system(flags.c_str());
    ifstream file ("src/plugins/autoc.log");
    if (file.is_open())
    {
        string in, success;
        while (getline(file,in))
            success = in;
        file.close();
        remove("src/plugins/autoc.log");
        if (success == "Installed . . .")
        {
            hmServerCommand("hm plugins load " + filename);
            hmReplyToClient(client,"Successfully recompiled and reloaded " + filename);
        }
        else
            hmReplyToClient(client,"Failed to compile " + filename);
    }
    else
        hmReplyToClient(client,"Error: Could not find compiler . . .");
}

int timedReload(hmHandle &handle, string args)
{
    recompile(gettok(args,1,"\n"),gettok(args,2,"\n"),gettok(args,3,"\n"));
    return 1;
}

int compileCmd(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " [-f flags] <plugin name>");
        return 1;
    }
    string filename, flags;
    if ((args[1] == "-f") && (argc > 3))
    {
        filename = args[3];
        flags = "cd src/plugins/; ./compile.sh --flag " + args[2] + " --install " + filename + ".cpp > autoc.log";
    }
    else if (args[1] != "-f")
    {
        filename = args[1];
        flags = "cd src/plugins/; ./compile.sh --install " + filename + ".cpp > autoc.log";
    }
    else
    {
        hmReplyToClient(client,"Usage: " + args[0] + " [-f flags] <plugin name>");
        return 1;
    }
    //filename = deltok(filename,numtok(filename,"."),".");
    if (hmIsPluginLoaded(filename))
    {
        hmServerCommand("hm plugins unload " + filename);
        // start a timer
        handle.createTimer(client.name + "\n" + filename,1,&timedReload,client.name + "\n" + filename + "\n" + flags);
    }
    else
        recompile(client.name,filename,flags);
    return 0;
}

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Easy Compile",
                      "nigel",
                      "Unload, compile, and reload plugins with one command.",
                      VERSION,
                      "http://easy.compile.made.you.justca.me/easier");
    handle.regAdminCmd("hm_compile",&compileCmd,FLAG_ROOT,"Unload, compile, and reload a plugin");
    return 0;
}

