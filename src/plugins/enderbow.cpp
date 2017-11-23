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

#define VERSION		"v0.0.4"

bool enabled = false;

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
	// THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
	recallGlobal(global);
	// ^^^^ THIS ONE ^^^^
	
	handle.pluginInfo(	"Ender Bows",
				"nigel",
				"Shoot ender pearls from your bow.",
				VERSION,
				"http://high.speed.pearls.justca.me/from/your/bow"	);
	handle.regAdminCmd("hm_enderbow","ebToggle",FLAG_CVAR,"Enable/Disable enderbows.");
}

// Generate mcfunction files if missing/corrupt
int onWorldInit(hmHandle &handle, smatch args)
{
	hmGlobal *global;
	global = recallGlobal(global);
	if (enabled)
	    hmSendRaw("scoreboard objectives add hmUseEndBow stat.useItem.minecraft.bow");
	else
	    hmSendRaw("scoreboard objectives remove hmUseEndBow");
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
			if (line == "execute @a[score_hmUseEndBow_min=1] ~ ~ ~ function hm:enderbow")
			    needWrite -= 1;
			else if (line == "scoreboard players reset @a[score_hmUseEndBow_min=1] hmUseEndBow")
			    needWrite -= 2;
		    else
		        lines = appendtok(lines,line,"\n");
		}
		file.close();
	}
	if (needWrite & 1)
	    lines = appendtok(lines,"execute @a[score_hmUseEndBow_min=1] ~ ~ ~ function hm:enderbow","\n");
    if (needWrite & 2)
	    lines = appendtok(lines,"scoreboard players reset @a[score_hmUseEndBow_min=1] hmUseEndBow","\n");
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
	file.open(path + "enderbow.mcfunction",ios_base::in);
	lines.clear();
	if (file.is_open())
	{
		while (getline(file,line))
			lines = appendtok(lines,line,"\n");
		file.close();
	}
	if (lines != "entitydata @e[type=minecraft:arrow,tag=!handled,c=1] {Tags:[\"endarrow\"]}\nkill @e[tag=endarrow]")
	{
		file.open(path + "enderbow.mcfunction",ios_base::out|ios_base::trunc);
		if (file.is_open())
		{
			reload = true;
			file<<"entitydata @e[type=minecraft:arrow,tag=!handled,c=1] {Tags:[\"endarrow\"]}\nkill @e[tag=endarrow]";
			file.close();
		}
	}
	hmSendRaw("gamerule gameLoopFunction hm:handle");
	if (reload)
		hmSendRaw("reload");
	return 0;
}

int ebToggle(hmHandle &handle, string client, string args[], int argc)
{
    enabled = !enabled;
    if (enabled)
    {
        hmSendRaw("scoreboard objectives add hmUseEndBow stat.useItem.minecraft.bow");
        handle.hookPattern("ebows","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(.+): Entity data updated to: (.*endarrow.*)\\]$","catchArrow");
        hmSendCommandFeedback(client,"Enabled Ender bows.");
    }
    else
    {
    hmSendRaw("scoreboard objectives remove hmUseEndBow");
        handle.unhookPattern("ebows");
        hmSendCommandFeedback(client,"Disabled Ender bows.");
    }
    return 0;
}

int catchArrow(hmHandle &handle, hmHook &hook, smatch args)
{
    string client = args[1], nbt = args[2];
    smatch ml;
    string pos;
	regex ptrn ("Pos:\\[(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+d)\\]");
	if (regex_search(nbt,ml,ptrn))
		pos = ml[1].str() + " " + ml[2].str() + " " + ml[3].str();
	ptrn = "Motion:\\[(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+)d\\]";
	string motion;
	if (regex_search(nbt,ml,ptrn))
		motion = data2str("Motion:[%.17fd,%.17fd,%.17fd]",stof(ml[1].str()),stof(ml[2].str()),stof(ml[3].str()));
	hmSendRaw("summon minecraft:ender_pearl " + pos + " {" + motion + ",ownerName:\"" + client + "\",Tags:[\"handled\"]}");
    return 1;
}

}
