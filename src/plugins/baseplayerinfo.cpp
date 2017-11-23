#include <iostream>
#include <string>
#include <regex>
#include <ctime>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.0.1"

string amtTime(/*love you*/long times);

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(  "Base Player Info",
                        "nigel",
                        "Retrieve info about players that are or have been on the server.",
                        VERSION,
                        "http://when.did.you.justca.me/" );
    handle.regConsoleCmd("hm_seen","seenPlayer","When was player last online?");
    handle.regAdminCmd("hm_whois","whoisPlayer",FLAG_ADMIN);
    handle.regConsoleCmd("hm_whereami","wherePlayer","Display your whereabouts to everyone on the server.");
    handle.regConsoleCmd("hm_timetillday","timeTillDay","Display the time until the sun rises.");
}

int seenPlayer(hmHandle &handle, string caller, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(caller,"Usage: " + args[0] + " <player>");
        return 1;
    }
    hmGlobal *global;
    global = recallGlobal(global);
    hmPlayer client = hmGetPlayerInfo(args[1]);
    time_t vTime = time(NULL);
    if (client.name != "")
    {
        vTime -= client.join;
        hmSendMessageAll(client.name + " has been online for " + amtTime(vTime) + "!");
    }
    else
    {
        client = hmGetPlayerData(args[1]);
        if (client.uuid != "")
        {
            vTime -= client.quit;
            hmSendMessageAll(client.name + " was last online " + amtTime(vTime) + " ago.");
        }
        else
            hmSendMessageAll("I have never seen " + args[1] + " before!");
    }
    return 0;
}

int whoisPlayer(hmHandle &handle, string caller, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(caller,"Usage: " + args[0] + " <player>");
        return 1;
    }
    caller = stripFormat(caller);
    hmGlobal *global;
    global = recallGlobal(global);
    vector<hmPlayer> targs;
	int targn = hmProcessTargets(caller,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR|FILTER_IP|FILTER_UUID);
	if (targn < 1)
	{
	    // lookup offline player
	    hmPlayer target = hmGetPlayerData(args[1]);
	    if (target.uuid != "")
	    {
	        string access;
	        if (target.flags == 0)
                access = "None";
            else
            {
                if ((target.flags & FLAG_ROOT) == FLAG_ROOT)
                    access = "root";
                else
                {
                    int FLAGS[25] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216 };
                    string readable[25] = { "admin","ban","chat","custom1","effect","config","changeworld","cvar","inventory","custom2","kick","custom3","gamemode","custom4","oper","custom5","rsvp","rcon","slay","time","unban","vote","whitelist","cheats","weather" };
                    for (int i = 0;i < 25;i++)
                       if ((target.flags & FLAGS[i]) > 0)
                           access = addtok(access,readable[i],", ");
                }
                access.front() = toupper(access.at(0));
            }
	        time_t cTime = time(NULL);
	        hmReplyToClient(caller,"Whois " + target.name + ":");
            hmReplyToClient(caller,"  Last IP: " + target.ip);
            hmReplyToClient(caller,"  UUID: " + target.uuid);
            hmReplyToClient(caller,"  Access: " + access);
            hmReplyToClient(caller,"  Last Online: " + amtTime(cTime-target.quit) + " ago");
            if (target.death > 0)
            {
                hmReplyToClient(caller,"  Last Death: " + amtTime(cTime-target.death) + " ago " + target.name + " " + target.deathmsg);
            }
            if (target.custom != "")
            {
                for (int i = 1;i <= numtok(target.custom,"\n");i++)
                    hmReplyToClient(caller,"  " + gettok(gettok(target.custom,i,"\n"),1,"=") + ": " + gettok(gettok(target.custom,i,"\n"),2,"="));
            }
	    }
	    else
	        hmReplyToClient(caller,"No matching players found.");
	}
	else
	{
	    string stripClient;
		for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
		{
			stripClient = stripFormat(it->name);
			handle.hookPattern(stripClient + "Whois" + caller,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Entity data updated to: \\{(.*Tags:\\[\"(" + stripClient + ")Whois(" + caller + ")\"\\].*)\\}$","whoisLookup");
			hmSendRaw("execute @a[name=" + stripClient + ",m=0] ~ ~ ~ summon minecraft:area_effect_cloud ~ ~ ~ {Duration:1,ReapplicationDelay:0,Tags:[\"" + stripClient + "Whois" + caller + "\"]}");
			hmSendRaw("execute @a[name=" + stripClient + ",m=1] ~ ~ ~ summon minecraft:area_effect_cloud ~ ~ ~ {Duration:1,ReapplicationDelay:1,Tags:[\"" + stripClient + "Whois" + caller + "\"]}");
			hmSendRaw("execute @a[name=" + stripClient + ",m=2] ~ ~ ~ summon minecraft:area_effect_cloud ~ ~ ~ {Duration:1,ReapplicationDelay:2,Tags:[\"" + stripClient + "Whois" + caller + "\"]}");
			hmSendRaw("execute @a[name=" + stripClient + ",m=3] ~ ~ ~ summon minecraft:area_effect_cloud ~ ~ ~ {Duration:1,ReapplicationDelay:3,Tags:[\"" + stripClient + "Whois" + caller + "\"]}");
			hmSendRaw("entitydata @e[tag=" + stripClient + "Whois" + caller + "] {PortalCooldown:1}");
		}
	}
    return 0;
}

int whoisLookup(hmHandle &handle, hmHook hook, smatch args)
{
    string tags = args[1].str(), client = args[2].str(), caller = args[3].str(), pos, dim, gm, access;
    regex ptrn ("Pos:\\[(-?[0-9]+)(\\.[0-9]+)?d,(-?[0-9]+)(\\.[0-9]+)?d,(-?[0-9]+)(\\.[0-9]+)?d\\]");
    smatch ml;
    regex_search(tags,ml,ptrn);
    pos = ml[1].str() + " " + ml[3].str() + " " + ml[5].str();
    ptrn = "Dimension:(-?[0-9]+)";
    regex_search(tags,ml,ptrn);
    dim = ml[1].str();
    switch (stoi(dim))
    {
        case -1:
        {
            dim = "The Nether";
            break;
        }
        case 0:
        {
            dim = "The Overworld";
            break;
        }
        case 1:
        {
            dim = "The End";
            break;
        }
        default:
            dim = "Unknown (" + dim + ")";
    }
    ptrn = "ReapplicationDelay:(-?[0-9]+)";
    regex_search(tags,ml,ptrn);
    gm = ml[1].str();
    switch (stoi(gm))
    {
        case 0:
        {
            gm = "Survival";
            break;
        }
        case 1:
        {
            gm = "Creative";
            break;
        }
        case 2:
        {
            gm = "Adventure";
            break;
        }
        case 3:
        {
            gm = "Spectator";
            break;
        }
        default:
            gm = "Unknown (" + gm + ")";
    }
    hmPlayer target = hmGetPlayerInfo(client);
    if (target.flags == 0)
        access = "None";
    else
    {
        int FLAGS[25] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216 };
        string readable[25] = { "admin","ban","chat","custom1","effect","config","changeworld","cvar","inventory","custom2","kick","custom3","gamemode","custom4","oper","custom5","rsvp","rcon","slay","time","unban","vote","whitelist","cheats","weather" };
        if ((target.flags & FLAG_ROOT) == FLAG_ROOT)
            access = "root";
        else
        {
            for (int i = 0;i < 25;i++)
               if ((target.flags & FLAGS[i]) > 0)
                   access = addtok(access,readable[i],", ");
        }
        access.front() = toupper(access.at(0));
    }
    time_t cTime = time(NULL);
    hmReplyToClient(caller,"Whois " + target.name + ":");
    hmReplyToClient(caller,"  IP: " + target.ip);
    hmReplyToClient(caller,"  UUID: " + target.uuid);
    hmReplyToClient(caller,"  Access: " + access);
    hmReplyToClient(caller,"  Gamemode: " + gm);
    hmReplyToClient(caller,"  Coords: " + pos);
    hmReplyToClient(caller,"  Dimension: " + dim);
    hmReplyToClient(caller,"  Online: " + amtTime(cTime-target.join));
    if (target.death > 0)
    {
        hmReplyToClient(caller,"  Last Death: " + amtTime(cTime-target.death) + " ago " + target.name + " " + target.deathmsg);
    }
    if (target.custom != "")
    {
        for (int i = 1;i <= numtok(target.custom,"\n");i++)
            hmReplyToClient(caller,"  " + gettok(gettok(target.custom,i,"\n"),1,"=") + ": " + gettok(gettok(target.custom,i,"\n"),2,"="));
    }
    handle.unhookPattern(hook.name);
    return 1;
}

int wherePlayer(hmHandle &handle, string client, string args[], int argc)
{
    client = stripFormat(lower(client));
    handle.hookPattern(client + "WhereAmI","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Entity data updated to: \\{(.*Tags:\\[\"(" + client + ")WhereAmI\"\\].*)\\}$","whereAmI");
    hmSendRaw("execute " + client + " ~ ~ ~ summon minecraft:area_effect_cloud ~ ~ ~ {Duration:1,Tags:[\"" + client + "WhereAmI\"]}\n\
               entitydata @e[tag=" + client + "WhereAmI] {PortalCooldown:1}");
    return 0;
}

int whereAmI(hmHandle &handle, hmHook hook, smatch args)
{
    string tags = args[1].str(), client = args[2].str(), pos, dim;
    regex ptrn ("Pos:\\[(-?[0-9]+)(\\.[0-9]+)?d,(-?[0-9]+)(\\.[0-9]+)?d,(-?[0-9]+)(\\.[0-9]+)?d\\]");
    smatch ml;
    regex_search(tags,ml,ptrn);
    pos = ml[1].str() + " " + ml[3].str() + " " + ml[5].str();
    ptrn = "Dimension:(-?[0-9]+)";
    regex_search(tags,ml,ptrn);
    dim = ml[1].str();
    switch (stoi(dim))
    {
        case -1:
        {
            dim = "Nether";
            break;
        }
        case 0:
        {
            dim = "Overworld";
            break;
        }
        case 1:
        {
            dim = "End";
            break;
        }
        default:
            dim = "Unknown (" + dim + ")";
    }
    hmSendMessageAll(hmGetPlayerInfo(client).name + " is at coordinates ( " + pos + " ) in the " + dim + ".");
    handle.unhookPattern(hook.name);
    return 1;
}

int timeTillDay(hmHandle &handle, string client, string args[], int argc)
{
    handle.hookPattern("daytime","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Time is ([0-9]+)","timeCheck");
    hmSendRaw("time query daytime");
    return 0;
}

int timeCheck(hmHandle &handle, hmHook hook, smatch args)
{
    int dTime = stoi(args[1].str()), ticks = 0;
    if (dTime < 12541)
        ticks = 12541 - dTime;
    else if (dTime > 23458)
        ticks = dTime - 10917;
    if (ticks > 0)
        hmSendMessageAll("The sun will set in " + amtTime(ticks/20) + ".");
    else
        hmSendMessageAll("The sun will rise in " + amtTime((23458-dTime)/20) + ".");
    handle.unhookPattern(hook.name);
    return 1;
}

}

string amtTime(long times)
{
    if (times == 0)
        return "0 seconds";
    string out = "";
    if (times >= 31536000)
    {
        out = data2str("%li year",times/31536000);
        if (times >= 63072000)
            out += "s";
        times %= 31536000;
    }
    if (times >= 2592000)
    {
        if (out != "")
            out += ", ";
        out += data2str("%li month",times/2592000);
        if (times >= 5184000)
            out += "s";
        times %= 2592000;
    }
    if (times >= 604800)
    {
        if (out != "")
            out += ", ";
        out += data2str("%li week",times/604800);
        if (times >= 1209600)
            out += "s";
        times %= 604800;
    }
    if (times >= 86400)
    {
        if (out != "")
            out += ", ";
        out += data2str("%li day",times/86400);
        if (times >= 172800)
            out += "s";
        times %= 86400;
    }
    if (times >= 3600)
    {
        if (out != "")
            out += ", ";
        out += data2str("%li hour",times/3600);
        if (times >= 7200)
            out += "s";
        times %= 3600;
    }
    if (times >= 60)
    {
        if (out != "")
            out += ", ";
        out += data2str("%li minute",times/60);
        if (times >= 120)
            out += "s";
        times %= 60;
    }
    if (times > 0)
    {
        if (out != "")
        {
            if (numtok(out,",") > 1)
                out += ", and ";
            else
                out += " and ";
        }
        out += data2str("%li second",times);
        if (times > 1)
            out += "s";
    }
    return out;
}

