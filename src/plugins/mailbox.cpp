#include <iostream>
#include <string>
#include <regex>
#include <ctime>
#include <cmath>
#include <sys/stat.h>
#include <fstream>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.1.4"

string amtTime(/*love you*/long times);

int getRealXP(int level);

void mbNotify(string client);

int xp2send;
string msgWith;

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    // THIS LINE IS REQUIRED IF YOU WANT TO PASS ANY INFO TO/FROM THE SERVER
    recallGlobal(global);
    // ^^^^ THIS ONE ^^^^
    
    handle.pluginInfo(  "Mailbox",
                        "nigel",
                        "Send and receive mail.",
                        VERSION,
                        "http://your.package.justca.me/" );
    handle.regConsoleCmd("hm_message","sendMessage","Leave a message for a player.");
    handle.regConsoleCmd("hm_sendchest","sendChest","Stand on top of a chest to seal and mail its contents to a player.");
    handle.regConsoleCmd("hm_senditem","sendItem","Send an item to a player.");
    handle.regConsoleCmd("hm_sendxp","sendXP","Send XP to a player.");
    handle.regConsoleCmd("hm_mailbox","checkMail","Check your mailbox.");
    handle.regConsoleCmd("hm_mb_open","mailOpen","Open an item from your mailbox.");
    handle.regConsoleCmd("hm_mb_accept","mailAccept","Accept an attatchment from a mailbox item.");
    handle.regConsoleCmd("hm_mb_return","mailReturn","Return mail to sender.");
    handle.regConsoleCmd("hm_mb_delete","mailDelete","Delete a mailbox item.");
    mkdirIf("./halfMod/plugins/mailbox/");
}

int onWorldInit(hmHandle &handle, smatch args)
{
    hmSendRaw("scoreboard objectives add hmMBXP dummy\nscoreboard players set #hm hmMBXP 0");
    return 0;
}

int onPlayerJoin(hmHandle &handle, smatch args)
{
    mbNotify(args[1].str());
    return 0;
}

int sendMessage(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 3)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <player> <message ...>");
        return 1;
    }
    hmPlayer mailbox = hmGetPlayerData(args[1]);
    if (mailbox.uuid != "")
    {
        string rec = stripFormat(lower(mailbox.name));
        ofstream file ("./halfMod/plugins/mailbox/" + rec + ".mail",ios_base::app);
        if (file.is_open())
        {
            time_t cTime = time(NULL);
            file<<"0:0:"<<client<<"="<<cTime<<"=0=";
            for (int i = 2;i < argc;i++)
                file<<args[i]<<" ";
            file<<endl;
            file.close();
            hmReplyToClient(client,"You have sent a message to " + mailbox.name + "!");
            mbNotify(mailbox.name);
        }
        else
            hmReplyToClient(client,"Error opening mailbox . . .");
    }
    else
        hmReplyToClient(client,"No record of player '" + args[1] + "' found.");
    return 0;
}

int sendItem(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <player> [message ...]");
        return 1;
    }
    client = stripFormat(client);
    hmPlayer mailbox = hmGetPlayerData(args[1]);
    if (mailbox.uuid != "")
    {
        if (argc > 2)
        {
            msgWith = args[2];
            for (int i = 3;i < argc;i++)
                msgWith = msgWith + " " + args[i];
        }
        else
            msgWith = "A shiny gift!";
        string target = stripFormat(lower(mailbox.name));
        // by using the same name for the pattern, we can unhook them both at the same time
        handle.hookPattern(client + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(" + client + "): Entity data updated to: (\\{.*Tags:\\[\"hmMBItem(" + target + ")\",\"killme\"\\].*\\})\\]$","sendItemCheck");
        handle.hookPattern(client + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Failed to execute 'entitydata @e\\[tag=hmMBItem" + target + ",c=1\\] \\{Owner:\"" + target + "\",PickupDelay:100s,Tags:\\[\"hmMBItem(" + target + ")\",\"killme\"\\]\\}' as (" + client + ")$","sendItemFail");
        hmSendRaw("execute " + client + " ~ ~ ~ scoreboard players tag @e[type=minecraft:item,r=10] add hmMBItem" + target + " {Thrower:\"" + client + "\"}\n\
                   execute " + client + " ~ ~ ~ entitydata @e[tag=hmMBItem" + target + ",c=1] {Owner:\"" + target + "\",PickupDelay:10s,Tags:[\"hmMBItem" + target + "\",\"killme\"]}\n\
                   scoreboard players tag @e[tag=hmMBItem" + target + "] remove hmMBItem" + target);
    }
    else
        hmReplyToClient(client,"No record of player '" + args[1] + "' found.");
    return 0;
}

int sendItemCheck(hmHandle &handle, hmHook hook, smatch args)
{
    string client = args[1].str(), tags = args[2].str(), target = args[3].str();
    regex ptrn ("UUID(Least|Most):[0-9]+?L,?");
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Tags:\\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Motion:\\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Rotation:\\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "FallDistance:[^f]*f,?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Dimension:[0-9]+?,?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Pos:\\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ofstream file ("./halfMod/plugins/mailbox/" + target + ".mail",ios_base::app);
    if (file.is_open())
    {
        hmSendRaw("kill @e[type=minecraft:item,tag=killme]");
        time_t cTime = time(NULL);
        file<<"0:2:"<<hmGetPlayerInfo(client).name<<"="<<cTime<<"="<<tags<<"="<<msgWith;
        file<<endl;
        file.close();
        hmReplyToClient(client,"You have sent an item to " + hmGetPlayerData(target).name + "!");
        mbNotify(target);
    }
    else
        hmReplyToClient(client,"Error opening mailbox . . .");
    handle.unhookPattern(hook.name);
    return 1;
}

int sendItemFail(hmHandle &handle, hmHook hook, smatch args)
{
    string target = args[1].str(), client = args[2].str();
    hmReplyToClient(client,"Unable to find an item to send! Drop an item from your inventory first!");
    handle.unhookPattern(hook.name);
    return 1;
}

int sendChest(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <player> [message ...]");
        return 1;
    }
    client = stripFormat(client);
    hmPlayer mailbox = hmGetPlayerData(args[1]);
    if (mailbox.uuid != "")
    {
        if (argc > 2)
        {
            msgWith = args[2];
            for (int i = 3;i < argc;i++)
                msgWith = msgWith + " " + args[i];
        }
        else
            msgWith = "A shiny gift!";
        string target = stripFormat(lower(mailbox.name));
        // by using the same name for the pattern, we can unhook them both at the same time
        // [15:36:38] [Server thread/INFO]: [nigathan: Block data updated to: {x:-249,y:63,z:-16,Items:[{Slot:0b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:1b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:2b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:3b,id:"minecraft:bowl",Count:1b,Damage:0s},{Slot:4b,id:"minecraft:chest",Count:1b,Damage:0s},{Slot:5b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:6b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:7b,id:"minecraft:iron_shovel",Count:1b,Damage:96s},{Slot:8b,id:"minecraft:wooden_pickaxe",Count:1b,Damage:6s},{Slot:9b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:10b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:11b,id:"minecraft:snowball",Count:16b,Damage:0s},{Slot:13b,id:"minecraft:mushroom_stew",Count:1b,Damage:0s},{Slot:14b,id:"minecraft:mushroom_stew",Count:1b,Damage:0s},{Slot:15b,id:"minecraft:mushroom_stew",Count:1b,Damage:0s},{Slot:16b,id:"minecraft:water_bucket",Count:1b,Damage:0s}],id:"minecraft:chest",Lock:"nigathan"}]
        handle.hookPattern("hmMBChest" + client,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: \\[(" + client + "): Block data updated to: \\{(.*Lock:\"(" + target + ")\".*)\\}\\]$","sendChestCheck");
        // [15:33:11] [Server thread/INFO]: Failed to execute 'detect' as nigathan
        handle.hookPattern("hmMBChest" + client,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Failed to execute 'detect' as (" + client + ")$","sendChestFail");
        // [02:16:29] [Server thread/INFO]: Selector '@a[name=nigathan,score_hmMBXP_min=5]' found nothing
        handle.hookPattern("hmMBChest" + client,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Selector '@a\\[name=(" + client + "),score_hmMBXP_min=5\\]' found nothing$","sendChestFailIron");
        //hmSendRaw("execute " + client + " ~ ~-0.5 ~ detect ~ ~ ~ minecraft:chest -1 blockdata ~ ~ ~ {Lock:\\\"" + target + "\\\"}");
        hmSendRaw("stats entity " + client + " set AffectedItems @s hmMBXP\n\
                   scoreboard players set " + client + " hmMBXP 0\n\
                   execute " + client + " ~ ~ ~ clear @s minecraft:iron_ingot -1 0\n\
                   stats entity " + client + " clear AffectedItems\n\
                   execute @a[name=" + client + ",score_hmMBXP_min=5] ~ ~-0.5 ~ detect ~ ~ ~ minecraft:chest -1 blockdata ~ ~ ~ {Lock:\"" + target + "\"}\n\
                   scoreboard players reset " + client + " hmMBXP");
    }
    else
        hmReplyToClient(client,"No record of player '" + args[1] + "' found.");
    return 0;
}

int sendChestCheck(hmHandle &handle, hmHook hook, smatch args)
{
    string client = args[1].str(), tags = args[2].str(), target = args[3].str();
    smatch ml;
    string pos = "";
    regex ptrn ("x:(-?[0-9]+)");
    if (regex_search(tags,ml,ptrn))
        pos = ml[1].str();
    ptrn = "y:(-?[0-9]+)";
    if (regex_search(tags,ml,ptrn))
        pos = pos + " " + ml[1].str();
    ptrn = "z:(-?[0-9]+)";
    if (regex_search(tags,ml,ptrn))
        pos = pos + " " + ml[1].str();
    while (!isin(gettok(tags,1,","),"Items:"))
        tags = deltok(tags,1,",");
    while (!isin(gettok(tags,-1,","),"]"))
        tags = deltok(tags,-1,",");
    tags = "{" + tags + "}";
    ofstream file ("./halfMod/plugins/mailbox/" + target + ".mail",ios_base::app);
    if (file.is_open())
    {
        hmSendRaw("setblock " + pos + " minecraft:air 0 replace\n\
                   clear " + client + " minecraft:iron_ingot -1 5");
        time_t cTime = time(NULL);
        file<<"0:3:"<<hmGetPlayerInfo(client).name<<"="<<cTime<<"="<<tags<<"="<<msgWith;
        file<<endl;
        file.close();
        hmReplyToClient(client,"You have sent a chest to " + hmGetPlayerData(target).name + "!");
        mbNotify(target);
    }
    else
    {
        hmReplyToClient(client,"Error opening mailbox . . .");
        hmSendRaw("blockdata " + pos + " {Lock:\"\"}");
    }
    handle.unhookPattern(hook.name);
    return 1;
}

int sendChestFail(hmHandle &handle, hmHook hook, smatch args)
{
    string client = args[1].str();
    hmReplyToClient(client,"Unable to find chest to send! Stand on top of a chest first!");
    handle.unhookPattern(hook.name);
    return 1;
}

int sendChestFailIron(hmHandle &handle, hmHook hook, smatch args)
{
    string client = args[1].str();
    hmReplyToClient(client,"It costs 5 iron ingots to wrap and send a chest! Try again when you can pay the fee!");
    handle.unhookPattern(hook.name);
    return 1;
}

int sendXP(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 3)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <player> <XP> [message ...] - XP is a valid amount of XP levels");
        return 1;
    }
    client = stripFormat(client);
    if ((stringisnum(args[2])) && ((xp2send = stoi(args[2])) > 0))
    {
        hmPlayer mailbox = hmGetPlayerData(args[1]);
        if (mailbox.uuid != "")
        {
            if (argc > 3)
            {
                msgWith = args[3];
                for (int i = 4;i < argc;i++)
                    msgWith = msgWith + " " + args[i];
            }
            else
                msgWith = "A shiny gift!";
            string fakeName = "#hmMBXP" + client + "#" + stripFormat(lower(mailbox.name));
        //  [23:53:26] [Server thread/INFO]: Set score of hmTest for player #fake to 10
            handle.hookPattern(fakeName,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Set score of hmMBXP for player #hmMBXP(" + client + ")#(" + stripFormat(lower(mailbox.name)) + ") to ([0-9]+)$","sendXPCheck");
            hmSendRaw("scoreboard players operation " + fakeName + " hmMBXP = #hm hmMBXP\n\
                       stats entity " + client + " set QueryResult " + fakeName + " hmMBXP\n\
                       execute " + client + " ~ ~ ~ xp 0L " + client + "\n\
                       stats entity " + client + " clear QueryResult\n\
                       scoreboard players add " + fakeName + " hmMBXP 0\n\
                       scoreboard players reset " + fakeName);
        }
        else
            hmReplyToClient(client,"No record of player '" + args[1] + "' found.");
    }
    else
        hmReplyToClient(client,"Usage: " + args[0] + " <player> <XP> [message ...] - XP is a valid amount of XP levels");
    return 0;
}

int sendXPCheck(hmHandle &handle, hmHook hook, smatch args)
{
    string client = args[1].str(), target = args[2].str();
    int levels = stoi(args[3].str()), amount;
    if (levels >= xp2send)
    {
        ofstream file ("./halfMod/plugins/mailbox/" + target + ".mail",ios_base::app);
        if (file.is_open())
        {
            amount = getRealXP(levels) - getRealXP(levels - xp2send);
            hmSendRaw(data2str("xp -%iL %s",xp2send,client.c_str()));
            time_t cTime = time(NULL);
            file<<"0:1:"<<hmGetPlayerInfo(client).name<<"="<<cTime<<"="<<amount<<"="<<msgWith;
            file<<endl;
            file.close();
            hmReplyToClient(client,data2str("You have sent %i XP to ",amount) + hmGetPlayerData(target).name + "!");
            mbNotify(target);
        }
        else
            hmReplyToClient(client,"Error opening mailbox . . .");
    }
    else
        hmReplyToClient(client,"You do not have enough XP to send! You only have " + data2str("%i levels!",levels));
    xp2send = 0;
    msgWith.clear();
    handle.unhookPattern(hook.name);
    return 1;
}

int checkMail(hmHandle &handle, string client, string args[], int argc)
{
    int page = 1;
    if ((argc > 1) && (stringisnum(args[1],2)))
        page = stoi(args[1]);
    client = stripFormat(lower(client));
    ifstream file ("./halfMod/plugins/mailbox/" + client + ".mail");
    if (file.is_open())
    {
        string line = "";
        vector<string> box;
        while (getline(file,line))
            if (line != "")
                box.push_back(line);
        file.close();
        if (box.size() < 1)
        {
            hmReplyToClient(client,"You have no mail! :(");
            return 1;
        }
        int i = 1;
        int pages = (box.size()/5);
        if ((box.size()%5) > 0) pages++;
        if (page > pages)
            page = pages;
        hmReplyToClient(client,"Displaying page " + data2str("%i/%i",page,pages) + " of your mailbox:");
        string json, sender;
        time_t timestamp, cTime = time(NULL);
        tm *tstamp;
        char tsBuf[17];
        for (auto it = box.begin(), ite = box.end();it != ite;++it)
        {
            if ((i > ((page-1)*5)) && (i <= (page*5)))
            {
                sender = gettok(gettok(*it,1,"="),3,":");
                timestamp = stoi(gettok(*it,2,"="));
                tstamp = localtime(&timestamp);
                strftime(tsBuf,17,"[%D %R]",tstamp);
                json = "[\"[HM] \",";
                if (it->at(0) == '0')
                    json += data2str("{\"text\":\"[%i] * \",\"color\":\"gold\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mb_open %i\"},\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Open\",\"color\":\"blue\"}]}}},",i,i);
                else
                    json += data2str("{\"text\":\"[%i] \",\"color\":\"gray\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mb_open %i\"},\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Re-open\",\"color\":\"blue\"}]}}},",i,i);
                if (it->at(2) > 51)
                {
                    if (it->at(0) != '2')
                        json += "{\"text\":\"[RA] \",\"color\":\"black\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Returned Attatchment!\",\"color\":\"red\"}]}}},";
                    else
                        json += "{\"text\":\"[RA] \",\"color\":\"dark_blue\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Attatchment Received!\",\"color\":\"dark_blue\"}]}}},";
                }
                else if (it->at(2) != '0')
                {
                    if (it->at(0) != '2')
                        json += "{\"text\":\"[A] \",\"color\":\"red\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Attatchment!\",\"color\":\"red\"}]}}},";
                    else
                        json += "{\"text\":\"[A] \",\"color\":\"dark_blue\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Attatchment Received!\",\"color\":\"dark_blue\"}]}}},";
                }
                hmSendRaw("tellraw " + client + " " + json + data2str("{\"text\":\"%s\",\"color\":\"dark_green\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"%s ago\",\"color\":\"dark_green\"}]}}},{\"text\":\" From \",\"color\":\"gray\"},{\"text\":\"%s\",\"color\":\"white\"}]",tsBuf,amtTime(cTime-timestamp).c_str(),sender.c_str()));
            }
            if (++i > (page*5))
                break;
        }
        if ((page > 1) && (page < pages))
            hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"<-- [Prev]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mailbox " + data2str("%i",(page-1)) + "\"}},{\"text\":\"  +  \",\"color\":\"none\"},{\"text\":\"[Next] -->\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mailbox " + data2str("%i",(page+1)) + "\"}}]");
        else if (page > 1)
            hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"<-- [Previous Page]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mailbox " + data2str("%i",(page-1)) + "\"}}]");
        else if (page < pages)
            hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"[Next Page] -->\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mailbox " + data2str("%i",(page+1)) + "\"}}]");
    }
    else
        hmReplyToClient(client,"You have no mail! :(");
    return 0;
}

int mailOpen(hmHandle &handle, string client, string args[], int argc)
{
    if ((argc < 2) || (!stringisnum(args[1],1)))
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <N> - N is a valid item number in your mailbox");
        return 1;
    }
    int item = stoi(args[1]);
    client = stripFormat(lower(client));
    fstream file ("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::in);
    if (file.is_open())
    {
        string line = "";
        vector<string> box;
        while (getline(file,line))
            if (line != "")
                box.push_back(line);
        file.close();
        if (item > box.size())
        {
            if (box.size() < 1)
                hmReplyToClient(client,"You have no mail! :(");
            else if (box.size() == 1)
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " message in your mailbox!");
            else
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " messages in your mailbox!");
            return 1;
        }
        int attType, xp, opened, count, i = 1;
        string json, sender, att, msg, id;
        time_t timestamp, cTime = time(NULL);
        tm *tstamp;
        char tsBuf[17];
        regex ptrn;// ("\\{(?>[^{}]|(?R))*\\}");
        smatch ml;
        for (auto it = box.begin(), ite = box.end();it != ite;++it)
        {
            if (i == item)
            {
                sender = gettok(gettok(*it,1,"="),3,":");
                timestamp = stoi(gettok(*it,2,"="));
                tstamp = localtime(&timestamp);
                strftime(tsBuf,17,"[%D %R]",tstamp);
                opened = it->at(0)-48;
                attType = it->at(2)-48;
                if ((attType > 1) && (attType != 4))
                {
                    att = "{" + deltok(deltok(*it,1,"{"),-1,"}") + "}";
                    //regex_search(*it,ml,ptrn);
                    //att = ml[0];
                    msg = strremove(*it,att + "=");
                    if ((attType == 2) || (attType == 5))
                    {
                        ptrn = "id:\"(.+?)\"";
                        regex_search(att,ml,ptrn);
                        id = ml[1];
                        ptrn = "Count:([0-9]+)b";
                        regex_search(att,ml,ptrn);
                        count = stoi(ml[1].str());
                    }
                }
                else
                {
                    xp = stoi(gettok(*it,3,"="));
                    msg = deltok(*it,3,"=");
                }
                msg = deltok(deltok(msg,1,"="),1,"=");
                hmSendRaw(data2str("tellraw %s [\"[HM] \",{\"text\":\"%s\",\"color\":\"dark_green\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"%s ago\",\"color\":\"dark_green\"}]}}},{\"text\":\" <%s> %s\",\"color\":\"white\"}]",client.c_str(),tsBuf,amtTime(cTime-timestamp).c_str(),sender.c_str(),msg.c_str()));
                if (attType > 0)
                {
                    json = " [\"[HM] \",";
                    if (opened != 2)
                        json += "{\"text\":\"[A] \",\"color\":\"red\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Click to claim this attatchment!\",\"color\":\"red\"}]}},\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mb_accept " + data2str("%i",item) + "\"}},";
                    else
                        json += "{\"text\":\"[A] \",\"color\":\"dark_blue\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Attatchment already received!\",\"color\":\"dark_blue\"}]}}},";
                    if ((attType == 1) || (attType == 4))
                        json += data2str("{\"text\":\"%i\",\"color\":\"light_purple\"},{\"text\":\" Experience Points!\",\"color\":\"blue\"}]",xp);
                    else if ((attType == 2) || (attType == 5))
                        json += data2str("{\"text\":\"%i\",\"color\":\"light_purple\"},{\"text\":\"x \",\"color\":\"white\"},{\"text\":\"Item(s)!\",\"color\":\"blue\",\"hoverEvent\":{\"action\":\"show_item\",\"value\":\"{\\\\\\\\\"id\\\\\\\\\":\\\\\\\\\"%s\\\\\\\\\",\\\\\\\\\"Count\\\\\\\\\":%i}\"}}]",count,id.c_str(),count);
                    else if ((attType == 3) || (attType == 6))
                        json += "{\"text\":\"A nicely wrapped package!\",\"color\":\"blue\"}]";
                    hmSendRaw("tellraw " + client + json);
                    hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"[Return to sender]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mb_return " + data2str("%i",item) + "\"}},{\"text\":\"  +  \",\"color\":\"white\"},{\"text\":\"[Delete]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mb_delete " + data2str("%i",item) + "\"}}]");
                }
                else
                    hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"[Delete]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mb_delete " + data2str("%i",item) + "\"}}]");
                break;
            }
            i++;
        }
        file.open("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::out|ios_base::trunc);
        if (file.is_open())
        {
            i = 0;
            item--;
            for (auto it = box.begin(), ite = box.end();it != ite;++it)
            {
                if ((i == item) && (it->at(0) == 48))
                {
                    it->front() = '1';
                    file<<*it<<endl;
                }
                else
                    file<<*it<<endl;
                i++;
            }
            file.close();
        }
    }
    else
        hmReplyToClient(client,"You have no mail! :(");
    return 0;
}

int mailAccept(hmHandle &handle, string client, string args[], int argc)
{
    if ((argc < 2) || (!stringisnum(args[1],1)))
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <N> - N is a valid item number in your mailbox");
        return 1;
    }
    int item = stoi(args[1]);
    client = stripFormat(lower(client));
    fstream file ("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::in);
    if (file.is_open())
    {
        string line = "";
        vector<string> box;
        while (getline(file,line))
            if (line != "")
                box.push_back(line);
        file.close();
        if (item > box.size())
        {
            if (box.size() < 1)
                hmReplyToClient(client,"You have no mail! :(");
            else if (box.size() == 1)
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " message in your mailbox!");
            else
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " messages in your mailbox!");
            return 1;
        }
        string mail = box[item-1];
        if (mail.at(0) > 49)
        {
            hmReplyToClient(client,"You have already claimed this attatchment!");
            return 1;
        }
        int attType = mail.at(2)-48;
        if (attType < 1)
        {
            hmReplyToClient(client,"This message has no attatchment!");
            return 1;
        }
        string att, sender = gettok(gettok(mail,1,"="),3,":");
        if ((attType > 1) && (attType != 4))
        {
            //att = "{" + strreplace(deltok(deltok(mail,1,"{"),-1,"}"),"\"","\\\"") + "}";
            att = "{" + deltok(deltok(mail,1,"{"),-1,"}") + "}";
            if ((attType == 2) || (attType == 5))
            {
                hmSendRaw("execute " + client + " ~ ~1.2 ~ summon minecraft:item ~ ~ ~ " + att);
                hmReplyToClient(client,"You have claimed your item! Be sure to thank " + sender + "!");
            }
            else
            {
                hmSendRaw("execute " + client + " ~ ~5 ~ summon minecraft:chest_minecart ~ ~ ~ " + att);
                hmReplyToClient(client,"You're package has arrived! Be sure to thank " + sender + "!");
            }
        }
        else
        {
            att = gettok(mail,3,"=");
            hmSendRaw("xp " + att + " " + client);
            hmReplyToClient(client,"You have claimed your " + att + " XP! Be sure to thank " + sender + "!");
        }
        file.open("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::out|ios_base::trunc);
        if (file.is_open())
        {
            int i = 0;
            item--;
            for (auto it = box.begin(), ite = box.end();it != ite;++it)
            {
                if ((i == item) && (it->at(0) != 50))
                {
                    it->front() = '2';
                    file<<*it<<endl;
                }
                else
                    file<<*it<<endl;
                i++;
            }
            file.close();
        }
    }
    else
        hmReplyToClient(client,"You have no mail! :(");
    return 0;
}

int mailDelete(hmHandle &handle, string client, string args[], int argc)
{
    if ((argc < 2) || (!stringisnum(args[1],1)))
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <N> - N is a valid item number in your mailbox");
        return 1;
    }
    int item = stoi(args[1]);
    client = stripFormat(lower(client));
    fstream file ("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::in);
    if (file.is_open())
    {
        string line = "";
        vector<string> box;
        while (getline(file,line))
            if (line != "")
                box.push_back(line);
        file.close();
        if (item > box.size())
        {
            if (box.size() < 1)
                hmReplyToClient(client,"You have no mail! :(");
            else if (box.size() == 1)
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " message in your mailbox!");
            else
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " messages in your mailbox!");
            return 1;
        }
        int i = 0;
        item--;
        file.open("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::out|ios_base::trunc);
        if (file.is_open())
        {
            for (auto it = box.begin(), ite = box.end();it != ite;++it)
            {
                if (i != item)
                    file<<*it<<endl;
                i++;
            }
            file.close();
            hmReplyToClient(client,"That piece of mail is gone forever!");
        }
        else
            hmReplyToClient(client,"Error opening mailbox . . .");
    }
    else
        hmReplyToClient(client,"You have no mail! :(");
    return 0;
}

int mailReturn(hmHandle &handle, string client, string args[], int argc)
{
    if ((argc < 2) || (!stringisnum(args[1],1)))
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <N> [message ...] - N is a valid item number in your mailbox");
        return 1;
    }
    string msg = "No thanks! You keep it :)";
    if (argc > 2)
    {
        msg = args[2];
        for (int i = 3;i < argc;i++)
            msg = msg + " " + args[i];
    }
    int item = stoi(args[1]);
    string stripClient = stripFormat(lower(client));
    fstream file ("./halfMod/plugins/mailbox/" + stripClient + ".mail",ios_base::in);
    if (file.is_open())
    {
        string line = "";
        vector<string> box;
        while (getline(file,line))
            if (line != "")
                box.push_back(line);
        file.close();
        if (item > box.size())
        {
            if (box.size() < 1)
                hmReplyToClient(client,"You have no mail! :(");
            else if (box.size() == 1)
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " message in your mailbox!");
            else
                hmReplyToClient(client,"You only have " + data2str("%li",box.size()) + " messages in your mailbox!");
            return 1;
        }
        item--;
        string mail = box[item];
        if (mail.at(0) == '2')
        {
            hmReplyToClient(client,"Surely they wouldn't want that empty box...");
            return 1;
        }
        if ((mail.at(2)-48) > 3)
        {
            hmReplyToClient(client,"You're probably better off just deleting this one...");
            return 1;
        }
        if (mail.at(2) == '0')
        {
            hmReplyToClient(client,"No attatchments to return!");
            return 1;
        }
        string sender = gettok(gettok(mail,3,":"),1,"=");
        time_t cTime = time(NULL);
        string data = "0";
        int type = mail.at(2)-45; // convert to integer and add 3
        if (type == 4)
            data = gettok(mail,3,"=");
        else
            data = "{" + deltok(deltok(mail,1,"{"),-1,"}") + "}";
        if (stripClient == stripFormat(lower(sender)))
            box.push_back(data2str("0:%i:%s=%li=%s=%s",type,client.c_str(),cTime,data.c_str(),msg.c_str()));
        else
        {
            file.open("./halfMod/plugins/mailbox/" + stripFormat(lower(sender)) + ".mail",ios_base::out|ios_base::app);
            if (file.is_open())
            {
                file<<"0:"<<type<<":"<<client<<"="<<cTime<<"="<<data<<"="<<msg<<endl;
                file.close();
            }
            else
            {
                hmReplyToClient(client,"Error opening mailbox . . .");
                return 1;
            }
        }
        int i = 0;
        file.open("./halfMod/plugins/mailbox/" + stripClient + ".mail",ios_base::out|ios_base::trunc);
        if (file.is_open())
        {
            for (auto it = box.begin(), ite = box.end();it != ite;++it)
            {
                if (i != item)
                    file<<*it<<endl;
                i++;
            }
            file.close();
        }
        else
            hmReplyToClient(client,"Error opening mailbox . . .");
    }
    else
        hmReplyToClient(client,"You have no mail! :(");
    hmReplyToClient(client,"The mail has been returned to sender!");
    return 0;
}

}

void mbNotify(string client)
{
    if (hmIsPlayerOnline(client))
    {
        int n = 0;
        client = stripFormat(lower(client));
        fstream file ("./halfMod/plugins/mailbox/" + client + ".mail",ios_base::in);
        if (file.is_open())
        {
            string line = "";
            vector<string> box;
            while (getline(file,line))
                if (line != "")
                    box.push_back(line);
            file.close();
            for (auto it = box.begin(), ite = box.end();it != ite;++it)
                if (it->at(0) == '0')
                    n++;
        }
        if (n > 0)
            hmSendRaw(data2str("tellraw %s [\"[HM] You have \",{\"text\":\"%i\",\"color\":\"gold\",\"bold\":true},{\"text\":\" unread message(s)! \",\"color\":\"none\",\"bold\":false},{\"text\":\"Click here\",\"color\":\"blue\",\"underlined\":true,\"clickEvent\":{\"action\":\"run_command\",\"value\":\"!mailbox\"}},{\"text\":\" to open your mailbox!\",\"color\":\"none\",\"underlined\":false}]",client.c_str(),n));            
    }
}

int getRealXP(int level)
{
    int amount = 0;
    if (level <= 16)
        amount = pow(level,2) + (6*level);
    else if (level <= 31)
        amount = int(float(2.5*pow(level,2)) - float(40.5*level) + 360);
    else
        amount = int(float(4.5*pow(level,2)) - float(162.5*level) + 2220);
    return amount;
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






