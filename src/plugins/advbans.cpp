//#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION     "v0.0.7"

#define BLINE       1
#define KLINE       2
#define FLINE       3

struct autoLine
{
    short type;
    string ptrnstr;
    rens::regex pattern;
    time_t cTime;
    int minutes;
    time_t expiration;
    string client;
    string reason;
};

vector<autoLine> alines;

int defBanTime = 0;

void loadLines();
void saveLine(autoLine line);
int removeLine(short type, string ptrn);
void blineUser(hmHandle &handle, autoLine *line, string client, string ip = "");
void klineUser(autoLine *line, string client);

extern "C" {

int onPluginStart(hmHandle &handle,hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Advanced Bans",
                      "nigel",
                      "Ban IP and UUID ranges.",
                      VERSION,
                      "http://look.who.cant.justca.me/back/now");
    handle.hookConVarChange(handle.createConVar("default_ban_time","0","Default number of minutes to ban someone for if no value is provided to the hm_ban command.",0,true,0.0),"banTimeChange");
    handle.regAdminCmd("hm_bline","blineCommand",FLAG_BAN,"Ban a username:uuid@ip mask.");
    handle.regAdminCmd("hm_kline","klineCommand",FLAG_BAN,"Auto kick a username:uuid@ip mask.");
    handle.regAdminCmd("hm_unbline","unblineCommand",FLAG_BAN,"Remove a bline pattern, but not any of the bans it created.");
    handle.regAdminCmd("hm_unkline","unklineCommand",FLAG_BAN,"Remove a kline pattern.");
    mkdirIf("./halfMod/plugins/advbans/");
    loadLines();
    return 0;
}

int banTimeChange(hmConVar &cvar, string oldVal, string newVal)
{
    defBanTime = cvar.getAsInt();
    return 0;
}

/*int onPlayerConnect(hmHandle &handle, smatch args)
{
    string client = args[1].str();
    string ip = args[2].str();
    time_t cTime = time(NULL);
    for (auto it = alines.begin();it != alines.end();)
    {
        if (it->expiration >= cTime)
            alines.erase(it);
        else
        {
            if ((it->type == BLINE) && ((regex_search(client,it->pattern)) || (regex_search(ip,it->pattern))))
                blineUser(handle,it,client,ip);
            ++it;
        }
    }
    return 0;
}*/

int onPlayerAuth(hmHandle &handle, rens::smatch args)
{
    string client = args[2].str();
    string uuid = args[3].str();
    string ip = hmGetPlayerData(client).ip;
    string mask = client + ":" + uuid + "@" + ip;
    time_t cTime = time(NULL);
    for (auto it = alines.begin();it != alines.end();)
    {
        if ((it->expiration > 0) && (it->expiration <= cTime))
            alines.erase(it);
        else
        {
            if ((it->type == BLINE) && (rens::regex_match(mask,it->pattern)))
                blineUser(handle,&*it,client,ip);
            ++it;
        }
    }
    return 0;
}

int onPlayerJoin(hmHandle &handle, rens::smatch args)
{
    string client = args[1].str();
    time_t cTime = time(NULL);
    hmPlayer *info = hmGetPlayerPtr(client);
    string mask = client + ":" + info->uuid + "@" + info->ip;
    for (auto it = alines.begin();it != alines.end();)
    {
        if ((it->expiration > 0) && (it->expiration <= cTime))
            alines.erase(it);
        else
        {
            if ((it->type == KLINE) && (rens::regex_match(mask,it->pattern)))
                klineUser(&*it,client);
            ++it;
        }
    }
    return 0;
}

int blineCommand(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    //hm_bline <pattern> [minutes] [reason]
    if (argc < 2)
        hmReplyToClient(client,"Usage: " + args[0] + " <pattern (username:uuid@ip)> [minutes, 0 = permanent] [reason]");
    else
    {
        int banTime;
        if (argc < 3)
            banTime = defBanTime;
        else if (!stringisnum(args[2],0))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " <pattern (username:uuid@ip)> [minutes, 0 = permanent] [reason]");
            hmReplyToClient(client,"Minutes must be a valid positive integer.");
            return 1;
        }
        else
            banTime = stoi(args[2]);
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
            else
                reason = "B-lined.";
		}
	    time_t cTime = time(NULL);
	    time_t expire = 0;
		if (banTime > 0)
		{
		    expire = cTime+(banTime*60);
		    hmLog("B-LINE: " + client.name + " added a rule for \"" + args[1] + "\" set to expire in " + to_string(banTime) + " minute(s).",LOG_BAN,"bans.log");
	    }
	    else
	        hmLog("B-LINE: " + client.name + " added a rule for \"" + args[1] + "\" permanently.",LOG_BAN,"bans.log");
        #ifdef HM_USE_PCRE2
		autoLine line = { BLINE, args[1], args[1], cTime, banTime, expire, client.name, reason };
		#else
		autoLine line = { BLINE, args[1], regex(args[1]), cTime, banTime, expire, client.name, reason };
		#endif
		saveLine(line);
		alines.push_back(line);
		string mask;
		for (auto it = recallGlobal(NULL)->players.begin(), ite = recallGlobal(NULL)->players.end();it != ite;++it)
		{
		    mask = it->second.name + ":" + it->second.uuid + "@" + it->second.ip;
		    if (rens::regex_search(mask,line.pattern))
		        blineUser(handle,&*alines.rbegin(),it->second.name,it->second.ip);
        }
	}
    return 0;
}

int klineCommand(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    //hm_kline <pattern> [minutes] [reason]
    if (argc < 2)
        hmReplyToClient(client,"Usage: " + args[0] + " <pattern (username:uuid@ip)> [minutes, 0 = permanent] [reason]");
    else
    {
        int banTime;
        if (argc < 3)
            banTime = defBanTime;
        else if (!stringisnum(args[2],0))
        {
            hmReplyToClient(client,"Usage: " + args[0] + " <pattern (username:uuid@ip)> [minutes, 0 = permanent] [reason]");
            hmReplyToClient(client,"Minutes must be a valid positive integer.");
            return 1;
        }
        else
            banTime = stoi(args[2]);
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
            else
                reason = "K-lined.";
		}
	    time_t cTime = time(NULL);
	    time_t expire = 0;
		if (banTime > 0)
		{
		    expire = cTime+(banTime*60);
		    hmLog("K-LINE: " + client.name + " added a rule for \"" + args[1] + "\" set to expire in " + to_string(banTime) + " minute(s).",LOG_BAN,"bans.log");
	    }
	    else
	        hmLog("K-LINE: " + client.name + " added a rule for \"" + args[1] + "\" permanently.",LOG_BAN,"bans.log");
        #ifdef HM_USE_PCRE2
        autoLine line = { KLINE, args[1], args[1], cTime, banTime, expire, client.name, reason };
        #else
		autoLine line = { KLINE, args[1], regex(args[1]), cTime, banTime, expire, client.name, reason };
		#endif
		saveLine(line);
		alines.push_back(line);
		string mask;
		for (auto it = recallGlobal(NULL)->players.begin(), ite = recallGlobal(NULL)->players.end();it != ite;++it)
		{
		    mask = it->second.name + ":" + it->second.uuid + "@" + it->second.ip;
		    if (rens::regex_search(mask,line.pattern))
		        klineUser(&*alines.rbegin(),it->second.name);
        }
	}
    return 0;
}

int unblineCommand(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    //hm_unbline <pattern>
    if (argc < 2)
        hmReplyToClient(client,"Usage: " + args[0] + " <pattern (username:uuid@ip)>");
    else
    {
        int lines = removeLine(BLINE,args[1]);
        if (!lines)
            hmReplyToClient(client,"No matching B-lines found.");
        else if (lines == 1)
            hmReplyToClient(client,"Removed 1 matching B-line.");
        else
            hmReplyToClient(client,"Removed " + to_string(lines) + " matching B-lines.");
    }
    return 0;
}

int unklineCommand(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    //hm_unkline <pattern>
    if (argc < 2)
        hmReplyToClient(client,"Usage: " + args[0] + " <pattern (username:uuid@ip)>");
    else
    {
        int lines = removeLine(KLINE,args[1]);
        if (!lines)
            hmReplyToClient(client,"No matching K-lines found.");
        else if (lines == 1)
            hmReplyToClient(client,"Removed 1 matching K-line.");
        else
            hmReplyToClient(client,"Removed " + to_string(lines) + " matching K-lines.");
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
	if (rens::regex_match(victim,ip_pattern))
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

}

void loadLines()
{
    ifstream file ("./halfMod/plugins/advbans/autolines.conf");
    if (file.is_open())
    {
        string line;
        autoLine temp;
        static rens::regex ptrn ("([0-9]) ([^ ]+) ([0-9]+) ([0-9]+) ([0-9]+) ([^ ]+) ?(.*)");
        rens::smatch ml;
        while (getline(file,line))
        {
            if (rens::regex_match(line,ml,ptrn))
            {
                temp.type = stoi(ml[1].str());
                temp.ptrnstr = ml[2].str();
                #ifdef HM_USE_PCRE2
                temp.pattern = temp.ptrnstr;
                #else
                temp.pattern = regex(temp.ptrnstr);
                #endif
                temp.cTime = stol(ml[3].str());
                temp.minutes = stoi(ml[4].str());
                temp.expiration = stol(ml[5].str());
                temp.client = ml[6].str();
                temp.reason = ml[7].str();
                alines.push_back(temp);
                //cout<<"[..] "<<temp.ptrnstr<<endl;
            }
        }
        file.close();
    }
}

void saveLine(autoLine line)
{
    ofstream file ("./halfMod/plugins/advbans/autolines.conf",ios_base::out|ios_base::app);
    if (file.is_open())
    {
        file<<line.type<<" "<<line.ptrnstr<<" "<<line.cTime<<" "<<line.minutes<<" "<<line.expiration<<" "<<line.client<<" "<<line.reason<<endl;
        file.close();
    }
}

int removeLine(short type, string ptrn)
{
    int ret = 0;
    for (auto it = alines.begin();it != alines.end();)
    {
        if ((it->type == type) && (it->ptrnstr == ptrn))
        {
            alines.erase(it);
            ret++;
        }
        else
            ++it;
    }
    if (ret)
    {
        remove("./halfMod/plugins/advbans/autolines.conf");
        for (auto it = alines.begin(), ite = alines.end();it != ite;++it)
            saveLine(*it);
    }
    return ret;
}

void blineUser(hmHandle &handle, autoLine *line, string client, string ip)
{
    hmSendRaw("ban " + client + " " + line->reason);
    if (ip.size() > 0)
        hmSendRaw("ban-ip " + ip + " " + line->reason);
    if ((int)line->expiration != 0)
    {
        time_t cTime = time(NULL);
        int banTime = line->expiration - cTime;
        ofstream oFile;
		oFile.open("./halfMod/plugins/admincmds/expirations.txt",ios_base::app);
		if (oFile.is_open())
		{
		    oFile<<"banExpire "<<line->expiration<<" "<<client<<endl;
		    if (ip.size() > 0)
    		    oFile<<"ipbanExpire "<<line->expiration<<" "<<ip<<endl;
			oFile.close();
		}
		else
			hmOutDebug(handle.getPlugin() + ": Cannot open file './halfMod/plugins/admincmds/expirations.txt'");
        handle.createTimer("unban",banTime,"banExpire","banExpire " + to_string(cTime) + " " + client);
        if (ip.size() > 0)
            handle.createTimer("unban",banTime,"ipbanExpire","ipbanExpire " + to_string(cTime) + " " + ip);
        hmLog("B-LINE (" + line->client + "): banned player " + client + " for " + to_string(banTime) + " seconds(s) (" + line->reason + ")",LOG_BAN,"bans.log");
        hmSendCommandFeedback(line->client,"Banned player " + client + " for " + to_string(banTime) + " second(s) (" + line->reason + ")");
    }
    else
    {
	    hmLog("B-LINE (" + line->client + "): permanently banned player " + client + " (" + line->reason + ")",LOG_BAN,"bans.log");
	    hmSendCommandFeedback(line->client,"Permanently banned player " + client + " (" + line->reason + ")");
    }
}

void klineUser(autoLine *line, string client)
{
    hmSendRaw("kick " + client + " " + line->reason);
    hmLog("K-LINE (" + line->client + "): kicked player " + client + " (" + line->reason + ")",LOG_BAN,"bans.log");
    hmSendCommandFeedback(line->client,"Kicked player " + client + " (" + line->reason + ")");
}















