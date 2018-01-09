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

#define VERSION "v0.2.0"

string amtTime(/*love you*/long times);

int getRealXP(int level);

void mbNotify(string client);

int xp2send;
string msgWith;

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
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
    return 0;
}

/*int onWorldInit(hmHandle &handle, smatch args)
{
    hmSendRaw("scoreboard objectives add hmMBXP dummy\nscoreboard players set #hm hmMBXP 0");
    return 0;
}*/

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
    hmPlayer mailbox;
    if (hmIsPlayerOnline(args[1]))
        mailbox = hmGetPlayerInfo(args[1]);
    else
        mailbox = hmGetPlayerData(args[1]);
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
    hmPlayer mailbox;
    if (hmIsPlayerOnline(args[1]))
        mailbox = hmGetPlayerInfo(args[1]);
    else
        mailbox = hmGetPlayerData(args[1]);
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
        handle.hookPattern(client + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: .+ has the following entity data: (\\{.*Tags: \\[\"hmMBItem(" + target + ")\".*\\})$","sendItemCheck");
        handle.hookPattern(client + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/ERROR\\]: Couldn't execute command for Server:\\s+data get entity @e\\[tag=hmMBItem" + target + ",limit=1\\]$","sendItemFail");
        hmSendRaw("execute at " + client + " run data merge entity @e[type=minecraft:item,distance=..10,limit=1,sort=nearest,nbt={Thrower:\"" + client + "\"}] {Owner:\"" + target + "\",PickupDelay:10s,Tags:[\"hmMBItem" + target + "\",\"killme\"]}\n" +
                  "data get entity @e[tag=hmMBItem" + target + ",limit=1]");
    }
    else
        hmReplyToClient(client,"No record of player '" + args[1] + "' found.");
    return 0;
}

int sendItemCheck(hmHandle &handle, hmHook hook, smatch args)
{
    string client = gettok(hook.name,1," "), tags = args[1].str(), target = args[2].str();
    regex ptrn ("UUID(Least|Most): -?[0-9]+?L,?");
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Tags: \\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Motion: \\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Rotation: \\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "FallDistance: [^f]*f,?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Dimension: -?[0-9]+?,?";
    tags = regex_replace(tags,ptrn,"");
    ptrn = "Pos: \\[[^\\]]*\\],?";
    tags = regex_replace(tags,ptrn,"");
    ofstream file ("./halfMod/plugins/mailbox/" + target + ".mail",ios_base::app);
    if (file.is_open())
    {
        hmSendRaw("kill @e[type=minecraft:item,tag=killme]");
        time_t cTime = time(NULL);
        file<<"0:2:"<<hmGetPlayerInfo(client).name<<"="<<cTime<<"="<<tags<<"="<<msgWith;
        file<<endl;
        file.close();
        if (hmIsPlayerOnline(target))
            hmReplyToClient(client,"You have sent an item to " + hmGetPlayerInfo(target).name + "!");
        else
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
    string client = gettok(hook.name,1," ");
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
    hmPlayer mailbox;
    if (hmIsPlayerOnline(args[1]))
        mailbox = hmGetPlayerInfo(args[1]);
    else
        mailbox = hmGetPlayerData(args[1]);
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
        // [04:16:59] [Server thread/INFO]: No items were found on player nigathan
        handle.hookPattern("hmMBChest " + client + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: No items were found on player (" + client + ")$","sendChestFailIron");
        // [04:17:50] [Server thread/INFO]: Found 64 matching items on player nigathan
        handle.hookPattern("hmMBChest " + client + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Found ([0-9]+) matching items on player (" + client + ")$","sendChestIron");
        hmSendRaw("clear " + client + " minecraft:iron_ingot 0");
//                 execute at " + client + " run data get block ~ ~-0.5 ~");
    }
    else
        hmReplyToClient(client,"No record of player '" + args[1] + "' found.");
    return 0;
}

int sendChestIron(hmHandle &handle, hmHook hook, smatch args)
{
    int count = stoi(args[1].str());
    string client = args[2].str();
    if (count < 5)
    {
        hmReplyToClient(client,"It costs 5 iron ingots to wrap and send a chest! Try again when you can pay the fee!");
        handle.unhookPattern(hook.name);
        return 1;
    }
    string target = gettok(hook.name,3," ");
    // [04:13:56] [Server thread/INFO]: -236, 67, -40 has the following block data: {x: -236, y: 67, z: -40, Items: [], id: "minecraft:chest", Lock: ""}
    handle.hookPattern("hmMBChest " + client + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (-?[0-9]+), (-?[0-9]+), (-?[0-9]+) has the following block data: \\{(.*id: \"(.+?)\".*)\\}$","sendChestCheck");
    // [04:13:16] [Server thread/INFO]: The target block is not a block entity
    handle.hookPattern("hmMBChest " + client + " " + target,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: The target block is not a block entity$","sendChestFail");
    hmSendRaw("execute at " + client + " run data get block ~ ~-0.5 ~");
    return 1;
}

int sendChestCheck(hmHandle &handle, hmHook hook, smatch args)
{
    string pos = args[1].str() + " " + args[2].str() + " " + args[3].str();
    string client = gettok(hook.name,2," "), tags = args[4].str(), block = args[5].str();
    // "minecraft:white_shulker_box minecraft:orange_shulker_box minecraft:magenta_shulker_box minecraft:light_blue_shulker_box minecraft:yellow_shulker_box minecraft:lime_shulker_box minecraft:pink_shulker_box minecraft:gray_shulker_box minecraft:light_gray_shulker_box minecraft:cyan_shulker_box minecraft:purple_shulker_box minecraft:blue_shulker_box minecraft:brown_shulker_box minecraft:green_shulker_box minecraft:red_shulker_box minecraft:black_shulker_box"
    if (!istok("minecraft:chest minecraft:trapped_chest",block," "))
    {
        hmReplyToClient(client,"Unable to find chest to send! Stand on top of a chest first!");
        handle.unhookPattern(hook.name);
        return 1;
    }
    string target = gettok(hook.name,3," ");
    while (!isin(gettok(tags,1,","),"Items: "))
        tags = deltok(tags,1,",");
    while (!isin(gettok(tags,-1,","),"]"))
        tags = deltok(tags,-1,",");
    tags = "{" + tags + "}";
    ofstream file ("./halfMod/plugins/mailbox/" + target + ".mail",ios_base::app);
    if (file.is_open())
    {
        hmSendRaw("setblock " + pos + " minecraft:air replace\n" +
                  "clear " + client + " minecraft:iron_ingot 5");
        time_t cTime = time(NULL);
        file<<"0:3:"<<hmGetPlayerInfo(client).name<<"="<<cTime<<"="<<tags<<"="<<msgWith;
        file<<endl;
        file.close();
        if (hmIsPlayerOnline(target))
            hmReplyToClient(client,"You have sent a chest to " + hmGetPlayerInfo(target).name + "!");
        else
            hmReplyToClient(client,"You have sent a chest to " + hmGetPlayerData(target).name + "!");
        mbNotify(target);
    }
    else
        hmReplyToClient(client,"Error opening mailbox . . .");
    handle.unhookPattern(hook.name);
    return 1;
}

int sendChestFail(hmHandle &handle, hmHook hook, smatch args)
{
    string client = gettok(hook.name,2," ");
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
        hmPlayer mailbox;
        if (hmIsPlayerOnline(args[1]))
            mailbox = hmGetPlayerInfo(args[1]);
        else
            mailbox = hmGetPlayerData(args[1]);
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
            // [04:56:21] [Server thread/INFO]: nigathan has 0 experience levels
            handle.hookPattern("hmMBXP " + client + " " + stripFormat(lower(mailbox.name)),"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (\\S+) has ([0-9]+) experience levels$","sendXPCheck");
            hmSendRaw("experience query " + client + " levels");
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
    string client = args[1].str();
    int levels = stoi(args[2].str()), amount;
    string target = gettok(hook.name,3," ");
    if (levels >= xp2send)
    {
        ofstream file ("./halfMod/plugins/mailbox/" + target + ".mail",ios_base::app);
        if (file.is_open())
        {
            amount = getRealXP(levels) - getRealXP(levels - xp2send);
            hmSendRaw(data2str("experience add %s -%i",client.c_str(),amount));
            time_t cTime = time(NULL);
            file<<"0:1:"<<hmGetPlayerInfo(client).name<<"="<<cTime<<"="<<amount<<"="<<msgWith;
            file<<endl;
            file.close();
            if (hmIsPlayerOnline(target))
                hmReplyToClient(client,data2str("You have sent %i XP to ",amount) + hmGetPlayerInfo(target).name + "!");
            else
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
                    json += data2str("{\"text\":\"[%i] * \",\"color\":\"gold\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mb_open %i\"},\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Open\",\"color\":\"blue\"}]}}},",i,i);
                else
                    json += data2str("{\"text\":\"[%i] \",\"color\":\"gray\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mb_open %i\"},\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Re-open\",\"color\":\"blue\"}]}}},",i,i);
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
            hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"<-- [Prev]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mailbox " + data2str("%i",(page-1)) + "\"}},{\"text\":\"  +  \",\"color\":\"none\"},{\"text\":\"[Next] -->\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mailbox " + data2str("%i",(page+1)) + "\"}}]");
        else if (page > 1)
            hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"<-- [Previous Page]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mailbox " + data2str("%i",(page-1)) + "\"}}]");
        else if (page < pages)
            hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"[Next Page] -->\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mailbox " + data2str("%i",(page+1)) + "\"}}]");
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
                        ptrn = "id: \"(.+?)\"";
                        regex_search(att,ml,ptrn);
                        id = ml[1];
                        ptrn = "Count: ([0-9]+)b";
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
                        json += "{\"text\":\"[A] \",\"color\":\"red\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Click to claim this attatchment!\",\"color\":\"red\"}]}},\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mb_accept " + data2str("%i",item) + "\"}},";
                    else
                        json += "{\"text\":\"[A] \",\"color\":\"dark_blue\",\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"Attatchment already received!\",\"color\":\"dark_blue\"}]}}},";
                    if ((attType == 1) || (attType == 4))
                        json += data2str("{\"text\":\"%i\",\"color\":\"light_purple\"},{\"text\":\" Experience Points!\",\"color\":\"blue\"}]",xp);
                    else if ((attType == 2) || (attType == 5))
                        json += data2str("{\"text\":\"%i\",\"color\":\"light_purple\"},{\"text\":\"x \",\"color\":\"white\"},{\"text\":\"Item(s)!\",\"color\":\"blue\",\"hoverEvent\":{\"action\":\"show_item\",\"value\":\"{\\\\\"id\\\\\":\\\\\"%s\\\\\",\\\\\"Count\\\\\":%i}\"}}]",count,id.c_str(),count);
                    else if ((attType == 3) || (attType == 6))
                        json += "{\"text\":\"A nicely wrapped package!\",\"color\":\"blue\"}]";
                    hmSendRaw("tellraw " + client + json);
                    hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"[Return to sender]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mb_return " + data2str("%i",item) + "\"}},{\"text\":\"  +  \",\"color\":\"white\"},{\"text\":\"[Delete]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mb_delete " + data2str("%i",item) + "\"}}]");
                }
                else
                    hmSendRaw("tellraw " + client + " [\"[HM] \",{\"text\":\"[Delete]\",\"color\":\"aqua\",\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mb_delete " + data2str("%i",item) + "\"}}]");
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
                hmSendRaw("execute as " + client + " at @s run summon minecraft:item ^ ^1.6 ^1 " + att);
                hmReplyToClient(client,"You have claimed your item! Be sure to thank " + sender + "!");
            }
            else
            {
                hmSendRaw("execute as " + client + " at @s run summon minecraft:chest_minecart ^ ^1.6 ^1.5 " + att);
                hmReplyToClient(client,"You're package has arrived! Be sure to thank " + sender + "!");
            }
        }
        else
        {
            att = gettok(mail,3,"=");
            hmSendRaw("experience add " + client + " " + att + " points");
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
        if (n > 1)
            hmSendRaw(data2str("tellraw %s [\"[HM] You have \",{\"text\":\"%i\",\"color\":\"gold\",\"bold\":true},{\"text\":\" unread messages! \",\"color\":\"none\",\"bold\":false},{\"text\":\"Click here\",\"color\":\"blue\",\"underlined\":true,\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mailbox\"}},{\"text\":\" to open your mailbox!\",\"color\":\"none\",\"underlined\":false}]",client.c_str(),n));            
        else if (n > 0)
            hmSendRaw(data2str("tellraw %s [\"[HM] You have \",{\"text\":\"%i\",\"color\":\"gold\",\"bold\":true},{\"text\":\" unread message! \",\"color\":\"none\",\"bold\":false},{\"text\":\"Click here\",\"color\":\"blue\",\"underlined\":true,\"clickEvent\":{\"action\":\"run_command\",\"value\":\"/mailbox\"}},{\"text\":\" to open your mailbox!\",\"color\":\"none\",\"underlined\":false}]",client.c_str(),n));            
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

/*
tellraw nigathan ["[HM] ",{"text":"[A] ","color":"red","hoverEvent":{"action":"show_text","value":{"text":"","extra":[{"text":"Click to claim this attatchment!","color":"red"}]}},"clickEvent":{"action":"run_command","value":"/mb_accept 1"}},{"text":"1","color":"light_purple"},{"text":"x ","color":"white"},{"text":"Item(s)!","color":"blue","hoverEvent":{"action":"show_item","value":"{\\"id\\":\\"minecraft:music_disc_mall\\",\\"Count\\":1}"}}]
com.mojang.brigadier.exceptions.CommandSyntaxException: Invalid chat component: Unterminated object at line 1 column 374 path $[4].hoverEvent.value at position 17: ... nigathan <--[HERE]
[05:38:07] [Server thread/INFO]: Invalid chat component: Unterminated object at line 1 column 374 path $[4].hoverEvent.value at position 17: ... nigathan <--[HERE]
*/


