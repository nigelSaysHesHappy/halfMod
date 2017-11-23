#include <iostream>
#include <string>
#include <regex>
#include <ctime>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.0.1"

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(  "First",
                        "nigel",
                        "My first plugin ever!",
                        VERSION,
                        "http://i.justca.me/first/" );
    handle.regConsoleCmd("hm_hello"/*commands beginning with hm_ can be triggered in chat by replacing it with a !*/,"helloWorld","Hello World!");
    handle.regAdminCmd("hm_piggy","goodPiggy"/*the name of the function*/,FLAG_ADMIN/*flags required to use the command*/,"Good, good rubber piggy");
    handle.regConsoleCmd("hm_findpiggy","aLostPiggyIsSadPiggy","Oh where oh where did my rubber piggy go?"/*Command description*/);
}

// called when the world has finished loading
int onWorldInit(hmHandle &handle, smatch args)
{
    hmSendRaw("execute @r[type=minecraft:pig] ~ ~ ~ say Oink!");
    // returning non-0 will disallow any plugins after this one to run their onWorldInit()'s
    return 0;
}

int helloWorld(hmHandle &handle, string client, string args[], int argc)
{
    hmSendMessageAll("Hello World!");
    time_t cTime;
    time(&cTime);
    tm *thetime = localtime(&cTime);
    hmSendMessageAll(strremove(data2str("The time is: %s",asctime(thetime)),"\n"));
    return 0;
}

int goodPiggy(hmHandle &handle, string client, string args[], int argc)
{
    string piggyName = "Pinky Piggy Rubber Baby Buggy Bumpers";
    piggyName = randtok(piggyName," "); // select a random name
    if (argc > 1)
        piggyName = args[1];
    string uuid = hmGetPlayerUUID(client);
    hmSendRaw("execute " + client + " ~ ~ ~ summon minecraft:pig ~ ~ ~ {Saddle:1,InLove:30,LoveCauseLeast:" + uuid + ",LoveCauseMost:" + uuid + ",CustomName:\"" + piggyName + "\",CustomNameVisible:1,Tags:[\"" + client + "Piggy\"]}");
    hmSendRaw("give " + client + " minecraft:carrot_on_a_stick");
    hmSendRaw("execute " + client + " ~ ~ ~ execute @e[type=minecraft:pig,name=" + piggyName + ",c=1] ~ ~ ~ say Oink!");
    return 0;
}

int aLostPiggyIsSadPiggy(hmHandle &handle, string client, string args[], int argc)
{
    handle.hookPattern(client + "Piggy","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(" + client + "): Entity data updated to: \\{(.*Tags:\\[\"" + client + "Piggy\"\\].*)\\}\\]$","happyPiggy");
    hmSendRaw("execute " + client + " ~ ~ ~ entitydata @e[tag=" + client + "Piggy,c=1] {PortalCooldown:1}");
    return 0;
}

int happyPiggy(hmHandle &handle, hmHook hook, smatch args)
{
    string client = args[1], tags = args[2].str();
    regex ptrn ("Pos:\\[(-?[0-9]+)(\\.[0-9]+)?d,(-?[0-9]+)(\\.[0-9]+)?d,(-?[0-9]+)(\\.[0-9]+)?d\\]");
    smatch ml;
    regex_search(tags,ml,ptrn);
    hmSendRaw("execute " + client + " ~ ~ ~ execute @e[tag=" + client + "Piggy,c=1] ~ ~ ~ say Oink, oink! I'm right here! ( " + ml[1].str() + ", " + ml[3].str() + ", " + ml[5].str() + " )");
    handle.unhookPattern(hook.name);
    // always return non-0 from hooks if no other plugin should need this data
    return 1;
}

}

