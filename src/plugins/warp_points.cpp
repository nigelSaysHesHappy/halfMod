#include "halfmod.h"
#include <vector>
#include <fstream>
#include "str_tok.h"
using namespace std;

#define VERSION "v0.1.1"

struct WarpPoint
{
    string name;
    int dimension;
    string pos;
};

bool warpEnabled = true;
int warpFlag = 0;

void loadWarpPoints();
vector<WarpPoint>::iterator getWarpPoint(string name);
bool hasWarpAccess(const hmPlayer &caller);
string getDimension(int dim);
string getDimensionHuman(int dim);
int parseDimension(string dim);
void saveWarpPoints();

int warpMenuCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc);
int warpCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc);
int warpAddCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc);
int getWarpPos(hmHandle &handle, hmHook hook, rens::smatch args);
int warpRemCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc);
int cWarpEnable(hmConVar &cvar, string oldVal, string newVal);
int cWarpFlags(hmConVar &cvar, string oldVal, string newVal);

vector<WarpPoint> warpPoints;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string toggleButton(string name, int socket, string ip, string client)
{
    if (warpEnabled)
    {
        hmFindConVar("warp_enabled")->setBool(false);
        return "Warp system is now disabled . . .";
    }
    hmFindConVar("warp_enabled")->setBool(true);
    return "Warp system is now enabled . . .";
}

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Warp Points",
                      "nigel",
                      "Create warp points",
                      VERSION,
                      "http://you.justca.me/to/my/pretty/house/in/the/forest");
    handle.regConsoleCmd("hm_warpmenu",&warpMenuCmd,"Open the warp menu");
    handle.regConsoleCmd("hm_warp",&warpCmd,"Warp to a point");
    handle.regAdminCmd("hm_warpadd",&warpAddCmd,FLAG_ADMIN,"Create a new warp point");
    handle.regAdminCmd("hm_warprem",&warpRemCmd,FLAG_ADMIN,"Remove a warp point");
    handle.hookConVarChange(handle.createConVar("warp_enabled","true","Enable the warp system",0,true,0.0,true,1.0),&cWarpEnable);
    handle.hookConVarChange(handle.createConVar("warp_flags","\0","Set the flag required to warp. Example: 'd' for CUSTOM1",0),&cWarpFlags);
    loadWarpPoints();
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            (*addConfigButtonCallback)("toggleWarp","Toggle Warp System",FLAG_CVAR,&toggleButton);
        }
    }
    return 0;
}

int cWarpEnable(hmConVar &cvar, string oldVal, string newVal)
{
    warpEnabled = cvar.getAsBool();
    return 0;
}

int cWarpFlags(hmConVar &cvar, string oldVal, string newVal)
{
    if ((isalpha(newVal[0])) || (newVal[0] == '\0'))
        warpFlag = hmResolveFlag(newVal[0]);
    else
        cvar.setString("\0");
    return 0;
}

void loadWarpPoints()
{
    warpPoints.clear();
    mkdirIf("./halfMod/plugins/warp_points");
    ifstream file ("./halfMod/plugins/warp_points/points.conf");
    if (file.is_open())
    {
        string line;
        static rens::regex ptrn ("^([^=]+)=(-?[0-9]+)=(-?[0-9]+\\.?[0-9]* -?[0-9]+\\.?[0-9]* -?[0-9]+\\.?[0-9]*)$");
        rens::smatch ml;
        while (getline(file,line))
            if (rens::regex_match(line,ml,ptrn))
                warpPoints.push_back({ml[1].str(),stoi(ml[2].str()),ml[3].str()});
        file.close();
    }
}

int warpMenuCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc)
{
    if (warpEnabled)
    {
        if (hasWarpAccess(caller))
        {
            if (warpPoints.size() < 1)
                hmReplyToClient(caller,"There are no warp points yet!");
            else
            {
                int page = 1, pages = warpPoints.size()/10;
                if ((argc > 1) && (stringisnum(args[1],1)))
                    page = stoi(args[1]);
                if ((int)warpPoints.size() % 10)
                    pages++;
                if (page > pages)
                    page = pages;
                hmReplyToClient(caller,"Displaying page " + to_string(page) + "/" + to_string(pages) + " of the warp points:");
                auto ite = warpPoints.end();
                if (page != pages)
                    ite = warpPoints.begin() + 10*page;
                for (auto it = warpPoints.begin() + 10*(page-1);it != ite;++it)
                    hmSendRaw("tellraw " + caller.name + " [{\"text\":\"    Warp to \"},{\"text\":\"" + it->name + "\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!warp \\\\\"" + it->name + "\\\\\"\"}},{\"text\":\" \",\"color\":\"none\"},{\"text\":\"[DELETE]\",\"color\":\"red\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!warprem \\\\\"" + it->name + "\\\\\"\"}}]");
            }
        }
        else
            hmReplyToClient(caller,"You do not have access to this command.");
    }
    else
        hmReplyToClient(caller,"The warp system is not enabled.");
    return 0;
}

int warpCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc)
{
    if (warpEnabled)
    {
        if (hasWarpAccess(caller))
        {
            if (argc < 2)
                hmReplyToClient(caller,"Usage: " + args[0] + " <point>");
            else
            {
                auto point = getWarpPoint(args[1]);
                if (point != warpPoints.end())
                {
                    hmReplyToClient(caller,"Warping to " + point->name + " . . .");
                    hmSendRaw("execute in " + getDimension(point->dimension) + " run teleport " + caller.name + " " + point->pos);
                }
                else
                    hmReplyToClient(caller,"Unknown warp point: " + args[1] + "!");
            }
        }
        else
            hmReplyToClient(caller,"You do not have access to this command.");
    }
    else
        hmReplyToClient(caller,"The warp system is not enabled.");
    return 0;
}

int warpAddCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc)
{
    if ((argc < 2) || ((argc < 6) && (argc > 2)))
    {
        hmReplyToClient(caller,"Usage: " + args[0] + " <name> - Uses current coords.");
        hmReplyToClient(caller,"Usage: " + args[0] + " <name> <dimension> <x> <y> <z> - Uses specified coords.");
    }
    else
    {
        if (getWarpPoint(args[1]) == warpPoints.end())
        {
            if (argc > 5)
            {
                static rens::regex ptrn ("-?[0-9]+|overworld|the_end|the_nether");
                if (!rens::regex_match(args[2],ptrn))
                {
                    hmReplyToClient(caller,"Usage: " + args[0] + " <name> - Uses current coords.");
                    hmReplyToClient(caller,"Usage: " + args[0] + " <name> <dimension> <x> <y> <z> - Uses specified coords.");
                    hmReplyToClient(caller,"Dimension can be an integer representation, \"the_nether\", \"overworld\", or \"the_end\".");
                    return 1;
                }
                int dim = parseDimension(args[2]);
                static rens::regex ptrn2 = "^-?[0-9]+\\.?[0-9]*$";
                for (int i = 3;i < 6;i++)
                {
                    if (!rens::regex_match(args[i],ptrn2))
                    {
                        hmReplyToClient(caller,"Usage: " + args[0] + " <name> - Uses current coords.");
                        hmReplyToClient(caller,"Usage: " + args[0] + " <name> <dimension> <x> <y> <z> - Uses specified coords.");
                        return 1;
                    }
                }
                warpPoints.push_back({args[1],dim,args[3] + " " + args[4] + " " + args[5]});
                saveWarpPoints();
                hmReplyToClient(caller,"Created a new warp point named " + args[1] + " in " + getDimensionHuman(dim) + " at " + args[3] + " " + args[4] + " " + args[5]);
            }
            else
            {
                handle.hookPattern("warpadd " + args[1],"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + caller.name + ") has the following entity data: \\{(.*Pos: \\[([^d.]+\\.?[0-9]*)d, ([^d.]+\\.?[0-9]*)d, ([^d.]+\\.?[0-9]*)d\\].*)\\}$",&getWarpPos);
                hmSendRaw("data get entity " + caller.name);
            }
        }
        else
            hmReplyToClient(caller,"A warp point with that name already exists!");
    }
    return 0;
}

int getWarpPos(hmHandle &handle, hmHook hook, rens::smatch args)
{
    string caller = args[1].str(), tags = args[2].str(), name = deltok(hook.name,1," ");
    int dim;
    string pos = args[3].str() + " " + args[4].str() + " " + args[5].str();
    static rens::regex ptrn ("Dimension: (-?[0-9]+)");
    rens::smatch ml;
    if (rens::regex_search(tags,ml,ptrn))
    {
        dim = stoi(ml[1].str());
        warpPoints.push_back({name,dim,pos});
        saveWarpPoints();
        hmReplyToClient(caller,"Created a new warp point named " + name + " in " + getDimensionHuman(dim) + " at " + pos);
    }
    else
        hmReplyToClient(caller,"An unknown error occurred...");
    handle.unhookPattern(hook.name);
    return 1;
}

int warpRemCmd(hmHandle &handle, const hmPlayer &caller, string args[], int argc)
{
    if (argc < 2)
        hmReplyToClient(caller,"Usage: " + args[0] + " <name>");
    else
    {
        auto point = getWarpPoint(args[1]);
        if (point != warpPoints.end())
        {
            warpPoints.erase(point);
            saveWarpPoints();
            hmReplyToClient(caller,"Removed the warp point named " + args[1]);
        }
        else
            hmReplyToClient(caller,"Unknown warp point: " + args[1] + "!");
    }
    return 0;
}

vector<WarpPoint>::iterator getWarpPoint(string name)
{
    name = lower(name);
    auto ite = warpPoints.end();
    for (auto it = warpPoints.begin();it != ite;++it)
        if (lower(it->name) == name)
            return it;
    return ite;
}

bool hasWarpAccess(const hmPlayer &caller)
{
    if ((warpFlag == 0) || ((caller.flags & warpFlag) == warpFlag))
        return true;
    return false;
}

string getDimension(int dim)
{
    switch (dim)
    {
        case -1:
            return "the_nether";
        case 0:
            return "overworld";
        case 1:
            return "the_end";
        default:
            return "";
    }
}

string getDimensionHuman(int dim)
{
    switch (dim)
    {
        case -1:
            return "The Nether";
        case 0:
            return "The Overworld";
        case 1:
            return "The End";
        default:
            return "Unknown (" + to_string(dim) + ")";
    }
}

int parseDimension(string dim)
{
    if (dim == "the_nether")
        return -1;
    if (dim == "overworld")
        return 0;
    if (dim == "the_end")
        return 1;
    return stoi(dim);
}

void saveWarpPoints()
{
    ofstream file ("./halfMod/plugins/warp_points/points.conf",ios_base::trunc);
    if (file.is_open())
    {
        for (auto it = warpPoints.begin(), ite = warpPoints.end();it != ite;++it)
            file<<it->name<<"="<<it->dimension<<"="<<it->pos<<"\n";
        file.close();
    }
}

