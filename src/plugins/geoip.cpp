#include <fstream>
#include <cstdlib>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION     "v0.1.1"
#define GEOIPDAT    "./geoip/GeoLiteCity.dat"

bool geoEnabled = false;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string toggleButton(string name, int socket, string ip, string client)
{
    if (geoEnabled)
    {
        hmFindConVar("geoip_onconnect")->setBool(false);
        return "GeoIP on connect is now disabled . . .";
    }
    hmFindConVar("geoip_onconnect")->setBool(true);
    return "GeoIP on connect is now enabled . . .";
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(    "GeoIP",
                          "nigel",
                          "Get GeoIP location of players when they connect.",
                          VERSION,
                          "http://now.we.know.where.you.justca.me/from/"    );
    handle.hookConVarChange(handle.createConVar("geoip_onconnect","false","Display player's GeoIP location info when connecting.",0,true,0.0,true,1.0),"cvarChange");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            //*(void **) (&addConfigButtonCmd) = it->getFunc("addConfigButtonCmd");
            (*addConfigButtonCallback)("toggleGeoIP","Toggle GeoIP",FLAG_CVAR,&toggleButton);
        }
    }
    return 0;
}

int cvarChange(hmConVar &cvar, string oldVal, string newVal)
{
    geoEnabled = cvar.getAsBool();
    return 0;
}

// event triggered when a player connects
int onPlayerConnect(hmHandle &handle, rens::smatch args)
{
    string ip = args[2].str();
    string client = args[1].str();
    string stripClient = stripFormat(client);
    if (ip == "127.0.0.1")
    {
        if (geoEnabled)
            hmSendMessageAll("Player " + client + " is connecting from localhost.");
        hmWritePlayerDat(stripClient,"GeoIP=localhost","GeoIP",true);
    }
    else
    {
        system(string("geoiplookup -f \"" + string(GEOIPDAT) + "\" \"" + ip + "\" > tempgeo").c_str());
        ifstream file ("tempgeo");
        string str, tmp;
        if (file.is_open())
        {
            getline(file,str);
            file.close();
            remove("tempgeo");
            string out;
            //regex ptrn (".*?: ((.*?)(, -?[0-9]*(.[0-9]*)?.*)?)");
            /*regex ptrn (".*?: ((.*?)(,([^,]*)){4}.*");
            smatch ml;
            regex_match(str,ml,ptrn);
            if (ml.size() > 2)
                out = ml[2];
            else if (ml.size() > 1)
                out = ml[1];
            else    out = deltok(str,1,":");*/
            str = deltok(str,1,":");
            static rens::regex ptrn ("([0-9]|N/A)");
            out = gettok(str,1,",");
            for (int i = 2;i < 6;i++)
            {
                tmp = gettok(str,i,",");
                if (!rens::regex_search(tmp,ptrn))
                    out = appendtok(out,tmp,",");
            }
            if (geoEnabled)
                hmSendMessageAll("Player " + client + " is connecting from" + out + ".");
            hmWritePlayerDat(stripClient,"GeoIP=" + out,"GeoIP",true);
        }
    }
    return 0;
}

}

