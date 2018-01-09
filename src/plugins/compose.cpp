#include "halfmod.h"
#include "str_tok.h"
#include <vector>
using namespace std;

#define VERSION "v0.0.5"

class composeVariable
{
    public:
        bool constant;
        string varName;
        string plain;
        string flags;
        string uuidLeast;
        string uuidMost;
        string id;
        string tags;
        hmPlayer loaded;
        composeVariable();
        composeVariable(string var, string player);
        void reload();
        string getMember(string member);
};

class variableList
{
    public:
        vector<composeVariable> list;
        composeVariable getVar(string name);
        vector<composeVariable>::iterator getIt(string name);
        void setVar(composeVariable var);
        int unsetVar(string name);
} variables;

const string memList[] = { "constant",
                           "name",
                           "uuid",
                           "uuidleast",
                           "uuidmost",
                           "ip",
                           "quit",
                           "death",
                           "id",
                           "tags" };

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Compose",
                      "nigel",
                      "Build and run commands from entiy data",
                      VERSION,
                      "http://all.these.commands.justca.me/with/my/name/on/them");
    handle.regAdminCmd("hm_comp_set","compSet",FLAG_CUSTOM1,"Set a variable");
    handle.regAdminCmd("hm_comp_unset","compUnset",FLAG_CUSTOM1,"Unset a variable");
    handle.regAdminCmd("hm_comp_edit","compEdit",FLAG_CUSTOM1,"Edit a variable's members");
    handle.regAdminCmd("hm_compose","composeCmd",FLAG_CUSTOM1,"Compose variables into a command");
    return 0;
}

int compSet(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 3)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <variable> <player|selector|constant>");
        return 1;
    }
    handle.hookPattern(client + "\n" + args[1] + "\n" + args[2],"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/INFO\\]: (.+) has the following entity data: (\\{.*\\})$","catchData");
    handle.hookPattern(client + "\n" + args[1] + "\n" + args[2],"^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[Server thread/ERROR\\]: Couldn't execute command for Server: data get entity .+$","catchDataFail");
    hmSendRaw("data get entity " + args[2]);
    return 0;
}

int catchDataFail(hmHandle &handle, hmHook hook, smatch args)
{
    composeVariable var(gettok(hook.name,2,"\n"),gettok(hook.name,3,"\n"));
    var.constant = true;
    variables.setVar(var);
    hmReplyToClient(gettok(hook.name,1,"\n"),"Set constant variable: ${" + var.varName + "} = `" + var.plain + "'");
    handle.unhookPattern(hook.name);
    return 1;
}

int catchData(hmHandle &handle, hmHook hook, smatch args)
{
    composeVariable var(gettok(hook.name,2,"\n"),gettok(hook.name,3,"\n"));
    var.constant = false;
    var.loaded.name = args[1].str();
    var.tags = args[2].str();
    regex ptrn ("UUIDLeast: (-?[0-9]+?L)");
    smatch ml;
    if (regex_search(var.tags,ml,ptrn))
        var.uuidLeast = ml[1].str();
    ptrn = "UUIDMost: (-?[0-9]+?L)";
    if (regex_search(var.tags,ml,ptrn))
        var.uuidMost = ml[1].str();
    ptrn = "id: (\".+?\")";
    if (regex_search(var.tags,ml,ptrn))
        var.id = ml[1].str();
    else
        var.id = "player";
    var.reload();
    variables.setVar(var);
    hmReplyToClient(gettok(hook.name,1,"\n"),"Set entity variable: ${" + var.varName + "} = `" + var.loaded.name + "'");
    handle.unhookPattern(hook.name);
    return 1;
}

int compUnset(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <variable>");
        return 1;
    }
    if (variables.unsetVar(args[1]))
        hmReplyToClient(client,"Unknown variable: " + args[1]);
    else
        hmReplyToClient(client,"Variable has been unset: " + args[1]);
    return 0;
}

int compEdit(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 4)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <variable> <member> <value>");
        return 1;
    }
    composeVariable var = variables.getVar(args[1]);
    if (var.varName != args[1])
    {
        hmReplyToClient(client,"Unknown variable: " + args[1]);
        return 1;
    }
    int member = 0;
    for (;member < 10;member++)
        if (args[2] == memList[member])
            break;
    switch (member)
    {
        case 0:  // constant
        {
            var.plain = args[3];
            break;
        }
        case 1:  // name
        {
            var.loaded.name = args[3];
            break;
        }
        case 2:  // uuid
        {
            var.loaded.uuid = args[3];
            break;
        }
        case 3:  // uuidleast
        {
            var.uuidLeast = args[3];
            break;
        }
        case 4:  // uuidmost
        {
            var.uuidMost = args[3];
            break;
        }
        case 5:  // ip
        {
            var.loaded.ip = args[3];
            break;
        }
        case 6:  // quit
        {
            var.loaded.quitmsg = args[3];
            break;
        }
        case 7:  // death
        {
            var.loaded.deathmsg = args[3];
            break;
        }
        case 8:  // id
        {
            var.id = args[3];
            break;
        }
        case 9:  // tags
        {
            var.tags = args[3];
            break;
        }
        default: // custom
        {
            bool found = false;
            if (var.loaded.custom != "")
            {
                for (int i = 1;i <= numtok(var.loaded.custom,"\n");i++)
                {
                    if (args[2] == gettok(gettok(var.loaded.custom,i,"\n"),1,"="))
                    {
                        found = true;
                        var.loaded.custom = puttok(var.loaded.custom,args[2] + "=" + args[3],i,"\n");
                        break;
                    }
                }
            }
            if (!found)
                var.loaded.custom = appendtok(var.loaded.custom,args[2] + "=" + args[3],"\n");
        }
    }
    variables.setVar(var);
    hmReplyToClient(client,"Set variable member: ${" + var.varName + "." + args[2] + "} = `" + args[3] + "'");
    return 0;
}

int composeCmd(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <command|variables ...>");
        return 1;
    }
    string com = args[1], var, mem, tmpCom;
    composeVariable vari;
    for (int i = 2;i < argc;i++)
        com = com + " " + args[i];
    regex ptrn ("\\$\\{([^.\\}]+)\\.?([^\\}]*)}");
    smatch ml;
    while (regex_search(com,ml,ptrn))
    {
        var = ml[1].str();
        mem = ml[2].str();
        tmpCom = ml.prefix();
        if (var.size() > 0)
        {
            vari = variables.getVar(var);
            if (mem.size() < 1)
            {
                if (vari.constant)
                    mem = "constant";
                else
                    mem = "name";
            }
            tmpCom += vari.getMember(mem);
        }
        tmpCom += ml.suffix();
        com = tmpCom;
    }
    //hmReplyToClient(client,"Composed: " + com);
    hmServerCommand(com);
    return 0;
}

}

composeVariable::composeVariable()
{
    constant = true;
}

composeVariable::composeVariable(string var, string player)
{
    varName = var;
    plain = player;
}

void composeVariable::reload()
{
    static const int FLAGS[26] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554431 };
    string player = loaded.name;
    if (player.size() > 0)
    {
        if (hmIsPlayerOnline(player))
            loaded = hmGetPlayerInfo(player);
        else
            loaded = hmGetPlayerData(player);
        flags.clear();
        if (loaded.flags > 0)
            for (int i = 0;i < 26;i++)
                if (loaded.flags & FLAGS[i])
                    flags += i+97;
    }
}

string composeVariable::getMember(string member)
{
    int memb = 0;
    for (;memb < 10;memb++)
        if (member == memList[memb])
            break;
    switch (memb)
    {
        case 0:  // constant
        {
            return plain;
        }
        case 1:  // name
        {
            return loaded.name;
        }
        case 2:  // uuid
        {
            return loaded.uuid;
        }
        case 3:  // uuidleast
        {
            return uuidLeast;
        }
        case 4:  // uuidmost
        {
            return uuidMost;
        }
        case 5:  // ip
        {
            return loaded.ip;
        }
        case 6:  // quit
        {
            return loaded.quitmsg;
        }
        case 7:  // death
        {
            return loaded.deathmsg;
        }
        case 8:  // id
        {
            return id;
        }
        case 9:  // tags
        {
            return tags;
        }
        default: // custom
        {
            if (loaded.custom != "")
                for (int i = 1;i <= numtok(loaded.custom,"\n");i++)
                    if (member == gettok(gettok(loaded.custom,i,"\n"),1,"="))
                        return deltok(gettok(loaded.custom,i,"\n"),1,"=");
        }
    }
    return "";
}

composeVariable variableList::getVar(string name)
{
    for (auto it = list.begin(), ite = list.end();it != ite;++it)
        if (it->varName == name)
            return *it;
    return composeVariable();
}

void variableList::setVar(composeVariable var)
{
    bool found = false;
    for (auto it = list.begin(), ite = list.end();it != ite;++it)
    {
        if (it->varName == var.varName)
        {
            found = true;
            *it = var;
            break;
        }
    }
    if (!found)
        list.push_back(var);
}

vector<composeVariable>::iterator variableList::getIt(string name)
{
    auto ite = list.end();
    for (auto it = list.begin();it != ite;++it)
        if (it->varName == name)
            return it;
    return ite;
}

int variableList::unsetVar(string name)
{
    for (auto it = list.begin(), ite = list.end();it != ite;++it)
    {
        if (it->varName == name)
        {
            list.erase(it);
            return 0;
        }
    }
    return 1;
}


/*
hm_comp_set
[HM] Usage: hm_comp_set <variable> <player|selector|constant>
hm_comp_unset
[HM] Usage: hm_comp_unset <variable>
hm_comp_edit
[HM] Usage: hm_comp_edit <variable> <member> <value>
hm_comp_set rand @e[limit=1]
[<<] data get entity @e[limit=1]
[01:14:11] [Server thread/INFO]: Zombie has the following entity data: {...}
[HM] Set entity variable: ${rand} = `Zombie'
hm_comp_edit rand poo flesh
[HM] Set variable member: ${rand.poo} = `flesh'
hm_compose ${rand} eats ${rand.poo}
[HM] Composed: Zombie eats flesh
hm_compose ${rand.constant} = ${rand.name} and eats his own ${rand.poo}
[HM] Composed: @e[limit=1] = Zombie and eats his own flesh
hm_comp_set var value
[<<] data get entity value
[01:15:56] [Server thread/ERROR]: Couldn't execute command for Server: data get entity value
[HM] Set constant variable: ${var} = `value'
[01:15:56] [Server thread/INFO]: No entity was found
hm_compose summon ${rand} ~ ~ ~ {NoAI:${var},CustomName:"${rand.constant}",Dimension:${var.name},Pos:[${var.0},${var.1},${rand.name}],Tags:["${rand.poo}"]}
[HM] Composed: summon Zombie ~ ~ ~ {NoAI:value,CustomName:"@e[limit=1]",Dimension:,Pos:[,,Zombie],Tags:["flesh"]}
hm_comp_unset rand
[HM] Variable has been unset: rand
hm_compose where did ${rand} go? I need dat ${rand.poo}
[HM] Composed: where did  go? I need dat
*/



