#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION		"v0.1.1"

int defBanTime = 0;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));
static void (*addConfigButtonCmd)(string,string,int,std::string);

string kickButton(string name, int socket, string ip, string client)
{
    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> !rcon hm_kick %all");
    return "Kicking all players . . .";
}

/*string floodButton(string name, int socket, string ip, string client)
{
    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> !rcon hm_flood");
    return "Toggled eternal flood . . .";
}

string droughtButton(string name, int socket, string ip, string client)
{
    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> !rcon hm_drought");
    return "Toggled eternal drought . . .";
}*/

string slayButton(string name, int socket, string ip, string client)
{
    hmSendRaw("hs raw [99:99:99] [Server thread/INFO]: <" + client + "> !rcon hm_slay %all");
    return "Slaying all players . . .";
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
	// THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
	recallGlobal(global);
	// ^^^^ THIS ONE ^^^^
	
	handle.pluginInfo(	"Admin Commands",
				"nigel",
				"Generic commands for admins.",
				VERSION,
				"http://aboooos.justca.me/"	);
	handle.hookConVarChange(handle.createConVar("default_ban_time","0","Default number of minutes to ban someone for if no value is provided to the hm_ban command.",0,true,0.0),"banTimeChange");
	handle.regAdminCmd("hm_kick","kickPlayer",FLAG_KICK,"Kick a player from the server.");
	handle.regAdminCmd("hm_ban","banPlayer",FLAG_BAN,"Ban a player from the server for X minutes.");
	handle.regAdminCmd("hm_banip","banIP",FLAG_BAN,"Ban any player connecting via IP from the server for X minutes.");
	handle.regAdminCmd("hm_unban","unbanPlayer",FLAG_UNBAN,"Remove a ban on a player.");
	handle.regAdminCmd("hm_unbanip","unbanIP",FLAG_UNBAN,"Remove a ban on an IP.");
	handle.regAdminCmd("hm_op","opPlayer",FLAG_OPER,"Grant operator status to a player.");
	handle.regAdminCmd("hm_deop","deopPlayer",FLAG_OPER,"Revoke a players' operator status.");
	handle.regAdminCmd("hm_timeadd","addTime",FLAG_TIME,"Add to the world time.");
	handle.regAdminCmd("hm_timeset","setTime",FLAG_TIME,"Set the time of day.");
	handle.regAdminCmd("hm_weather","changeWeather",FLAG_WEATHER,"Clear or set the weather.");
	//handle.regAdminCmd("hm_gamerule","setGamerule",FLAG_CVAR,"Set or view gamerules.");
	handle.regAdminCmd("hm_gamemode","setGamemode",FLAG_GAMEMODE,"Set yours or anothers' gamemode.");
    handle.regAdminCmd("hm_tphere","bringPlayer",FLAG_ADMIN,"Teleport a player to you.");
    handle.regAdminCmd("hm_noclip","toggleNoclip",FLAG_ADMIN,"Toggle noclip on players.");
    handle.regAdminCmd("hm_drought","toggleDrought",FLAG_WEATHER,"Enable/Disable eternal drought.");
    handle.regAdminCmd("hm_flood","toggleFlood",FLAG_WEATHER,"Enable/Disable eternal rain.");
    handle.regAdminCmd("hm_smite","smitePlayer",FLAG_SLAY,"Strike down a target with lightning.");
    handle.regAdminCmd("hm_rocket","rocketPlayer",FLAG_SLAY,"Launch a target in the air like a firework.");
    handle.regAdminCmd("hm_slay","slayPlayer",FLAG_SLAY,"Kill a target.");
    handle.regAdminCmd("hm_explode","explodePlayer",FLAG_SLAY,"Kill a target with an explosion.");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            *(void **) (&addConfigButtonCmd) = it->getFunc("addConfigButtonCmd");
            (*addConfigButtonCallback)("kick","Kick All",FLAG_KICK,&kickButton);
            //(*addConfigButtonCallback)("flood","Toggle Flood",FLAG_WEATHER,&floodButton);
            //(*addConfigButtonCallback)("drought","Toggle Drought",FLAG_WEATHER,&droughtButton);
            (*addConfigButtonCmd)("slay","Slay All",FLAG_SLAY,"hm_slay %all");
        }
    }
    return 0;
}

int banTimeChange(hmConVar &cvar, string oldVal, string newVal)
{
    defBanTime = cvar.getAsInt();
    return 0;
}

int onWorldInit(hmHandle &handle, smatch args)
{
	mkdirIf("./halfMod/plugins/admincmds/");
	time_t cTime = time(NULL);
	string line, buffer, victim;
	fstream file ("./halfMod/plugins/admincmds/expirations.txt",ios_base::in);
	if (file.is_open())
	{
		while (getline(file,line))
		{
			victim = gettok(line,3," ");
			if (gettok(line,1," ") == "banExpire")
			{
				if (cTime < stoi(gettok(line,2," ")))
				{
					handle.createTimer("unban",stoi(gettok(line,2," "))-cTime,"banExpire",line);
					buffer = buffer + line + "\n";
				}
				else
				{
					hmSendRaw("pardon " + victim);
					hmLog("Expired ban: " + victim,LOG_BAN,"bans.log");
				}
			}
			else if (gettok(line,1," ") == "ipbanExpire")
			{
				if (cTime < stoi(gettok(line,2," ")))
				{
					handle.createTimer("unban",stoi(gettok(line,2," "))-cTime,"ipbanExpire",line);
					buffer = buffer + line + "\n";
				}
				else
				{
					if (regex_match(victim,regex("([0-9]{1,3}\\.){3}[0-9]{1,3}")))
					{
						hmSendRaw("pardon-ip " + victim);
						hmLog("Expired ip-ban: " + victim,LOG_BAN,"bans.log");
					}
					else
					{
						hmPlayer temp = hmGetPlayerData(victim);
						if (temp.ip == "")
						{
							hmSendRaw("pardon-ip " + victim);
							hmLog("Expired ip-ban: " + victim + ". Unknown user/ip.",LOG_BAN,"bans.log");
						}
						else
						{
							hmSendRaw("pardon-ip " + temp.ip);
							hmLog("Expired ip-ban: " + temp.ip + " via player ID (" + victim + ")",LOG_BAN,"bans.log");
						}
					}
				}
			}
		}
		file.close();
		file.open("./halfMod/plugins/admincmds/expirations.txt",ios_base::out|ios_base::trunc);
		if (file.is_open())
		{
			file<<buffer;
			file.close();
		}
	}
	return 0;
}

int kickPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target>");
	else
	{
		string reason = "";
		if (argc > 2)
		{
			for (int i = 2;i < argc;i++)
				reason = appendtok(reason,args[i]," ");
		}
		else
			reason = "Bye! :)";
		vector<hmPlayer> targs;
		int targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
		if (targn < 1)
			hmReplyToClient(client,"No matching players found.");
		else
		{
			for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
			{
				hmSendRaw("kick " + it->name + " " + reason);
				hmSendCommandFeedback(client,"Kicked player " + it->name + " (" + reason + ")");
				hmLog(client.name + " kicked player " + it->name + " (" + reason + ")",LOG_KICK,"bans.log"); // will only log if (global->logMethod & LOG_KICK)
			}
		}
	}
	return 0;
}

int banPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target> [minutes, 0 = permanent] [reason]");
	else
	{
		int banTime;
		if (argc < 3)
			banTime = defBanTime;
		else if (!stringisnum(args[2],0))
		{
			hmReplyToClient(client,"Usage: " + args[0] + " <target> [minutes, 0 = permanent] [reason]");
			hmReplyToClient(client,"Minutes must be a valid positive integer.");
			return 1;
		}
		else
			banTime = atoi(args[2].c_str());
		string reason;
		if (argc > 3)
		{
			for (int i = 3;i < argc;i++)
				reason = appendtok(reason,args[i]," ");
		}
		else
		{
			if (banTime > 0)
				reason = args[2] + " minute(s)";
			else	reason = "Bye! :)";
		}
		vector<hmPlayer> targs;
		int targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
		if (targn < 1)
			hmReplyToClient(client,"No matching players found.");
		else
		{
			ofstream oFile;
			time_t cTime = time(NULL);
			if (banTime)
				oFile.open("./halfMod/plugins/admincmds/expirations.txt",ios_base::app);
			for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
			{
				hmSendRaw("kick " + it->name + " " + reason + "\nban " + it->name + " " + reason);
				if (banTime == 0)
				{
					hmSendCommandFeedback(client,"Permanently banned player " + it->name + " (" + reason + ")");
					hmLog(client.name + " permanently banned player " + it->name + " (" + reason + ")",LOG_BAN,"bans.log");
				}
				else
				{
					if (oFile.is_open())
						oFile<<"banExpire "<<cTime+(banTime*60)<<" "<<it->name<<endl;
					handle.createTimer("unban",banTime*60,"banExpire",data2str("banExpire %lu %s",cTime,it->name.c_str()));
					hmLog(data2str("%s banned player %s for %i minute(s) (%s)",client.name.c_str(),it->name.c_str(),banTime,reason.c_str()),LOG_BAN,"bans.log");
					hmSendCommandFeedback(client,data2str("Banned player %s for %i minute(s) (%s)",it->name.c_str(),banTime,reason.c_str()));
				}
			}
			if (oFile.is_open())
				oFile.close();
			else
				hmOutDebug(handle.getPlugin() + ": Cannot open file './halfMod/plugins/admincmds/expirations.txt'");
		}
	}
	return 0;
}

int banIP(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target|IP> [minutes, 0 = permanent] [reason]");
	else
	{
		int banTime;
		if (argc < 3)
			banTime = 0;
		else if (!stringisnum(args[2],0))
		{
			hmReplyToClient(client,"Usage: " + args[0] + " <target|IP> [minutes, 0 = permanent] [reason]");
			hmReplyToClient(client,"Minutes must be a valid positive integer.");
			return 1;
		}
		else
			banTime = atoi(args[2].c_str());
		string reason;
		if (argc > 3)
		{
			for (int i = 3;i < argc;i++)
				reason = appendtok(reason,args[i]," ");
		}
		else
		{
			if (banTime > 0)
				reason = args[2] + " minute(s)";
			else	reason = "Bye! :)";
		}
		ofstream oFile;
		time_t cTime = time(NULL);
		if (banTime)
			oFile.open("./halfMod/plugins/admincmds/expirations.txt",ios_base::app);
		if (regex_match(args[1],regex("([0-9]{1,3}\\.){3}[0-9]{1,3}")))
		{
			hmSendRaw("ban-ip " + args[1] + " " + reason);
			if (banTime == 0)
			{
				hmSendCommandFeedback(client,"Permanently banned IP " + args[1] + " (" + reason + ")");
				hmLog(client.name + " permanently banned ip " + args[1] + " (" + reason + ")",LOG_BAN,"bans.log");
			}
			else
			{
				if (oFile.is_open())
					oFile<<"ipbanExpire "<<cTime+(banTime*60)<<" "<<args[1]<<endl;
				handle.createTimer("unban",banTime*60,"ipbanExpire",data2str("ipbanExpire %lu %s",cTime,args[1].c_str()));
				hmLog(data2str("%s banned ip %s for %i minute(s) (%s)",client.name.c_str(),args[1].c_str(),banTime,reason.c_str()),LOG_BAN,"bans.log");
				hmSendCommandFeedback(client,data2str("Banned IP %s for %i minute(s) (%s)",args[1].c_str(),banTime,reason.c_str()));
			}
		}
		else
		{
			vector<hmPlayer> targs;
			int targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
			if (targn < 1)
				hmReplyToClient(client,"No matching players found.");
			else
			{
				for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
				{
					hmSendRaw("kick " + it->name + " " + reason + "\nban-ip " + it->ip + " " + reason);
					if (banTime == 0)
					{
						hmSendCommandFeedback(client,"Permanently ip-banned player " + it->name + " (" + reason + ")");
						hmLog(client.name + " permanently ip-banned player " + it->name + ":" + it->ip + " (" + reason + ")",LOG_BAN,"bans.log");
					}
					else
					{
						if (oFile.is_open())
							oFile<<"ipbanExpire "<<cTime+(banTime*60)<<" "<<it->name<<endl;
						handle.createTimer("unban",banTime*60,"ipbanExpire",data2str("ipbanExpire %lu %s",cTime,it->name.c_str()));
						hmLog(data2str("%s ip-banned player %s (%s) for %i minute(s) (%s)",client.name.c_str(),it->name.c_str(),it->ip.c_str(),banTime,reason.c_str()),LOG_BAN,"bans.log");
						hmSendCommandFeedback(client,data2str("IP-Banned player %s for %i minute(s) (%s)",it->name.c_str(),banTime,reason.c_str()));
					}
				}
			}
		}
		if (oFile.is_open())
			oFile.close();
		else
			hmOutDebug(handle.getPlugin() + ": Cannot open file './halfMod/plugins/admincmds/expirations.txt'");
	}
	return 0;
}

int unbanPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target>");
	else
	{
		hmSendRaw("pardon " + args[1]);
		hmSendCommandFeedback(client,"Lifted the ban on player " + args[1]);
		hmLog(client.name + " unbanned player " + args[1],LOG_BAN,"bans.log");
	}
	return 0;
}

int unbanIP(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target|IP>");
	else
	{
		if (regex_match(args[1],regex("([0-9]{1,3}\\.){3}[0-9]{1,3}")))
		{
			hmSendRaw("pardon-ip " + args[1]);
			hmSendCommandFeedback(client,"Lifted the ban on ip " + args[1]);
			hmLog(client.name + " unbanned ip " + args[1],LOG_BAN,"bans.log");
		}
		else
		{
			hmPlayer temp = hmGetPlayerData(args[1]);
			if (temp.ip == "")
				hmReplyToClient(client,args[0] + ": Unknown player (" + args[1] + ")");
			else
			{
				hmSendRaw("pardon-ip " + temp.ip);
				hmSendCommandFeedback(client,"Lifted the ip-ban on player " + args[1]);
				hmLog(client.name + " unbanned ip " + temp.ip + " via player ID (" + args[1] + ")",LOG_BAN,"bans.log");
			}
		}
	}
	return 0;
}

int opPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target>");
	else
	{
		vector<hmPlayer> targs;
		int targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
		if (targn < 1)
			targs.push_back({ args[1],0,"","",0,0,"",0,"","" });
		for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
		{
			hmSendRaw("op " + it->name);
			hmSendCommandFeedback(client,"Granted operator status for player " + it->name);
			hmLog(client.name + " Granted operator status for player " + it->name,LOG_OP,"op.log");
		}
	}
	return 0;
}

int deopPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
	if (argc < 2)
		hmReplyToClient(client,"Usage: " + args[0] + " <target>");
	else
	{
		vector<hmPlayer> targs;
		int targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
		if (targn < 1)
			targs.push_back({ args[1],0,"","",0,0,"",0,"","" });
		for (vector<hmPlayer>::iterator it = targs.begin(), ite = targs.end();it != ite;++it)
		{
			hmSendRaw("deop " + it->name);
			hmSendCommandFeedback(client,"Revoked operator status for player " + it->name);
			hmLog(client.name + " Revoked operator status for player " + it->name,LOG_OP,"op.log");
		}
	}
	return 0;
}

int banExpire(hmHandle &handle, string args)
{
	string victim = gettok(args,3," "), line, buffer;
	fstream file ("./halfMod/plugins/admincmds/expirations.txt",ios_base::in);
	if (file.is_open())
	{
		while (getline(file,line))
			if (line != args)
				buffer = buffer + line + "\n";
		file.close();
		file.open("./halfMod/plugins/admincmds/expirations.txt",ios_base::out|ios_base::trunc);
		file<<buffer;
		file.close();
	}
	hmSendRaw("pardon " + victim);
	hmSendCommandFeedback("#SERVER","Ban on " + victim + " has expired.");
	hmLog("Expired ban: " + victim,LOG_BAN,"bans.log");
	return 1;
}

int ipbanExpire(hmHandle &handle, string args)
{
	string victim = gettok(args,3," "), line, buffer;
	fstream file ("./halfMod/plugins/admincmds/expirations.txt",ios_base::in);
	if (file.is_open())
	{
		while (getline(file,line))
			if (line != args)
				buffer = buffer + line + "\n";
		file.close();
		file.open("./halfMod/plugins/admincmds/expirations.txt",ios_base::out|ios_base::trunc);
		file<<buffer;
		file.close();
	}
	if (regex_match(victim,regex("([0-9]{1,3}\\.){3}[0-9]{1,3}")))
	{
		hmSendRaw("pardon-ip " + victim);
		hmSendCommandFeedback("#SERVER","IP-Ban on " + victim + " has expired.");
		hmLog("Expired ip-ban: " + victim,LOG_BAN,"bans.log");
	}
	else
	{
		hmPlayer temp = hmGetPlayerData(victim);
		if (temp.ip == "")
		{
			hmSendRaw("pardon-ip " + victim);
			hmSendCommandFeedback("#SERVER","IP-Ban on " + victim + " has expired.");
			hmLog("Expired ip-ban: " + victim + ". Unknown user/ip.",LOG_BAN,"bans.log");
		}
		else
		{
			hmSendRaw("pardon-ip " + temp.ip);
			hmSendCommandFeedback("#SERVER","IP-Ban on player " + temp.name + " has expired.");
			hmLog("Expired ip-ban: " + temp.ip + " via player ID (" + victim + ")",LOG_BAN,"bans.log");
		}
	}
	return 1;
}

int addTime(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if ((argc < 2) || (!stringisnum(args[1],1)))
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <value> - value must be a positive integer greater than 0.");
        return 1;
    }
    hmSendRaw("time add " + args[1]);
    hmReplyToClient(client,"Added " + args[1] + " to the current world time.");
    return 0;
}

int setTime(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if ((argc < 2) || ((!stringisnum(args[1],0)) && (!isin("day night",args[1]))))
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <value> - value must be 'day', 'night', or a positive integer.");
        return 1;
    }
    hmSendRaw("time set " + args[1]);
    hmReplyToClient(client,"Set the time to " + args[1] + ".");
    return 0;
}

int changeWeather(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <clear|rain|thunder> [duration]");
        return 1;
    }
    if ((argc > 2) && (stringisnum(args[2],0)))
    {
        hmSendRaw("weather " + args[1] + " " + args[2]);
        if (args[1] == "clear")
            hmReplyToClient(client,"Clearing the weather for " + args[2] + " seconds.");
        else
            hmReplyToClient(client,"Changing the weather to " + args[1] + " for " + args[2] + " seconds.");
    }
    else
    {
        hmSendRaw("weather " + args[1]);
        if (args[1] == "clear")
            hmReplyToClient(client,"Clearing the weather.");
        else
            hmReplyToClient(client,"Changing the weather to " + args[1] + ".");
    }
    return 0;
}

/*int setGamerule(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <gamerule> [value]");
        return 1;
    }
    if (argc < 3)
    {
        handle.hookPattern(client,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Gamerule (" + args[1] + ") is currently set to: (.*)$","getGamerule");
        hmSendRaw("gamerule " + args[1]);
    }
    else
    {
        string arg = "";
        for (int i = 2;i < argc;i++)
            arg = addtok(arg,args[i]," ");
        hmSendRaw("gamerule " + args[1] + " " + arg);
        hmReplyToClient(client,"Attempted to set gamerule '" + args[1] + "' to '" + arg + "'.");
    }
    return 0;
}

int getGamerule(hmHandle &handle, hmHook hook, smatch args)
{
    hmReplyToClient(hook.name,"Gamerule '" + args[1].str() + "' = '" + args[2].str() + "'.");
    handle.unhookPattern(hook.name);
    return 1;
}*/

int setGamemode(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <mode> [target]");
        return 1;
    }
    int targn;
    vector<hmPlayer> targs;
    if (argc > 2)
    {
	    targn = hmProcessTargets(client,args[2],targs,FILTER_NAME|FILTER_NO_SELECTOR);
	    if (targn < 1)
	    {
		    hmReplyToClient(client,"No matching players found.");
		    return 2;
	    }
    }
    else
        targs.push_back(client);
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        hmSendRaw("gamemode " + args[1] + " " + stripFormat(it->name));
        hmSendCommandFeedback(client,"Changed the gamemode of player " + it->name + " to " + args[1] + ".");
    }
    return 0;
}

int bringPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <target>");
        return 1;
    }
    int targn;
    vector<hmPlayer> targs;
    targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
    if (targn < 1)
    {
	    hmReplyToClient(client,"No matching players found.");
	    return 2;
    }
    string name;
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        name = stripFormat(it->name);
        hmSendRaw("execute as " + stripFormat(client.name) + " at @s run teleport " + name + " ^ ^1.66 ^2");
        hmSendCommandFeedback(client,"Teleported " + it->name + ".");
    }
    return 0;
}

int toggleNoclip(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    vector<hmPlayer> targs;
    int targn;
    if (argc < 2)
    {
        targn = 1;
        targs.push_back(client);
    }
    else
        targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
    string name;
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        name = stripFormat(it->name);
        /*
        hmSendRaw("scoreboard players tag @a[name=" + name + ",gamemode=survival] add hmNoclip0",false);
        hmSendRaw("scoreboard players tag @a[name=" + name + ",gamemode=creative] add hmNoclip1",false);
        hmSendRaw("scoreboard players tag @a[name=" + name + ",gamemode=adventure] add hmNoclip2",false);
        hmSendRaw("scoreboard players tag @a[name=" + name + ",gamemode=!spectator] add hmNoclipping",false);
        */
        hmSendRaw("tag @a[name=" + name + ",gamemode=survival] add hmNoclip0",false);
        hmSendRaw("tag @a[name=" + name + ",gamemode=creative] add hmNoclip1",false);
        hmSendRaw("tag @a[name=" + name + ",gamemode=adventure] add hmNoclip2",false);
        hmSendRaw("tag @a[name=" + name + ",gamemode=!spectator] add hmNoclipping",false);
        hmSendRaw("gamemode spectator @a[name=" + name + ",tag=hmNoclipping]",false);
        hmSendRaw("gamemode survival @a[name=" + name + ",tag=!hmNoclipping,tag=hmNoclip0]",false);
        hmSendRaw("gamemode creative @a[name=" + name + ",tag=!hmNoclipping,tag=hmNoclip1]",false);
        hmSendRaw("gamemode adventure @a[name=" + name + ",tag=!hmNoclipping,tag=hmNoclip2]",false);
        /*
        hmSendRaw("scoreboard players tag @a[name=" + name + ",tag=!hmNoclipping] remove hmNoclip0",false);
        hmSendRaw("scoreboard players tag @a[name=" + name + ",tag=!hmNoclipping] remove hmNoclip1",false);
        hmSendRaw("scoreboard players tag @a[name=" + name + ",tag=!hmNoclipping] remove hmNoclip2",false);
        hmSendRaw("scoreboard players tag @a[name=" + name + ",tag=hmNoclipping] remove hmNoclipping",false);
        */
        hmSendRaw("tag @a[name=" + name + ",tag=!hmNoclipping] remove hmNoclip0",false);
        hmSendRaw("tag @a[name=" + name + ",tag=!hmNoclipping] remove hmNoclip1",false);
        hmSendRaw("tag @a[name=" + name + ",tag=!hmNoclipping] remove hmNoclip2",false);
        hmSendRaw("tag @a[name=" + name + ",tag=hmNoclipping] remove hmNoclipping",false);
        hmSendCommandFeedback(client,"Has toggled noclip on " + it->name);
    }
    return 0;
}

int toggleDrought(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    static bool enabled = false;
    enabled = !enabled;
    if (enabled)
    {
        handle.createTimer("drought",299,"timerDrought");
        hmSendRaw("weather clear 300",false);
        hmSendCommandFeedback(client,"Will this drought ever end?");
    }
    else
    {
        handle.killTimer("drought");
        hmSendRaw("weather rain 30",false);
        hmSendCommandFeedback(client,"The gods have answered our prayers of rain!");
    }
    return 0;
}

int timerDrought(hmHandle &handle, string args)
{
    hmSendRaw("weather clear 300",false);
    return 0;
}

int toggleFlood(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    static bool enabled = false;
    enabled = !enabled;
    if (enabled)
    {
        handle.createTimer("flood",299,"timerFlood");
        hmSendRaw("weather rain 300",false);
        hmSendCommandFeedback(client,"Will this flood never let up?");
    }
    else
    {
        handle.killTimer("flood");
        hmSendRaw("weather clear 30",false);
        hmSendCommandFeedback(client,"The sun has finally appeared from behind the clouds!");
    }
    return 0;
}

int timerFlood(hmHandle &handle, string args)
{
    hmSendRaw("weather rain 300",false);
    return 0;
}

int smitePlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <target>");
        return 1;
    }
    int targn;
    vector<hmPlayer> targs;
    targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
    if (targn < 1)
    {
	    hmReplyToClient(client,"No matching players found.");
	    return 2;
    }
    string name;
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        name = stripFormat(it->name);
        hmSendRaw("execute at " + name + " run summon minecraft:lightning_bolt");
        handle.createTimer("smite" + name,0,"delayedKill",name);
        hmSendCommandFeedback(client,"Smote " + it->name + ".");
    }
    return 0;
}

int delayedKill(hmHandle &handle, string client)
{
    hmSendRaw("kill " + client);
    return 1;
}

int rocketPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <target>");
        return 1;
    }
    int targn;
    vector<hmPlayer> targs;
    targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
    if (targn < 1)
    {
	    hmReplyToClient(client,"No matching players found.");
	    return 2;
    }
    string name;
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        name = stripFormat(it->name);
        hmSendRaw("effect give " + name + " minecraft:levitation 5 25");
        handle.createTimer("explode" + name,3,"delayedExplode",name);
        hmSendCommandFeedback(client,"Launched " + it->name + " into space.");
    }
    return 0;
}

int delayedExplode(hmHandle &handle, string client)
{
    hmSendRaw("execute at " + client + " run summon minecraft:creeper ~ ~ ~ {Fuse:0,ignited:1}");
    handle.createTimer("kill" + client,0,"delayedKill",client);
    return 1;
}

int slayPlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <target>");
        return 1;
    }
    int targn;
    vector<hmPlayer> targs;
    targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
    if (targn < 1)
    {
	    hmReplyToClient(client,"No matching players found.");
	    return 2;
    }
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        hmSendRaw("kill " + stripFormat(it->name));
        hmSendCommandFeedback(client,"Slayed " + it->name + ".");
    }
    return 0;
}

int explodePlayer(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <target>");
        return 1;
    }
    int targn;
    vector<hmPlayer> targs;
    targn = hmProcessTargets(client,args[1],targs,FILTER_NAME|FILTER_NO_SELECTOR);
    if (targn < 1)
    {
	    hmReplyToClient(client,"No matching players found.");
	    return 2;
    }
    for (auto it = targs.begin(), ite = targs.end();it != ite;++it)
    {
        delayedExplode(handle,stripFormat(it->name));
        hmSendCommandFeedback(client,"Exploded " + it->name + ".");
    }
    return 0;
}

}

