#include <fstream>
#include <cstdlib>
#include <iostream>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION     "v0.0.8"

bool loadMotd();
string parseMotdLine(string client, string line);
bool motdEnabled;
bool useJson;
vector<string> motdLines;
vector<string> motdCmds;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));
static void (*addConfigButtonCmd)(string,string,int,std::string);

string toggleButton(string name, int socket, string ip, string client)
{
    if (motdEnabled)
    {
        hmFindConVar("motd_enabled")->setBool(false);
        return "The MOTD plugin is now disabled . . .";
    }
    hmFindConVar("motd_enabled")->setBool(true);
    return "The MOTD plugin is now enabled . . .";
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo(    "MOTD",
                          "nigel",
                          "Welcome Message of the Day",
                          VERSION,
                          "http://welcome.all.who.justca.me/to/the/server/"    );
    handle.hookConVarChange(handle.createConVar("motd_enabled","1","Enable or disable the motd plugin.",FCVAR_NOTIFY,true,0.0,true,1.0),"enablePlugin");
    handle.regAdminCmd("hm_motd_reload","reloadMotd",FLAG_CHAT,"Reload the motd files.");
    mkdirIf("./halfMod/config/motd/");
    motdEnabled = loadMotd();
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            *(void **) (&addConfigButtonCmd) = it->getFunc("addConfigButtonCmd");
            (*addConfigButtonCallback)("toggleMotd","Toggle MOTD",FLAG_CVAR,&toggleButton);
            (*addConfigButtonCmd)("reloadMotd","Reload MOTD",FLAG_CHAT,"hm_motd_reload");
        }
    }
    return 0;
}

int enablePlugin(hmConVar &cvar, string oldVal, string newVal)
{
    motdEnabled = cvar.getAsBool();
    if (motdEnabled)
    {
        motdEnabled = loadMotd();
        if (!motdEnabled)
        {
            hmOutDebug("Error: Missing motd files `./halfMod/config/motd/motd.json|txt` and `./halfMod/config/motd/exec.txt` . . .");
            cvar.setBool(motdEnabled);
        }
    }
    else
    {
        motdLines.clear();
        motdCmds.clear();
        //hmReplyToClient(client,"The MOTD plugin is now disabled.");
    }
    return 0;
}

int reloadMotd(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (!motdEnabled)
        hmReplyToClient(client,"The MOTD plugin is not enabled!");
    else
    {
        motdEnabled = loadMotd();
        if (!motdEnabled)
            hmReplyToClient(client,"Error: Missing motd files `./halfMod/config/motd/motd.json|txt` and `./halfMod/config/motd/exec.txt` . . .");
        else
            hmReplyToClient(client,"The MOTD has been reloaded from file.");
    }
    return 0;
}

int onPlayerJoin(hmHandle &handle, rens::smatch args)
{
    string client = stripFormat(args[1].str());
    if (motdEnabled)
    {
        if (motdLines.size() > 0)
        {
            if (useJson)
            {
                for (auto it = motdLines.begin(), ite = motdLines.end();it != ite;++it)
                    hmSendRaw("execute as " + client + " at @s run tellraw " + client + " " + *it,false);
            }
            else
            {
                for (auto it = motdLines.begin(), ite = motdLines.end();it != ite;++it)
                    hmSendRaw("tellraw " + client + " [\"" + parseMotdLine(client,*it) + "\"]",false);
            }
        }
        if (motdCmds.size() > 0)
        {
            for (auto it = motdCmds.begin(), ite = motdCmds.end();it != ite;++it)
                hmServerCommand(parseMotdLine(client,*it),false);
        }
    }
    return 0;
}

}

bool loadMotd()
{
    string line;
    motdLines.clear();
    motdCmds.clear();
    ifstream file ("./halfMod/config/motd/motd.json");
    static rens::regex comment ("\\s*#.*");
    if (file.is_open())
    {
        useJson = true;
        while (getline(file,line))
        {
            if (rens::regex_match(line,comment))
                continue;
            motdLines.push_back(line);
        }
        file.close();
    }
    else
    {
        file.open("./halfMod/config/motd/motd.txt");
        if (file.is_open())
        {
            useJson = false;
            while (getline(file,line))
            {
                if (rens::regex_match(line,comment))
                    continue;
                motdLines.push_back(line);
            }
            file.close();
        }
        else
            hmOutDebug("[MOTD]: `./halfMod/config/motd/motd.json|txt` file is missing . . .");
    }
    file.open("./halfMod/config/motd/exec.txt");
    if (file.is_open())
    {
        while (getline(file,line))
        {
            if (rens::regex_match(line,comment))
                continue;
            motdCmds.push_back(line);
        }
        file.close();
    }
    else
        hmOutDebug("[MOTD]: `./halfMod/config/motd/exec.txt` file is missing . . .");
    if (motdLines.size() + motdCmds.size() > 0)
        return true;
    return false;
}

string parseMotdLine(string client, string line)
{
    string out;
    string word;
    vector<hmPlayer> targs;
    int targn;
    string space;
    bool skipSpace = true;
    if (line.size() > 0)
    {
        while ((word = gettok(line,1," ")).size() > 0)
        {
            if (skipSpace)
            {
                space.clear();
                skipSpace = false;
            }
            else
                space = " ";
            line = deltok(line,1," ");
            if (word.substr(0,2) == "%%")
                out.append(space + word.erase(0,1));
            else if (word[0] == '%')
            {
                if (word == "%+")
                    skipSpace = true;
                else
                {
                    targn = hmProcessTargets(client,word,targs,FILTER_NAME);
                    if (targn > 0)
                    {
                        for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
                            out.append(space + it->name + ",");
                        out.pop_back();
                    }
                    else
                        out += space;
                }
            }
            else
                out.append(space + word);
        }
    }
    return out;
}

