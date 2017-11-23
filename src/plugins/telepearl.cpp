#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <sys/stat.h>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION	"v0.0.1"

string targets;

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
	// THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
	recallGlobal(global);
	// ^^^^ THIS ONE ^^^^
	
	handle.pluginInfo(	"Almighty Telepearl",
				"nigel",
				"Summon others at will with your ender pearls.",
				VERSION,
				"http://telepearl.made.you.justca.me/"	);
	handle.regAdminCmd("hm_telepearl","setPlayerTPearl",FLAG_SLAY);
	handle.hookPattern("pearlTagged","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(.+): Tag hmPearlObj added\\]$","catchPlayerTPearl");
}

int onWorldInit(hmHandle &handle, smatch args)
{
	hmGlobal *global;
	global = recallGlobal(global);
	hmSendRaw("scoreboard objectives add hmPearl stat.useItem.minecraft.ender_pearl");
	string path = "./" + strremove(global->world,"\"") + "/data/functions/hm/";
	mkdirIf(path.c_str());
	fstream file;
	file.open(path + "handle.mcfunction",ios_base::in);
	int needWrite = 3;
	bool reload = false;
	string line, lines;
	if (file.is_open())
	{
		while (getline(file,line))
		{
			if (line == "execute @a[tag=hmPearl,score_hmPearl_min=1] ~ ~ ~ function hm:telepearl")
				needWrite -= 1;
			else if (line == "scoreboard players reset @a[score_hmPearl_min=1] hmPearl")
				needWrite -= 2;
			else
				lines = addtok(lines,line,"\n");
		}
		file.close();
	}
	if (needWrite & 1)
		lines = addtok(lines,"execute @a[tag=hmPearl,score_hmPearl_min=1] ~ ~ ~ function hm:telepearl","\n");
	if (needWrite & 2)
		lines = addtok(lines,"scoreboard players reset @a[score_hmPearl_min=1] hmPearl","\n");
	if (needWrite)
	{
		file.open(path + "handle.mcfunction",ios_base::out|ios_base::trunc);
		if (file.is_open())
		{
			reload = true;
			file<<lines;
			file.close();
		}
	}
	file.open(path + "telepearl.mcfunction",ios_base::in);
	needWrite = 1;
	if (file.is_open())
	{
		while (getline(file,line))
		{
			if (line == "scoreboard players tag @e[type=minecraft:ender_pearl,c=1] add hmPearlObj")
				needWrite = 0;
			else
			{
				needWrite = 1;
				break;
			}
		}
		file.close();
	}
	if (needWrite)
	{
		file.open(path + "telepearl.mcfunction",ios_base::out|ios_base::trunc);
		if (file.is_open())
		{
			reload = true;
			file<<"scoreboard players tag @e[type=minecraft:ender_pearl,c=1] add hmPearlObj";
			file.close();
		}
	}
	hmSendRaw("gamerule gameLoopFunction hm:handle");
	if (reload)
		hmSendRaw("reload");
	return 0;
}

int setPlayerTPearl(hmHandle &handle, string client, string args[], int argc)
{
	targets = delwildtok(targets,client + "=*",1," ");
	if (argc < 2)
	{
		hmSendRaw("scoreboard players tag " + client + " remove hmPearl");
		hmReplyToClient(client,"Your Telepearl target has been reset.");
	}
	else
	{
		targets = addtok(targets,client + "=" + args[1]," ");
		hmSendRaw("scoreboard players tag " + client + " add hmPearl");
		hmReplyToClient(client,"Your ender pearls will now summon " + args[1] + "!");
	}
	return 0;
}

int catchPlayerTPearl(hmHandle &handle, hmHook hook, smatch ml)
{
	if (ml.size() > 1)
	{
		string client = ml[1];
		string target = gettok(wildtok(targets,client + "=*",1," "),2,"=");
		if (target != "")
			hmSendRaw("execute " + client + " ~ ~ ~ entitydata @e[tag=hmPearlObj,c=1] {ownerName:\"" + target + "\",Tags:[\"handled\"]}");
	}
	return 0;
}

}

