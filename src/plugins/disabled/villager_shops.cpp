#include <iostream>
#include "halfmod.h"
#include "str_tok.h"
#include "nbtmap.h"
using namespace std;

#define VERSION		"v0.2.6"

struct Trade
{
    string buy[2] = { "", "" };
    string sell = "{id:\"minecraft:air\",Count:1b}";
};

struct TradeStock
{
    int uses;
    int stock;
    string buy[2];
    int buyCount[2];
    string buyID[2];
    string buyTags[2];
    string sell;
    int sellCount;
    string sellNoCount;
};

bool shopsEnable = true;
float shopsDistance = 0.0;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string toggleButton(string name, int socket, string ip, string client)
{
    if (shopsEnable)
    {
        hmFindConVar("villager_shop_enabled")->setBool(false);
        return "Players can no longer create villager shops . . .";
    }
    hmFindConVar("villager_shop_enabled")->setBool(true);
    return "Players can now create villager shops . . .";
}

int cChange(hmConVar &cvar, string oldVar, string newVar);
int cDisChange(hmConVar &cvar, string oldVar, string newVar);
int newShopCmd(hmHandle &handle, const hmPlayer &client, string args[], int argc);
int restockShopCmd(hmHandle &handle, const hmPlayer &client, string args[], int argc);

int shopGetCoords(hmHandle &handle, hmHook hook, rens::smatch args);
int diamondCheck(hmHandle &handle, hmHook hook, rens::smatch args);
int failDiamondCheck(hmHandle &handle, hmHook hook, rens::smatch args);
int chestCheck(hmHandle &handle, hmHook hook, rens::smatch args);
int failChestCheck(hmHandle &handle, hmHook hook, rens::smatch args);
int findVillager(hmHandle &handle, hmHook hook, rens::smatch args);
int failFindVillager(hmHandle &handle, hmHook hook, rens::smatch args);
int stockChestCheck(hmHandle &handle, hmHook hook, rens::smatch args);
int failStockChestCheck(hmHandle &handle, hmHook hook, rens::smatch args);

vector<Trade> parseChestNBT(NBTList &items);
vector<TradeStock> parseTradeNBT(string starting, string recipes);
NBTList parseChestNBT(NBTList &items, vector<TradeStock> &trades);
string getDimension(int dim);

extern "C" int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo(	"Villager Shops",
                        "nigel",
                        "Create survival shops using villagers",
                        VERSION,
                        "http://villagers.justca.me/to/sell/your/items");
    handle.hookConVarChange(handle.createConVar("villager_shop_enabled","true","Enable or disable creating villager shops.",FCVAR_NOTIFY,true,0.0,true,1.0),&cChange);
    handle.hookConVarChange(handle.createConVar("villager_restock_distance","0","Set the maximum distance you can be away from a villager to restock it, 0 disables this.",FCVAR_NOTIFY,true,0.0),&cDisChange);
    handle.regConsoleCmd("hm_newshop",&newShopCmd,"Create a new villager shop");
    handle.regConsoleCmd("hm_restockshop",&restockShopCmd,"Restock and receive payments from a villager shop");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            (*addConfigButtonCallback)("toggleVShops","Toggle Villager Shops",FLAG_CVAR,&toggleButton);
        }
    }
    return 0;
}

int cChange(hmConVar &cvar, string oldVar, string newVar)
{
    shopsEnable = cvar.getAsBool();
    return 0;
}

int cDisChange(hmConVar &cvar, string oldVar, string newVar)
{
    shopsDistance = cvar.getAsFloat();
    return 0;
}

int newShopCmd(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    // hm_newshop <profession,career|random> [noai=false] [invulnerable=false] [name=null]
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <profession#[,career#=1]|random> [noai=false] [invulnerable=false] [custom_name] - noai and invulnerable each cost 1 diamond, custom_name is free.");
        return 1;
    }
    string info;
    int cost = 0;
    if (lower(args[1]) == "random")
        info = "Profession:" + to_string(rand()%5) + ",Career:1";
    else if (stringisnum(args[1],0,4))
        info = "Profession:" + args[1] + ",Career:1";
    else if ((numtok(args[1],",") == 2) && (stringisnum(gettok(args[1],1,","),0,4)) && (stringisnum(gettok(args[1],2,","),1,4)))
        info = "Profession:" + gettok(args[1],1,",") + ",Career:" + gettok(args[1],2,",");
    else
    {
        hmReplyToClient(client,"Invalid profession/career ID. Syntax: '3,2' for weapon smith, or 'random'.");
        hmReplyToClient(client,"Usage: " + args[0] + " <profession#[,career#=1]|random> [noai=false] [invulnerable=false] [custom_name] - Costs 1 diamond + noai and invulnerable each cost 1 more diamond, custom_name is free.");
        return 1;
    }
    if (argc > 2)
    {
        if (lower(args[2]) == "true")
        {
            info += ",NoAI:1b";
            cost++;
        }
        if (argc > 3)
        {
            if (lower(args[3]) == "true")
            {
                info += ",Invulnerable:1b";
                cost++;
            }
            if (argc > 4)
                info = info + ",CustomName:\"\\\\\"" + args[4] + "\\\\\"\"";
        }
    }
    if (cost > 0)
    {
        // [04:16:59] [Server thread/INFO]: No items were found on player nigathan
        handle.hookPattern("shop " + client.name + " " + to_string(cost) + " " + info,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: No items were found on player (" + client.name + ").?.?$",&failDiamondCheck);
        // [04:17:50] [Server thread/INFO]: Found 64 matching items on player nigathan
        handle.hookPattern("shop " + client.name + " " + to_string(cost) + " " + info,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: Found ([0-9]+) matching items on player (" + client.name + ")$",&diamondCheck);
        hmSendRaw("clear " + client.name + " minecraft:diamond 0");
    }
    else
    {
        handle.hookPattern("shop " + client.name + " " + to_string(cost) + " " + info,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + client.name + ") has the following entity data: (\\{.*\\})$",&shopGetCoords);
        hmSendRaw("data get entity " + client.name);
    }
    return 0;
}

int diamondCheck(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    int count = stoi(args[1].str());
    string client = gettok(hook.name,2," ");
    int cost = stoi(gettok(hook.name,3," "));
    if (count < cost)
    {
        hmReplyToClient(client,"You only have " + to_string(count) + " diamond(s), but it costs " + to_string(cost) + " to create your requested villager! Remove some upgrades or try again when you can pay the fee!");
        return 1;
    }
    handle.hookPattern(hook.name,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + client + ") has the following entity data: (\\{.*\\})$",&shopGetCoords);
    hmSendRaw("data get entity " + client);
    return 1;
}

int failDiamondCheck(hmHandle &handle, hmHook hook, rens::smatch args)
{
    string client = gettok(hook.name,2," ");
    hmReplyToClient(client,"You need " + gettok(hook.name,3," ") + " diamonds to create your requested villager. A villager without noai and invulnerable is always free!");
    handle.unhookPattern(hook.name);
    return 1;
}

int shopGetCoords(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    string client = args[1].str();//, dimension = getDimension(stoi(args[2].str()));
    NBTCompound nbt (args[2].str());
    string dimension = getDimension(stoi(nbt["Dimension"]));
    NBTList position (nbt.get("Pos"));
    string posa[3] = { gettok(position[0],1,"."), gettok(position[1],1,"."), gettok(position[2],1,".") };
    for (int i = 0;i < 3;i++)
        if (*posa[i].begin() == '-')
            posa[i] = to_string(stol(posa[i])-1);
    string pos = posa[0] + " " + posa[1] + " " + posa[2];
    if (gettok(hook.name,1," ") == "shop")
    {
        // [04:13:56] [Server thread/INFO]: -236, 67, -40 has the following block data: {x: -236, y: 67, z: -40, Items: [], id: "minecraft:chest", Lock: ""}
        handle.hookPattern(hook.name + "\n" + dimension + "\n" + pos,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (-?[0-9]+), (-?[0-9]+), (-?[0-9]+) has the following block data: (\\{.*\\})$",&chestCheck);
        // [04:13:16] [Server thread/INFO]: The target block is not a block entity
        handle.hookPattern(hook.name + "\n" + dimension + "\n" + pos,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: The target block is not a block entity$",&failChestCheck);
        hmSendRaw("execute in " + dimension + " run data get block " + pos);
    }
    else
    {
        handle.hookPattern(hook.name + " " + dimension + "\n" + pos,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (.+?) has the following entity data: (\\{.*\\})$",&findVillager);
        handle.hookPattern(hook.name + " " + dimension + "\n" + pos,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: No entity was found$",&failFindVillager);
        if (shopsDistance == 0.0)
            hmSendRaw("execute in " + dimension + " positioned " + pos + " run data get entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "]");
        else
            hmSendRaw("execute in " + dimension + " positioned " + pos + " run data get entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "distance=.." + to_string(shopsDistance) + "]");
    }
    return 1;
}

int chestCheck(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    string pos = args[1].str() + " " + args[2].str() + " " + args[3].str();
    NBTCompound NBT (args[4].str());
    NBTList ITEMS (NBT.get("Items"));
    string client = gettok(hook.name,2," "), tags = args[5].str(), block = args[6].str();
    string cost = gettok(hook.name,3," "), dimension = gettok(hook.name,2,"\n");
    string nbt = "{" + deltok(deltok(deltok(gettok(hook.name,1,"\n"),1," "),1," "),1," ") + ",CareerLevel:2147483647,Willing:0b,Offers:{Recipes:[";
    if (!istok("minecraft:chest minecraft:trapped_chest",strleft(strright(NBT["id"],-1),-1)," "))
    {
        hmReplyToClient(client,"Unable to find chest! Stand on top of a chest first!");
        return 1;
    }
    //cout<<"[...] "<<tags<<endl;
    vector<Trade> trades = parseChestNBT(ITEMS);
    //{maxUses:2147483647,uses:2147483646,rewardExp:0b,sell:{id:"minecraft:wither_skeleton_skull",Count:1b},buy:{id:"minecraft:diamond",Count:1b}}]}
    if (trades.size() > 0)
    {
        for (auto it = trades.begin(), ite = trades.end();it != ite;++it)
        {
            nbt = nbt + "{maxUses:2147483647,uses:2147483646,rewardExp:0b,sell:" + it->sell + ",buy:" + it->buy[0];
            if (it->buy[1].size() > 0)
                nbt = nbt + ",buyB:" + it->buy[1];
            nbt += "},";
        }
        nbt.erase(nbt.size()-1);
        nbt += "]},Tags:[\"" + client + "\",";
        for (int i = 0;i < trades.size();i++)
            nbt = nbt + "\"" + to_string(i) + ";1\",";
        nbt.erase(nbt.size()-1);
        nbt += "]}";
        hmSendRaw("execute in " + dimension + " run data merge block " + pos + " {Items:" + ITEMS.get() + "}");
        if (cost != "0")
            hmSendRaw("clear " + client + " minecraft:diamond " + cost);
        hmSendRaw("execute in " + dimension + " run summon minecraft:villager " + pos + " " + nbt);
    }
    else
        hmReplyToClient(client,"The chest appears to be empty!");
    return 1;
}

int failChestCheck(hmHandle &handle, hmHook hook, rens::smatch args)
{
    string client = gettok(hook.name,2," ");
    hmReplyToClient(client,"Unable to find chest! Stand on top of a chest first!");
    handle.unhookPattern(hook.name);
    return 1;
}

int restockShopCmd(hmHandle &handle, const hmPlayer &client, string args[], int argc)
{
    handle.hookPattern("restock " + client.name,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (" + client.name + ") has the following entity data: (\\{.*\\})$",&shopGetCoords);
    hmSendRaw("data get entity " + client.name);
    return 0;
}

int findVillager(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    string client = gettok(hook.name,2," "), dimension = gettok(gettok(hook.name,1,"\n"),3," "), pos = gettok(hook.name,2,"\n");
    string villy = args[1].str();
    NBTCompound nbt (args[2].str());
    NBTCompound recipes (nbt.get("Offers"));
    handle.hookPattern("restock\n" + client + "\n" + villy + "\n" + nbt["Tags"] + "\n" + recipes["Recipes"] + "\n" + dimension + "\n" + pos,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (-?[0-9]+), (-?[0-9]+), (-?[0-9]+) has the following block data: (\\{.*\\})$",&stockChestCheck);
    handle.hookPattern("restock\n" + client + "\n" + villy + "\n" + nbt["Tags"] + "\n" + recipes["Recipes"] + "\n" + dimension + "\n" + pos,"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: The target block is not a block entity$",&failStockChestCheck);
    hmSendRaw("execute in " + dimension + " run data get block " + pos);
    return 1;
}

int failFindVillager(hmHandle &handle, hmHook hook, rens::smatch args)
{
    string client = gettok(hook.name,2," ");
    hmReplyToClient(client,"Unable to locate your villager :(");
    handle.unhookPattern(hook.name);
    return 1;
}

int stockChestCheck(hmHandle &handle, hmHook hook, rens::smatch args)
{
    handle.unhookPattern(hook.name);
    string pos = args[1].str() + " " + args[2].str() + " " + args[3].str(), dimension = gettok(hook.name,6,"\n");
    string shopName = gettok(hook.name,3,"\n");
    string client = gettok(hook.name,2,"\n");
    NBTCompound nbt (args[4].str());
    NBTList items (nbt.get("Items"));
    // parse all data, give client all payments, overwrite villager's tags to remaining stock
    vector<TradeStock> trades = parseTradeNBT(gettok(hook.name,4,"\n"),gettok(hook.name,5,"\n"));
    for (auto it = trades.begin(), ite = trades.end();it != ite;++it)
    {
        if (it->uses > 0)
        {
            if (it->buy[0].size() > 0)
                hmSendRaw("give " + client + " " + it->buyID[0] + it->buyTags[0] + " " + to_string(it->uses*it->buyCount[0]));
            if (it->buy[1].size() > 0)
                hmSendRaw("give " + client + " " + it->buyID[1] + it->buyTags[1] + " " + to_string(it->uses*it->buyCount[1]));
        }
    }
    if (!istok("minecraft:chest minecraft:trapped_chest",strleft(strright(nbt["id"],-1),-1)," "))
    {
        string stock = "\"" + client + "\"";
        int i = 0;
        for (auto it = trades.begin(), ite = trades.end();it != ite;++it)
        {
            stock = stock + ",\"" + to_string(i) + ";" + to_string(it->stock) + "\"";
            i++;
        }
        if (shopsDistance == 0.0)
            hmSendRaw("execute in " + dimension + " positioned " + pos + " run data merge entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "] {Tags:[" + stock + "]}");
        else
            hmSendRaw("execute in " + dimension + " positioned " + pos + " run data merge entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "distance=.." + to_string(shopsDistance) + "] {Tags:[" + stock + "]}");
        hmReplyToClient(client,"Unable to find chest, nothing restocked! Here are your payments from " + gettok(hook.name,3,"\n"));
        return 1;
    }
    // restock the villager
    NBTList tags (parseChestNBT(items,trades));
    tags.push_back("\"" + client + "\"");
    string stock = "{Tags:" + tags.get() + ",Offers:{Recipes:[";
    if (shopsDistance == 0.0)
        stock = "execute in " + dimension + " positioned " + pos + " run data merge entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "] " + stock;
    else
        stock = "execute in " + dimension + " positioned " + pos + " run data merge entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "distance=.." + to_string(shopsDistance) + "] " + stock;
    for (auto it = trades.begin(), ite = trades.end();it != ite;++it)
    {
        stock = stock + "{maxUses:2147483647,uses:" + to_string(it->uses) + ",rewardExp:0b,sell:" + it->sell + ",buy:" + it->buy[0];
        if (it->buy[1].size() > 0)
            stock = stock + ",buyB:" + it->buy[1];
        stock += "},";
    }
    stock.erase(stock.size()-1);
    stock += "]}}";
    hmSendRaw(stock + "\nexecute in " + dimension + " run data merge block " + pos + " {Items:" + items.get() + "}");
    hmReplyToClient(client,"Your " + shopName + " shop has been restocked and you have received all payments!");
    return 1;
}

int failStockChestCheck(hmHandle &handle, hmHook hook, rens::smatch args)
{
    string client = gettok(hook.name,2,"\n"), dimension = gettok(hook.name,6,"\n"), pos = gettok(hook.name,7,"\n");
    hmReplyToClient(client,"Unable to find chest, nothing restocked! Here are your payments from " + gettok(hook.name,3,"\n"));
    handle.unhookPattern(hook.name);
    vector<TradeStock> trades = parseTradeNBT(gettok(hook.name,4,"\n"),gettok(hook.name,5,"\n"));
    string stock = "\"" + client + "\"";
    int i = 0;
    for (auto it = trades.begin(), ite = trades.end();it != ite;++it)
    {
        if (it->uses > 0)
        {
            if (it->buy[0].size() > 0)
                hmSendRaw("give " + client + " " + it->buyID[0] + it->buyTags[0] + " " + to_string(it->uses*it->buyCount[0]));
            if (it->buy[1].size() > 0)
                hmSendRaw("give " + client + " " + it->buyID[1] + it->buyTags[1] + " " + to_string(it->uses*it->buyCount[1]));
        }
        stock = stock + ",\"" + to_string(i) + ";" + to_string(it->stock) + "\"";
        i++;
    }
    if (shopsDistance == 0.0)
        hmSendRaw("execute in " + dimension + " positioned " + pos + " run data merge entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "] {Tags:[" + stock + "]}");
    else
        hmSendRaw("execute in " + dimension + " positioned " + pos + " run data merge entity @e[type=minecraft:villager,limit=1,sort=nearest,tag=" + client + "distance=.." + to_string(shopsDistance) + "] {Tags:[" + stock + "]}");
    return 1;
}

vector<Trade> parseChestNBT(NBTList &items)
{
    vector<Trade> trades;
    for (auto itv = items.begin();itv != items.end();)
    {
        NBTCompound item (*itv);
        int slot = stoi(item.get("Slot"));
        item.erase("Slot");
        if (slot < 9)
            items.erase(itv);
        else
            ++itv;
        if (slot < 9)
        {
            Trade trade;
            trade.sell = item.get();
            trade.buy[0] = "{id:\"minecraft:dirt\",Count:1b}";
            trades.push_back(trade);
        }
        else if (slot < 18)
        {
            slot -= 9;
            if (trades.size() < slot+1)
            {
                Trade trade;
                trade.buy[0] = item.get();
                trades.push_back(trade);
            }
            else
                trades[slot].buy[0] = item.get();
        }
        else
        {
            slot -= 18;
            if (trades.size() < slot+1)
            {
                Trade trade;
                trade.buy[0] = item.get();
                trades.push_back(trade);
            }
            else
                trades[slot].buy[1] = item.get();
        }
    }
    return trades;
}

vector<TradeStock> parseTradeNBT(string starting, string recipes)
{
    vector<TradeStock> trades;
    NBTList items (recipes);
    for (int i = 0, j = numtok(starting,",");i < items.size();i++)
    {
        for (int k = 0;k < j;k++)
        {
            string token = strleft(strright(gettok(starting,k+1,", "),-1),-1);
            //if (iswm(token,to_string(i) + ";*"))
            if (gettok(token,1,";") == to_string(i))
            {
                TradeStock trade;
                trade.stock = stoi(gettok(token,2,";"));
                trade.uses = 2147483647 - trade.stock;
                trades.push_back(trade);
                break;
            }
        }
        if (trades.size() <= i)
        {
            TradeStock trade;
            trade.uses = 2147483647;
            trades.push_back(trade);
        }
    }
    auto recipe = trades.begin();
    for (auto it = items.begin(), ite = items.end();it != ite;++it,++recipe)
    {
        NBTCompound trade (*it);
        NBTCompound buy (trade.get("buy"));
        NBTCompound buyB (trade.get("buyB"));
        NBTCompound sell (trade.get("sell"));
        if (buy.size() > 0)
        {
            recipe->buy[0] = trade.get("buy");
            recipe->buyID[0] = buy.get("id");
            recipe->buyID[0] = recipe->buyID[0].substr(1,recipe->buyID[0].size()-2);
            recipe->buyCount[0] = stoi(buy.get("Count"));
            recipe->buyTags[0] = buy.get("Tags");
        }
        if (buyB.size() > 0)
        {
            recipe->buy[1] = trade.get("buyB");
            recipe->buyID[1] = buyB.get("id");
            recipe->buyID[1] = recipe->buyID[1].substr(1,recipe->buyID[1].size()-2);
            recipe->buyCount[1] = stoi(buyB.get("Count"));
            recipe->buyTags[1] = buyB.get("Tags");
        }
        if (sell.size() > 0)
        {
            recipe->sell = trade.get("sell");
            recipe->sellCount = stoi(sell.get("Count"));
            sell.erase("Count");
            recipe->sellNoCount = sell.get();
        }
        recipe->uses = stoi(trade.get("uses")) - recipe->uses;
        recipe->stock -= recipe->uses;
        if (recipe->uses < 0)
            recipe->uses = 0;
    }
    return trades;
}

NBTList parseChestNBT(NBTList &items, vector<TradeStock> &trades)
{
    int counts[9] = { 0 };
    NBTList unused;
    int index = 0;
    for (auto it = items.begin(), ite = items.end();it != ite;++it)
    {
        NBTCompound item (*it);
        int slot = stoi(item.get("Slot")) % 9;
        NBTCompound sell;
        if (slot < trades.size())
            sell.parse(trades[slot].sell);
        if ((item.get("id") == sell.get("id")) && (item.get("tag") == sell.get("tag")))
            counts[slot] += stoi(item.get("Count"));
        else
        {
            item["Slot"] = to_string(index++) + "b";
            unused.push_back(item.get());
        }
    }
    NBTList stock;
    for (size_t i = 0;i < trades.size();i++)
    {
        int mod = counts[i] % trades[i].sellCount;
        if (mod != 0)
        {
            counts[i] -= mod;
            string temp = "{Slot: " + to_string(index++) + "b, ";
            temp = temp.append(trades[i].sellNoCount,1,trades[i].sellNoCount.size()-2) + ", Count: " + to_string(mod) + "b}";
            unused.push_back(temp);
        }
        trades[i].stock += (counts[i]/trades[i].sellCount);
        trades[i].uses = 2147483647 - trades[i].stock;
        stock.push_back("\"" + to_string(i) + ";" + to_string(trades[i].stock) + "\"");
    }
    items = unused;
    return stock;
}

string getDimension(int dim)
{
    switch (dim)
    {
        case -1:
            return "the_nether";
        case 0:
            return "overworld";
        case 1:
            return "the_end";
        default:
            return "";
    }
}

