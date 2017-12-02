#include <iostream>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <regex>
#include <fstream>
#include <ctime>
#include "str_tok.h"
#include "halfmod.h"
using namespace std;

int mcVerInt(string version)
{
    static string oldVer = version;
    static int verInt = 0;
    if ((oldVer != version) || (verInt == 0))
    {
        oldVer = version;
        verInt = 0;
        int p = 7;
        if (oldVer.find("w") != string::npos)
            verInt = (stoi(gettok(oldVer,1,"w"))*1000000) + (stoi(gettok(oldVer,2,"w").substr(0,oldVer.size()-1))*1000) + (int(oldVer.at(oldVer.size()-1)-96));
        else for (int i = 1, j = numtok(oldVer,".");i <= j;i++)
            verInt += stoi(gettok(oldVer,i,".") + strrep("0",p-(i*2)));
    }
    return verInt;
}

void mkdirIf(const char *path)
{
	struct stat dir;
	stat(path,&dir);
	if (!S_ISDIR(dir.st_mode))
		mkdir(path,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
}

hmHandle::hmHandle()
{
	loaded = false;
}

hmHandle::hmHandle(string pluginPath, hmGlobal *global)
{
	loaded = false;
	load(pluginPath,global);
}

/*hmHandle::~hmHandle()
{
	hooks.clear();
	events.clear();
	cmds.clear();
	timers.clear();
}*/

void hmHandle::unload()
{
	if (loaded)
	{
		void (*end)(hmHandle&);
		*(void **) (&end) = dlsym(module, HM_ONPLUGINEND_FUNC);
		if (dlerror() == NULL)
			(*end)(*this);
		loaded = false;
		dlclose(module);
	}
}

bool hmHandle::load(string pluginPath, hmGlobal *global)
{
	if (!loaded)
	{
		module = dlopen(pluginPath.c_str(), RTLD_LAZY);
		if (!module)
			cerr<<"Error loading plugin \""<<pluginPath<<"\" "<<dlerror()<<endl;
		else
		{
			modulePath = pluginPath;
			char *error;
			void (*start)(hmHandle&,hmGlobal*);
			*(void **) (&start) = dlsym(module, HM_ONPLUGINSTART_FUNC);
			if ((error = dlerror()) != NULL)
				fputs(error, stderr);
			else
			{
				(*start)(*this,global);
				loaded = true;
				pluginName = deltok(deltok(deltok(deltok(modulePath,1,"/"),1,"/"),1,"/"),-1,".");
				hookEvent(HM_ONCONNECT,HM_ONCONNECT_FUNC);
				hookEvent(HM_ONAUTH,HM_ONAUTH_FUNC);
				hookEvent(HM_ONJOIN,HM_ONJOIN_FUNC);
				hookEvent(HM_ONWARN,HM_ONWARN_FUNC);
				hookEvent(HM_ONINFO,HM_ONINFO_FUNC);
				hookEvent(HM_ONSHUTDOWN,HM_ONSHUTDOWN_FUNC);
				hookEvent(HM_ONDISCONNECT,HM_ONDISCONNECT_FUNC);
				hookEvent(HM_ONPART,HM_ONPART_FUNC);
				hookEvent(HM_ONADVANCE,HM_ONADVANCE_FUNC);
				hookEvent(HM_ONGOAL,HM_ONGOAL_FUNC);
				hookEvent(HM_ONCHALLENGE,HM_ONCHALLENGE_FUNC);
				hookEvent(HM_ONACTION,HM_ONACTION_FUNC);
				hookEvent(HM_ONTEXT,HM_ONTEXT_FUNC);
				hookEvent(HM_ONFAKETEXT,HM_ONFAKETEXT_FUNC);
				hookEvent(HM_ONDEATH,HM_ONDEATH_FUNC);
				hookEvent(HM_ONWORLDINIT,HM_ONWORLDINIT_FUNC);
				hookEvent(HM_ONHSCONNECT,HM_ONHSCONNECT_FUNC);
				hookEvent(HM_ONHSDISCONNECT,HM_ONHSDISCONNECT_FUNC);
			}
		}
	}
	return loaded;
}

string hmHandle::getAPI()
{
	return API_VER;
}

bool hmHandle::isLoaded()
{
	return loaded;
}

void hmHandle::pluginInfo(string name, string author, string description, string version, string url)
{
	info.name = name;
	info.author = author;
	info.description = description;
	info.version = version;
	info.url = url;
	API_VER = API_VERSION;
}

hmInfo hmHandle::getInfo()
{
	return info;
}

int hmHandle::hookEvent(int event, string function)
{
	hmEvent tmpEvent;
	*(void **) (&tmpEvent.func) = dlsym(module, function.c_str());
	if (dlerror() == NULL)
	{
		tmpEvent.event = event;
		events.push_back(tmpEvent);
		return events.size();
	}
	return -1;
}

int hmHandle::regAdminCmd(string command, string function, int flags, string description)
{
	hmCommand tmpCommand;
	*(void **) (&tmpCommand.func) = dlsym(module, function.c_str());
	if (dlerror() == NULL)
	{
		tmpCommand.cmd = command;
		tmpCommand.flag = flags;
		tmpCommand.desc = description;
		cmds.push_back(tmpCommand);
		return cmds.size();
	}
	return -1;
}

int hmHandle::regConsoleCmd(string command, string function, string description)
{
	hmCommand tmpCommand;
	*(void **) (&tmpCommand.func) = dlsym(module, function.c_str());
	if (dlerror() == NULL)
	{
		tmpCommand.cmd = command;
		tmpCommand.flag = 0;
		tmpCommand.desc = description;
		cmds.push_back(tmpCommand);
		return cmds.size();
	}
	return -1;
}

int hmHandle::unregCmd(string command)
{
	for (auto it = cmds.begin();it != cmds.end();)
	{
		if (it->cmd == command)
			it = cmds.erase(it);
		else
			++it;
	}
	return cmds.size();
}

int hmHandle::hookPattern(string name, string pattern, string function)
{
	hmHook tmpHook;
	*(void **) (&tmpHook.func) = dlsym(module, function.c_str());
	if (dlerror() == NULL)
	{
		tmpHook.name = name;
		tmpHook.ptrn = pattern;
		hooks.push_back(tmpHook);
		return hooks.size();
	}
	return -1;
}

int hmHandle::unhookPattern(string name)
{
	for (auto it = hooks.begin();it != hooks.end();)
	{
		if (it->name == name)
			it = hooks.erase(it);
		else
			++it;
	}
	return hooks.size();
}

hmCommand hmHandle::findCmd(string command)
{
	for (auto it = cmds.begin(), ite = cmds.end();it != ite;++it)
		if (it->cmd == command)
			return *it;
	return hmCommand();
}

hmHook hmHandle::findHook(string name)
{
	for (auto it = hooks.begin(), ite = hooks.end();it != ite;++it)
		if (it->name == name)
			return *it;
	return hmHook();
}

hmEvent hmHandle::findEvent(int event)
{
	for (auto it = events.begin(), ite = events.end();it != ite;++it)
		if (it->event == event)
			return *it;
	return hmEvent();
}

string hmHandle::getPath()
{
	return modulePath;
}

int hmHandle::createTimer(string name, time_t interval, string function, string args)
{
	hmTimer temp;
	*(void **) (&temp.func) = dlsym(module, function.c_str());
	if (dlerror() == NULL)
	{
		temp.name = name;
		temp.interval = interval;
		temp.args = args;
		temp.last = time(NULL);
		timers.push_back(temp);
		invalidTimers = true;
		return timers.size();
	}
	return -1;
}

int hmHandle::killTimer(string name)
{
    for (vector<hmTimer>::iterator it = timers.begin();it != timers.end();)
    {
        if (it->name == name)
        {
            it = timers.erase(it);
            invalidTimers = true;
        }
        else
            it++;
    }
    return timers.size();
}

int hmHandle::triggerTimer(string name)
{
    int count = 0;
    for (vector<hmTimer>::iterator it = timers.begin(), ite = timers.end();it != ite;++it)
    {
        if (it->name == name)
        {
            it->last = it->interval*-1;
            count++;
        }
    }
    return count;
}

hmTimer hmHandle::findTimer(string name)
{
    for (vector<hmTimer>::iterator it = timers.begin(), ite = timers.end();it != ite;++it)
        if (it->name == name)
            return *it;
    return hmTimer();
}

string hmHandle::getPlugin()
{
	return pluginName;
}

int hmHandle::totalCmds()
{
	return cmds.size();
}

int hmHandle::totalEvents()
{
	return events.size();
}

hmGlobal *recallGlobal(hmGlobal *global)
{
	static hmGlobal *globe = global;
	return globe;
}

void hmSendRaw(string raw, bool output)
{
    hmGlobal *global;
	int socket = recallGlobal(global)->hsSocket;
	if (socket)
	{
		if (output)
			hmOutQuiet(raw);
		raw += "\r";
		write(socket,raw.c_str(),raw.size());
	}
	else
		hmOutDebug("Error: No connection to the halfShell server . . .");
}

void hmReplyToClient(string client, string message)
{
    hmGlobal *global;
	int ver = mcVerInt(recallGlobal(global)->mcVer);
	string com = "tellraw ", pre = " [\"[HM] ", suf = "\"]";
	if (((ver <= 13003700) && (ver > 1000000)) || (ver < 107020))
	{
	    com = "tell ";
	    pre = " [HM] ";
	    suf = "";
    }
    else
		hmSendRaw(com + stripFormat(client) + pre + message + suf,false);
	if ((client == "") || (client == "0") || (client == "#SERVER"))
		cout<<"[HM] "<<message<<endl;
}

void hmSendCommandFeedback(string client, string message)
{
	hmGlobal *global;
	global = recallGlobal(global);
	int ver = mcVerInt(global->mcVer);
	string com = "tellraw ", pre = " [\"[HM] ", suf = "\"]";
	if (((ver <= 13003700) && (ver > 1000000)) || (ver < 107020))
	{
	    com = "tell ";
	    pre = " [HM] ";
	    suf = "";
    }
	cout<<"[HM] " + client + ": " + message<<endl;
	if (global->hsSocket)
	{
		string strippedClient = stripFormat(lower(client)), strippedTarget;
		for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
		{
			strippedTarget = stripFormat(lower(it->name));
			if (strippedTarget == strippedClient)
				hmSendRaw(com + strippedClient + pre + message + suf,false);
			else if (it->flags & FLAG_ADMIN)
				hmSendRaw(com + strippedTarget + pre + client + ": " + message + suf,false);
			else
				hmSendRaw(com + strippedTarget + pre + "ADMIN: " + message + suf,false);
		}
	}
	else
		hmOutDebug("Error: No connection to the halfShell server . . .");
}

void hmSendMessageAll(string message)
{
    hmGlobal *global;
	int ver = mcVerInt(recallGlobal(global)->mcVer);
	cout<<"[HM] " + message<<endl;
	if ((ver > 13003700) || ((ver >= 107020) && (ver < 1000000)))
    	hmSendRaw("tellraw @a [\"[HM] " + message + "\"]",false);
	else
	    hmSendRaw("say [HM] " + message,false);
}

bool hmIsPlayerOnline(string client)
{
	client = stripFormat(lower(client));
	hmGlobal *global;
	global = recallGlobal(global);
	for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
		if (stripFormat(lower(it->name)) == client)
			return true;
	return false;
}

hmPlayer hmGetPlayerInfo(string client)
{
	client = stripFormat(lower(client));
	hmGlobal *global;
	global = recallGlobal(global);
	for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
		if (stripFormat(lower(it->name)) == client)
			return *it;
	return hmPlayer();
}

string hmGetPlayerUUID(string client)
{
	return hmGetPlayerInfo(stripFormat(lower(client))).uuid;
}
string hmGetPlayerIP(string client)
{
	return hmGetPlayerInfo(stripFormat(lower(client))).ip;
}

int hmGetPlayerFlags(string client)
{
	if (client == "#SERVER")
		return FLAG_ROOT;
	return hmGetPlayerInfo(stripFormat(lower(client))).flags;
}

void hmOutQuiet(string text)
{
	hmGlobal *global;
	if (!recallGlobal(global)->quiet)
		cout<<"[<<] "<<text<<endl;
}

void hmOutVerbose(string text)
{
	hmGlobal *global;
	if (recallGlobal(global)->verbose)
		cout<<"[..] "<<text<<endl;
}

void hmOutDebug(string text)
{
	hmGlobal *global;
	if (recallGlobal(global)->debug)
		cerr<<"[DEBUG] "<<text<<endl;
}

hmPlayer hmGetPlayerData(string client)
{
	hmGlobal *global;
	global = recallGlobal(global);
	string line;
	hmPlayer temp = { client, 0, "", "", 0, 0, "", 0, "", "" };
	client = stripFormat(lower(client));
	for (vector<hmAdmin>::iterator it = global->admins.begin(), ite = global->admins.end();it != ite;++it)
	{
		if (lower(it->client) == client)
		{
			temp.flags = it->flags;
			break;
		}
	}
	ifstream file ("./halfMod/userdata/" + client + ".dat");
	if (file.is_open())
	{
		while (getline(file,line))
		{
			switch (findtok("ip uuid join quit quitmsg death deathmsg",gettok(line,1,"="),1," "))
			{
				case 1: // ip
				{
					temp.ip = deltok(line,1,"=");
					break;
				}
				case 2: // uuid
				{
					temp.uuid = deltok(line,1,"=");
					break;
				}
				case 3: // join time
				{
					temp.join = atoi(gettok(line,2,"=").c_str());
					break;
				}
				case 4: // quit time
				{
					temp.quit = atoi(gettok(line,2,"=").c_str());
					break;
				}
				case 5: // quit message
				{
					temp.quitmsg = deltok(line,1,"=");
					break;
				}
				case 6: // last death time
				{
					temp.death = atoi(gettok(line,2,"=").c_str());
					break;
				}
				case 7: // last death message
				{
					temp.deathmsg = deltok(line,1,"=");
					break;
				}
				default: // custom
				{
					temp.custom = addtok(temp.custom,line,"\n");
				}
			}
		}
		file.close();
	}
	return temp;
}

string stripFormat(string str)
{
	regex ptrn ("(§.)");
	return regex_replace(str,ptrn,"");
}

void hmLog(string data, int logType, string logFile)
{
	hmGlobal *global;
	global = recallGlobal(global);
	if ((global->logMethod & logType) || (logType == LOG_ALWAYS))
	{
		ofstream file ("./halfMod/logs/" + logFile, ios_base::out|ios_base::app);
		if (file.is_open())
		{
			char timestamp[20];
			strftime(timestamp,19,"[%D %T] ",localtime(NULL));
			file<<timestamp<<data<<endl;
			file.close();
		}
	}
}

int hmProcessTargets(string client, string target, vector<hmPlayer> &targList, int filter)
{
	int FLAGS[26] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554431 };
	hmGlobal *global;
	global = recallGlobal(global);
	if (!global->players.size())
	    return 0;
	targList.clear();
	client = stripFormat(lower(client));
	target = stripFormat(lower(target));
	int c = 0, flags = 0;
	if (filter & FILTER_NAME)
	{
		if (((filter & FILTER_NO_SELECTOR) == 0) && (target.at(0) == '@'))
			targList.push_back({ target,0,"","",0,0,"",0,"","" });
		else if ((target.at(0) == '%') && ((filter & FILTER_NO_EVAL) == 0))
		{
			if (target == "%me")
				c = 1;
			else if (target == "%!me")
				c = 2;
			else if (target == "%all")
				c = 3;
			else if (target.at(1) == 'f')
			{
			    if (target.at(2) == '!')
			        c = 5;
			    else
				    c = 4;
				for (auto it = target.begin()+(c-2), ite = target.end();it != ite;++it)
					flags |= FLAGS[*it-97];
			}
			else if (target.at(1) == 'r')
			{
				if (target == "%r!")
					c = 6;
				else
				{
					targList.push_back(global->players[rand() % int(global->players.size())]);
					return 1;
				}
			}
		}
	}
	else if (filter & FILTER_FLAG)
	{
		if (target.at(1) == '!')
	        c = 5;
	    else
		    c = 4;
		for (auto it = target.begin()+(c-4), ite = target.end();it != ite;++it)
			flags |= FLAGS[*it-97];
	}
	vector<hmPlayer> temp;
	for (vector<hmPlayer>::iterator it = global->players.begin(), ite = global->players.end();it != ite;++it)
	{
	    if ((filter & FILTER_UUID) && (it->uuid == target))
			targList.push_back(*it);
	    else if ((filter & FILTER_IP) && (it->ip == target))
	        targList.push_back(*it);
	    else switch (c)
		{
			case 1:
			{
				if (stripFormat(lower(it->name)) == client)
					targList.push_back(*it);
				break;
			}
			case 2:
			{
				if (stripFormat(lower(it->name)) != client)
					targList.push_back(*it);
				break;
			}
			case 3:
			{
				targList.push_back(*it);
				break;
			}
			case 4:
			{
				if (it->flags & flags)
					targList.push_back(*it);
				break;
			}
			case 5:
			{
				if ((it->flags & flags) == 0)
					targList.push_back(*it);
				break;
			}
			case 6:
			{
				if (stripFormat(lower(it->name)) != client)
					temp.push_back(*it);
				break;
			}
			default:
			{
				if (isin(stripFormat(lower(it->name)),target))
					targList.push_back(*it);
			}
		}
	}
	if ((c == 6) && (temp.size() > 0))
		targList.push_back(temp[rand() % int(temp.size())]);
	return targList.size();
}











