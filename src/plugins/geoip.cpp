#include <fstream>
#include <cstdlib>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION     "v0.0.3"
#define GEOIPDAT    "./geoip/GeoLiteCity.dat"

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(    "GeoIP",
                          "nigel",
                          "Get GeoIP location of players when they connect.",
                          VERSION,
                          "http://now.we.know.where.you.justca.me/from/"    );
}

// event triggered when a player connects
int onPlayerConnect(hmHandle &handle, smatch args)
{
    string ip = args[2].str();
    if (ip == "127.0.0.1")
        hmSendMessageAll("Player " + args[1].str() + " is connecting from localhost.");
    else
    {
        system(string("geoiplookup -f \"" + string(GEOIPDAT) + "\" \"" + ip + "\" > tempgeo").c_str());
        ifstream file ("tempgeo");
        string str;
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
            regex ptrn ("(.*?[0-9].*|N/A)");
            out = gettok(str,1,",");
            for (int i = 2;i < 6;i++)
                if (!regex_match(gettok(str,i,", "),ptrn))
                    out = addtok(out,gettok(str,i,", "),", ");
            hmSendMessageAll("Player " + args[1].str() + " is connecting from" + out + ".");
        }
    }
    return 0;
}

}

