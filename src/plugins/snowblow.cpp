#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <sys/stat.h>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION	"v0.0.4"

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
	// THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
	recallGlobal(global);
	// ^^^^ THIS ONE ^^^^
	
	handle.pluginInfo(	"Snowgun",
				"nigel",
				"Snowballs become arrows!",
				VERSION,
				"http://snowballs.justca.me/in/your/face/"	);
	handle.hookPattern("snowballFound","^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[Snowball: Entity data updated to: (.*)\\]$","catchSnowball");
//          Name of the hook ^                                        ^ Regex pattern to match ^                      	function name to call when matched ^
}


// Generate mcfunction files if missing/corrupt
int onWorldInit(hmHandle &handle, smatch args)
{
	hmGlobal *global;
	global = recallGlobal(global);
	string path = "./" + strremove(global->world,"\"") + "/data/functions/hm/";
	mkdirIf(path.c_str());
	fstream file;
	file.open(path + "handle.mcfunction",ios_base::in);
	bool needWrite = true;
	bool reload = false;
	string line, lines;
	if (file.is_open())
	{
		while (getline(file,line))
		{
			if (line == "execute @e[type=minecraft:snowball,tag=!handled,c=1] ~ ~ ~ function hm:snowgun unless @e[tag=snowblow]")
			{
				needWrite = false;
				break;
			}
		}
		file.close();
	}
	if (needWrite)
	{
		file.open(path + "handle.mcfunction",ios_base::out|ios_base::app);
		if (file.is_open())
		{
			reload = true;
			file<<"\nexecute @e[type=minecraft:snowball,tag=!handled,c=1] ~ ~ ~ function hm:snowgun unless @e[tag=snowblow]";
			file.close();
		}
	}
	file.open(path + "snowgun.mcfunction",ios_base::in);
	lines.clear();
	if (file.is_open())
	{
		while (getline(file,line))
			lines = addtok(lines,line,"\n");
		file.close();
	}
	if (lines != "entitydata @e[type=minecraft:snowball,tag=!snowblow] {Tags:[\"snowblow\"]}\nkill @e[tag=snowblow]")
	//if (lines != "entitydata @s {Tags:[\"snowblow\"]}")
	{
		file.open(path + "snowgun.mcfunction",ios_base::out|ios_base::trunc);
		if (file.is_open())
		{
			reload = true;
			file<<"entitydata @e[type=minecraft:snowball,tag=!snowblow] {Tags:[\"snowblow\"]}\nkill @e[tag=snowblow]";
			//file<<"entitydata @s {Tags:[\"snowblow\"]}";
			file.close();
		}
	}
	hmSendRaw("gamerule gameLoopFunction hm:handle");
	if (reload)
		hmSendRaw("reload");
	return 0;
}

int catchSnowball(hmHandle &handle, hmHook hook, smatch args)
{
	string rep = args[1];
	regex ptrn ("UUID(Least|Most):.+?[,\\b]");
	rep = regex_replace(rep,ptrn,"");
//	ptrn = "(PortalCooldown):[0-9]{1,}";
//	rep = regex_replace(rep,ptrn,"\\1:100");
	ptrn = "\\bsnowblow\\b";
	rep = regex_replace(rep,ptrn,"arrowball\",\"handled");
	ptrn = "^\\{";
	rep = regex_replace(rep,ptrn,"{Color:-1,damage:2.0d,pickup:0b,");
	ptrn = "Motion:\\[(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+)d\\]";
	smatch ml;
	string motion;
	if (regex_search(rep,ml,ptrn))
	{
		motion = data2str("Motion:[%.17fd,%.17fd,%.17fd]",atof(string(ml[1]).c_str())*2.5,atof(string(ml[2]).c_str())*2.5,atof(string(ml[3]).c_str())*2.5);
		rep = regex_replace(rep,ptrn,motion);
		//hmSendRaw("entitydata @e[tag=snowblow,c=1] {" + motion + ",Tags:[\"handled\"]}");
	}
	string pos;
	ptrn = "Pos:\\[(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+)d,(-?[0-9]+\\.[0-9]+d)\\]";
	if (regex_search(rep,ml,ptrn))
		pos = string(ml[1]) + " " + string(ml[2]) + " " + string(ml[3]);
	//ptrn = "\"";
	//hmSendRaw("summon minecraft:arrow " + pos + " " + regex_replace(rep,ptrn,"\\\""));
	hmSendRaw("summon minecraft:arrow " + pos + " " + rep);
	return 1;
}

}

