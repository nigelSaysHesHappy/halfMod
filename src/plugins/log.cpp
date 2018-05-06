#include <iostream>
#include <fstream>
#include <ctime>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.1.7"

#define LOG_FILTER_TEXT     0
#define LOG_FILTER_REGEX    1
#define LOG_FILTER_WILD     2

struct logInfo
{
    string client;
    string handle;
    string message;
    string pos;
};

vector<logInfo> pendingLog;

int isinnc(string text, string subtext, int pos = 0)
{
    return isin(lower(text),lower(subtext),pos);
}
bool iswmnc(string text, string subtext)
{
    return iswm(lower(text),lower(subtext));
}
extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Logs",
                      "nigel",
                      "User Defined Personal and Global Logs",
                      VERSION,
                      "http://captains.log.justca.me/");
    handle.regConsoleCmd("hm_captainslog","selfLog","Log something that only you can see");
    handle.regConsoleCmd("hm_publiclog","publicLog","Log something publicly");
    //handle.regAdminCmd("hm_purgelog","purgeLog",FLAG_ADMIN,"Purge the public or personal logs");
    mkdirIf("./halfMod/plugins/playerLogs/");
    mkdirIf("./halfMod/plugins/playerLogs/players/");
    return 0;
}

int publicLog(hmHandle &handle, string client, string args[], int argc)
{
    int length = 5;
    string filter = "";
    int filterType = LOG_FILTER_TEXT;
    if (argc > 1)
    {
        if (args[1] == "help")
        {
            hmReplyToClient(client,"Usage: " + args[0] + " <text_to_log_not_beginning_with_number>");
            hmReplyToClient(client,"Usage: " + args[0] + " [#_lines_to_display]");
            hmReplyToClient(client,"Usage: " + args[0] + " <#_lines_to_display> [filter <text|wild|regex> <pattern ...>] [pattern ...]");
            return 1;
        }
        if (stringisnum(args[1]))
        {
            length = stoi(args[1]);
            if (argc > 2)
            {
                if (args[2] == "filter")
                {
                    if (argc < 5)
                    {
                        hmReplyToClient(client,"Usage: " + args[0] + " <#_lines_to_display> [filter <text|wild|regex> <pattern ...>] [pattern ...]");
                        return 1;
                    }
                    if (args[3] == "text")
                        filterType = LOG_FILTER_TEXT;
                    else if (args[3] == "wild")
                        filterType = LOG_FILTER_WILD;
                    else if (args[3] == "regex")
                        filterType = LOG_FILTER_REGEX;
                    else
                    {
                        hmReplyToClient(client,"Invalid filter type: " + args[3] + ". Valid options: text, wild, regex.");
                        return 1;
                    }
                    for (int i = 4;i < argc;i++)
                        filter = appendtok(filter,args[i]," ");
                }
                else for (int i = 2;i < argc;i++)
                    filter = appendtok(filter,args[i]," ");
            }
        }
        else
        {
            logInfo temp;
            temp.client = client;
            temp.handle = client + ":" + to_string(rand());
            for (int i = 1;i < argc;i++)
                temp.message = temp.message + args[i] + " ";
            if (client != "#SERVER")
            {
                pendingLog.push_back(temp);
                //[12:12:35] [Server thread/INFO]: nigathan has the following entity data: [-299.73073281022636d, 102.94437987049848d, -29.250267740956563d]
                handle.hookPattern(temp.handle,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + stripFormat(client) + " has the following entity data: \\[([^d.]+)\\.?[0-9]*d, ([^d.]+)\\.?[0-9]*d, ([^d.]+)\\.?[0-9]*d\\]$","publicPos");
                handle.hookPattern(temp.handle,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + stripFormat(client) + " has the following entity data: (-?[0-9]+)$","publicDim");
                hmSendRaw("data get entity " + client + " Pos\ndata get entity " + client + " Dimension");
            }
            else
            {
                ofstream file ("./halfMod/plugins/playerLogs/public.log",ios_base::app);
                if (file.is_open())
                {
                    time_t cTime = time(NULL);
                    tm *tstamp;
                    tstamp = localtime(&cTime);
                    char tsBuf[18];
                    strftime(tsBuf,18,"[%D %R] ",tstamp);
                    file<<tsBuf<<"<"<<client<<"> "<<temp.message<<endl;
                    file.close();
                    hmReplyToClient(client,"Public log updated!");
                }
                else
                    hmReplyToClient(client,"Error opening the public log :(");
            }
            return 0;
        }
    }
    ifstream file ("./halfMod/plugins/playerLogs/public.log");
    if (file.is_open())
    {
        vector<string> log;
        string line;
        if (filter.size() < 1)
        {
            while (getline(file,line))
            {
                log.push_back(line);
                if (log.size() > length)
                    log.erase(log.begin());
            }
        }
        else
        {
            while (getline(file,line))
                log.push_back(line);
            if (filterType == LOG_FILTER_TEXT)
            {
                for (auto it = log.begin();it != log.end();)
                {
                    if (!isinnc(*it,filter))
                        log.erase(it);
                    else ++it;
                }
            }
            else if (filterType == LOG_FILTER_WILD)
            {
                for (auto it = log.begin();it != log.end();)
                {
                    if (!iswmnc(*it,filter))
                        log.erase(it);
                    else ++it;
                }
            }
            else if (filterType == LOG_FILTER_REGEX)
            {
                regex ptrn (filter);
                for (auto it = log.begin();it != log.end();)
                {
                    if (!regex_search(*it,ptrn))
                        log.erase(it);
                    else ++it;
                }
            }
            while (log.size() > length)
                log.erase(log.begin());
        }
        file.close();
        if (log.size() < 1)
            hmReplyToClient(client,"Nothing here! :(");
        else
        {
            int i = 0;
            for (auto it = log.begin(), ite = log.end();it != ite;++it)
            {
                hmReplyToClient(client,"[" + to_string(i) + "] " + *it);
                i++;
            }
        }
    }
    else
        hmReplyToClient(client,"Error opening the public log :(");
    return 0;
}

int publicPos(hmHandle &handle, hmHook hook, smatch args)
{
    for (auto it = pendingLog.begin(), ite = pendingLog.end();it != ite;++it)
    {
        if (it->handle == hook.name)
        {
            it->pos = "(" + args[1].str() + " " + args[2].str() + " " + args[3].str() + " in ";
            break;
        }
    }
    return 1;
}

int publicDim(hmHandle &handle, hmHook hook, smatch args)
{
    for (auto it = pendingLog.begin(), ite = pendingLog.end();it != ite;++it)
    {
        if (it->handle == hook.name)
        {
            int dim = stoi(args[1].str());
            switch (dim)
            {
                case -1:
                {
                    it->pos += "The Nether)";
                    break;
                }
                case 0:
                {
                    it->pos += "The Overworld)";
                    break;
                }
                case 1:
                {
                    it->pos += "The End)";
                    break;
                }
                default:
                    it->pos += "Unknown: " + args[1].str() + ")";
            }
            ofstream file ("./halfMod/plugins/playerLogs/public.log",ios_base::app);
            if (file.is_open())
            {
                time_t cTime = time(NULL);
                tm *tstamp;
                tstamp = localtime(&cTime);
                char tsBuf[18];
                strftime(tsBuf,18,"[%D %R] ",tstamp);
                file<<tsBuf<<it->pos<<" <"<<it->client<<"> "<<it->message<<endl;
                file.close();
                hmReplyToClient(it->client,"Public log updated!");
            }
            else
                hmReplyToClient(it->client,"Error opening the public log :(");
            pendingLog.erase(it);
            break;
        }
    }
    handle.unhookPattern(hook.name);
    return 1;
}

int selfLog(hmHandle &handle, string client, string args[], int argc)
{
    if (client == "#SERVER")
    {
        hmReplyToClient(client,"The captain's log is only available for players!");
        return 1;
    }
    int length = 5;
    string filter = "";
    int filterType = LOG_FILTER_TEXT;
    if (argc > 1)
    {
        if (args[1] == "help")
        {
            hmReplyToClient(client,"Usage: " + args[0] + " <text_to_log_not_beginning_with_number>");
            hmReplyToClient(client,"Usage: " + args[0] + " [#_lines_to_display]");
            hmReplyToClient(client,"Usage: " + args[0] + " <#_lines_to_display> [filter <text|wild|regex> <pattern ...>] [pattern ...]");
            return 1;
        }
        if (stringisnum(args[1]))
        {
            length = stoi(args[1]);
            if (argc > 2)
            {
                if (args[2] == "filter")
                {
                    if (argc < 5)
                    {
                        hmReplyToClient(client,"Usage: " + args[0] + " <#_lines_to_display> [filter <text|wild|regex> <pattern ...>] [pattern ...]");
                        return 1;
                    }
                    if (args[3] == "text")
                        filterType = LOG_FILTER_TEXT;
                    else if (args[3] == "wild")
                        filterType = LOG_FILTER_WILD;
                    else if (args[3] == "regex")
                        filterType = LOG_FILTER_REGEX;
                    else
                    {
                        hmReplyToClient(client,"Invalid filter type: " + args[3] + ". Valid options: text, wild, regex.");
                        return 1;
                    }
                    for (int i = 4;i < argc;i++)
                        filter = appendtok(filter,args[i]," ");
                }
                else for (int i = 2;i < argc;i++)
                    filter = appendtok(filter,args[i]," ");
            }
        }
        else
        {
            logInfo temp;
            temp.client = client;
            temp.handle = client + ":" + to_string(rand());
            for (int i = 1;i < argc;i++)
                temp.message = temp.message + args[i] + " ";
            pendingLog.push_back(temp);
            //[12:12:35] [Server thread/INFO]: nigathan has the following entity data: [-299.73073281022636d, 102.94437987049848d, -29.250267740956563d]
            handle.hookPattern(temp.handle,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + stripFormat(client) + " has the following entity data: \\[([^d.]+)\\.?[0-9]*d, ([^d.]+)\\.?[0-9]*d, ([^d.]+)\\.?[0-9]*d\\]$","selfPos");
            handle.hookPattern(temp.handle,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: " + stripFormat(client) + " has the following entity data: (-?[0-9]+)$","selfDim");
            hmSendRaw("data get entity " + client + " Pos\ndata get entity " + client + " Dimension");
            return 0;
        }
    }
    ifstream file ("./halfMod/plugins/playerLogs/players/" + stripFormat(lower(client)) + ".log");
    if (file.is_open())
    {
        vector<string> log;
        string line;
        if (filter.size() < 1)
        {
            while (getline(file,line))
            {
                log.push_back(line);
                if (log.size() > length)
                    log.erase(log.begin());
            }
        }
        else
        {
            while (getline(file,line))
                log.push_back(line);
            if (filterType == LOG_FILTER_TEXT)
            {
                for (auto it = log.begin();it != log.end();)
                {
                    if (!isinnc(*it,filter))
                        log.erase(it);
                    else ++it;
                }
            }
            else if (filterType == LOG_FILTER_WILD)
            {
                for (auto it = log.begin();it != log.end();)
                {
                    if (!iswmnc(*it,filter))
                        log.erase(it);
                    else ++it;
                }
            }
            else if (filterType == LOG_FILTER_REGEX)
            {
                regex ptrn (filter);
                for (auto it = log.begin();it != log.end();)
                {
                    if (!regex_match(*it,ptrn))
                        log.erase(it);
                    else ++it;
                }
            }
            while (log.size() > length)
                log.erase(log.begin());
        }
        file.close();
        if (log.size() < 1)
            hmReplyToClient(client,"Nothing here! :(");
        else
        {
            int i = 0;
            for (auto it = log.begin(), ite = log.end();it != ite;++it)
            {
                hmReplyToClient(client,"[" + to_string(i) + "] " + *it);
                i++;
            }
        }
    }
    else
        hmReplyToClient(client,"Error opening your captain's log :(");
    return 0;
}

int selfPos(hmHandle &handle, hmHook hook, smatch args)
{
    for (auto it = pendingLog.begin(), ite = pendingLog.end();it != ite;++it)
    {
        if (it->handle == hook.name)
        {
            it->pos = "(" + args[1].str() + " " + args[2].str() + " " + args[3].str() + " in ";
            break;
        }
    }
    return 1;
}

int selfDim(hmHandle &handle, hmHook hook, smatch args)
{
    for (auto it = pendingLog.begin(), ite = pendingLog.end();it != ite;++it)
    {
        if (it->handle == hook.name)
        {
            int dim = stoi(args[1].str());
            switch (dim)
            {
                case -1:
                {
                    it->pos += "The Nether)";
                    break;
                }
                case 0:
                {
                    it->pos += "The Overworld)";
                    break;
                }
                case 1:
                {
                    it->pos += "The End)";
                    break;
                }
                default:
                    it->pos += "Unknown: " + args[1].str() + ")";
            }
            ofstream file ("./halfMod/plugins/playerLogs/players/" + stripFormat(lower(it->client)) + ".log",ios_base::app);
            if (file.is_open())
            {
                time_t cTime = time(NULL);
                tm *tstamp;
                tstamp = localtime(&cTime);
                char tsBuf[18];
                strftime(tsBuf,18,"[%D %R] ",tstamp);
                file<<tsBuf<<it->pos<<" "<<it->message<<endl;
                file.close();
                hmReplyToClient(it->client,"Captain's log updated!");
            }
            else
                hmReplyToClient(it->client,"Error opening your captain's log :(");
            pendingLog.erase(it);
            break;
        }
    }
    handle.unhookPattern(hook.name);
    return 1;
}

}

