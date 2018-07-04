#include <iostream>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <regex>
#include <fstream>
#include <ctime>
#include <chrono>
#include <ratio>
#include "str_tok.h"
#include "halfmod.h"
using namespace std;

int mcVerInt(string version)
{
    version = gettok(version,1,"-");
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

hmHandle::hmHandle(const string &pluginPath, hmGlobal *global)
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
        auto end = events.find(HM_ONPLUGINEND);
        if (end != events.end())
            (*end->second.func)(*this);
        /*void (*end)(hmHandle&);
        *(void **) (&end) = dlsym(module, HM_ONPLUGINEND_FUNC);
        if (dlerror() == NULL)
            (*end)(*this);*/
        hmGlobal *global = recallGlobal(NULL);
        for (auto it = conVarNames.begin(), ite = conVarNames.end();it != ite;++it)
        {
            global->conVars.erase(hmFindConVarIt(*it));
        }
        for (auto it = global->pluginList.begin(), ite = global->pluginList.end();it != ite;++it)
        {
            if (it->path == modulePath)
            {
                global->pluginList.erase(it);
                break;
            }
        }
        loaded = false;
        dlclose(module);
    }
}

bool hmHandle::load(const string &pluginPath, hmGlobal *global)
{
    if (!loaded)
    {
        module = dlopen(pluginPath.c_str(), RTLD_LAZY|RTLD_LOCAL);
        if (!module)
            cerr<<"Error loading plugin \""<<pluginPath<<"\" "<<dlerror()<<endl;
        else
        {
            modulePath = pluginPath;
            char *error;
            int (*start)(hmHandle&,hmGlobal*);
            *(void **) (&start) = dlsym(module, HM_ONPLUGINSTART_FUNC);
            if ((error = dlerror()) != NULL)
                fputs(error, stderr);
            else
            {
                if ((*start)(*this,global))
                    return false;
                pluginName = deltok(deltok(deltok(deltok(modulePath,1,"/"),1,"/"),1,"/"),-1,".");
                global->pluginList.push_back({modulePath,pluginName,info.version});
                loaded = true;
                hookEvent(HM_ONPLUGINEND,HM_ONPLUGINEND_FUNC,false);
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
                hookEvent(HM_ONHSDISCONNECT,HM_ONHSDISCONNECT_FUNC,false);
                hookEvent(HM_ONSHUTDOWNPOST,HM_ONSHUTDOWNPOST_FUNC,false);
                hookEvent(HM_ONREHASHFILTER,HM_ONREHASHFILTER_FUNC,false);
                hookEvent(HM_ONGAMERULE,HM_ONGAMERULE_FUNC);
                hookEvent(HM_ONGLOBALMSG,HM_ONGLOBALMSG_FUNC);
                hookEvent(HM_ONPRINTMSG,HM_ONPRINTMSG_FUNC);
                hookEvent(HM_ONCONSOLERECV,HM_ONCONSOLERECV_FUNC);
                hookEvent(HM_ONPLUGINSLOADED,HM_ONPLUGINSLOADED_FUNC,false);
                hookEvent(HM_ONCUSTOM_1,HM_ONCUSTOM_1_FUNC);
                hookEvent(HM_ONCUSTOM_2,HM_ONCUSTOM_2_FUNC);
                hookEvent(HM_ONCUSTOM_3,HM_ONCUSTOM_3_FUNC);
                hookEvent(HM_ONCUSTOM_4,HM_ONCUSTOM_4_FUNC);
                hookEvent(HM_ONCUSTOM_5,HM_ONCUSTOM_5_FUNC);
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

void hmHandle::pluginInfo(const string &name, const string &author, const string &description, const string &version, const string &url)
{
    API_VER = API_VERSION;
    info.name = name;
    info.author = author;
    info.description = description;
    info.version = version;
    info.url = url;
}

hmInfo hmHandle::getInfo()
{
    return info;
}

int hmHandle::hookEvent(int event, const string &function, bool withSmatch)
{
    hmEvent tmpEvent;
    if (withSmatch)
        *(void **) (&tmpEvent.func_with) = dlsym(module, function.c_str());
    else
        *(void **) (&tmpEvent.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        tmpEvent.event = event;
        //events.push_back(tmpEvent);
        events.emplace(event,tmpEvent);
        return events.size();
    }
    return -1;
}

int hmHandle::hookEvent(int event, int (*func)(hmHandle&))
{
    hmEvent tmpEvent;
    tmpEvent.func = func;
    tmpEvent.event = event;
    //events.push_back(tmpEvent);
    events.emplace(event,tmpEvent);
    return events.size();
}

int hmHandle::hookEvent(int event, int (*func_with)(hmHandle&,std::smatch))
{
    hmEvent tmpEvent;
    tmpEvent.func_with = func_with;
    tmpEvent.event = event;
    //events.push_back(tmpEvent);
    events.emplace(event,tmpEvent);
    return events.size();
}

int hmHandle::regAdminCmd(const string &command, const string &function, int flags, const string &description)
{
    hmCommand tmpCommand;
    *(void **) (&tmpCommand.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        tmpCommand.cmd = command;
        tmpCommand.flag = flags;
        tmpCommand.desc = description;
        //cmds.push_back(tmpCommand);
        cmds.emplace(command,tmpCommand);
        return cmds.size();
    }
    return -1;
}

int hmHandle::regAdminCmd(const string &command, int (*func)(hmHandle&,const hmPlayer&,std::string[],int), int flags, const string &description)
{
    hmCommand tmpCommand;
    tmpCommand.func = func;
    tmpCommand.cmd = command;
    tmpCommand.flag = flags;
    tmpCommand.desc = description;
    //cmds.push_back(tmpCommand);
    cmds.emplace(command,tmpCommand);
    return cmds.size();
}

int hmHandle::regConsoleCmd(const string &command, const string &function, const string &description)
{
    hmCommand tmpCommand;
    *(void **) (&tmpCommand.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        tmpCommand.cmd = command;
        tmpCommand.flag = 0;
        tmpCommand.desc = description;
        //cmds.push_back(tmpCommand);
        cmds.emplace(command,tmpCommand);
        return cmds.size();
    }
    return -1;
}

int hmHandle::regConsoleCmd(const string &command, int (*func)(hmHandle&,const hmPlayer&,std::string[],int), const string &description)
{
    hmCommand tmpCommand;
    tmpCommand.func = func;
    tmpCommand.cmd = command;
    tmpCommand.flag = 0;
    tmpCommand.desc = description;
    //cmds.push_back(tmpCommand);
    cmds.emplace(command,tmpCommand);
    return cmds.size();
}

int hmHandle::unregCmd(const string &command)
{
    /*for (auto it = cmds.begin();it != cmds.end();)
    {
        if (it->cmd == command)
            it = cmds.erase(it);
        else
            ++it;
    }*/
    cmds.erase(command);
    return cmds.size();
}

int hmHandle::hookPattern(const string &name, const string &pattern, const string &function)
{
    hmHook tmpHook;
    *(void **) (&tmpHook.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        tmpHook.name = name;
        tmpHook.ptrn = pattern;
        tmpHook.rptrn = regex(pattern);
        hooks.push_back(tmpHook);
        return hooks.size();
    }
    return -1;
}

int hmHandle::hookPattern(const string &name, const string &pattern, int (*func)(hmHandle&,hmHook,std::smatch))
{
    hmHook tmpHook;
    tmpHook.func = func;
    tmpHook.name = name;
    tmpHook.ptrn = pattern;
    tmpHook.rptrn = regex(pattern);
    hooks.push_back(tmpHook);
    return hooks.size();
}

int hmHandle::unhookPattern(const string &name)
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

bool hmHandle::findCmd(const string &command, hmCommand &cmd)
{
    auto c = cmds.find(command);
    if (c != cmds.end())
    {
        cmd = c->second;
        return true;
    }
    return false;
}

bool hmHandle::findHook(const string &name, hmHook &hook)
{
    for (auto it = hooks.begin(), ite = hooks.end();it != ite;++it)
    {
        if (it->name == name)
        {
            hook = *it;
            return true;
        }
    }
    return false;
}

bool hmHandle::findEvent(int event, hmEvent &evnt)
{
    /*for (auto it = events.begin(), ite = events.end();it != ite;++it)
        if (it->event == event)
            return *it;*/
    auto c = events.find(event);
    if (c != events.end())
    {
        evnt = c->second;
        return true;
    }
    return false;
}

string hmHandle::getPath()
{
    return modulePath;
}

int hmHandle::createTimer(const string &name, long interval, const string &function, const string &args, short type)
{
    hmTimer temp;
    *(void **) (&temp.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        temp.type = type;
        temp.name = name;
        temp.interval = interval;
        temp.args = args;
        temp.last = chrono::high_resolution_clock::now();
        timers.push_back(temp);
        invalidTimers = true;
        return timers.size();
    }
    return -1;
}

int hmHandle::createTimer(const string &name, long interval, int (*func)(hmHandle&,std::string), const string &args, short type)
{
    hmTimer temp;
    temp.func = func;
    temp.type = type;
    temp.name = name;
    temp.interval = interval;
    temp.args = args;
    temp.last = chrono::high_resolution_clock::now();
    timers.push_back(temp);
    invalidTimers = true;
    return timers.size();
}

int hmHandle::killTimer(const string &name)
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

int hmHandle::triggerTimer(const string &name)
{
    int count = 0;
    chrono::nanoseconds interval;
    for (vector<hmTimer>::iterator it = timers.begin(), ite = timers.end();it != ite;++it)
    {
        if (it->name == name)
        {
            switch (it->type)
            {
                case MILLISECONDS:
                {
                    interval = (chrono::nanoseconds)(it->interval * 1000000);
                    break;
                }
                case MICROSECONDS:
                {
                    interval = (chrono::nanoseconds)(it->interval * 1000);
                    break;
                }
                case NANOSECONDS:
                {
                    interval = (chrono::nanoseconds)(it->interval);
                    break;
                }
                default:
                {
                    interval = (chrono::nanoseconds)(it->interval * 1000000000);
                }
            }
            it->last -= interval;
            count++;
        }
    }
    return count;
}

bool hmHandle::findTimer(const string &name, hmTimer &timer)
{
    for (vector<hmTimer>::iterator it = timers.begin(), ite = timers.end();it != ite;++it)
    {
        if (it->name == name)
        {
            timer = *it;
            return true;
        }
    }
    return false;
}

int hmExtension::createTimer(const string &name, long interval, const string &function, const string &args, short type)
{
    hmExtTimer temp;
    *(void **) (&temp.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        temp.type = type;
        temp.name = name;
        temp.interval = interval;
        temp.args = args;
        temp.last = chrono::high_resolution_clock::now();
        timers.push_back(temp);
        invalidTimers = true;
        return timers.size();
    }
    return -1;
}

int hmExtension::createTimer(const string &name, long interval, int (*func)(hmExtension&,std::string), const string &args, short type)
{
    hmExtTimer temp;
    temp.func = func;
    temp.type = type;
    temp.name = name;
    temp.interval = interval;
    temp.args = args;
    temp.last = chrono::high_resolution_clock::now();
    timers.push_back(temp);
    invalidTimers = true;
    return timers.size();
}

int hmExtension::killTimer(const string &name)
{
    for (vector<hmExtTimer>::iterator it = timers.begin();it != timers.end();)
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

int hmExtension::triggerTimer(const string &name)
{
    int count = 0;
    chrono::nanoseconds interval;
    for (vector<hmExtTimer>::iterator it = timers.begin(), ite = timers.end();it != ite;++it)
    {
        if (it->name == name)
        {
            switch (it->type)
            {
                case MILLISECONDS:
                {
                    interval = (chrono::nanoseconds)(it->interval * 1000000);
                    break;
                }
                case MICROSECONDS:
                {
                    interval = (chrono::nanoseconds)(it->interval * 1000);
                    break;
                }
                case NANOSECONDS:
                {
                    interval = (chrono::nanoseconds)(it->interval);
                    break;
                }
                default:
                {
                    interval = (chrono::nanoseconds)(it->interval * 1000000000);
                }
            }
            it->last -= interval;
            count++;
        }
    }
    return count;
}

bool hmExtension::findTimer(const string &name, hmExtTimer &timer)
{
    for (vector<hmExtTimer>::iterator it = timers.begin(), ite = timers.end();it != ite;++it)
    {
        if (it->name == name)
        {
            timer = *it;
            return true;
        }
    }
    return false;
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

hmExtension::hmExtension()
{
    loaded = false;
}

hmExtension::hmExtension(const string &extensionPath, hmGlobal *global)
{
    loaded = false;
    load(extensionPath,global);
}

void hmExtension::unload()
{
    if (loaded)
    {
        void (*end)(hmExtension&);
        *(void **) (&end) = dlsym(module, "onExtensionUnload");
        if (dlerror() == NULL)
            (*end)(*this);
        loaded = false;
        dlclose(module);
    }
}

bool hmExtension::load(const string &extensionPath, hmGlobal *global)
{
    if (!loaded)
    {
        module = dlopen(extensionPath.c_str(), RTLD_NOW|RTLD_GLOBAL);
        if (!module)
            cerr<<"Error loading extension \""<<extensionPath<<"\" "<<dlerror()<<endl;
        else
        {
            modulePath = extensionPath;
            char *error;
            int (*start)(hmExtension&,hmGlobal*);
            *(void **) (&start) = dlsym(module, "onExtensionLoad");
            if ((error = dlerror()) != NULL)
                fputs(error, stderr);
            else
            {
                if ((*start)(*this,global))
                    return false;
                extensionName = deltok(deltok(deltok(deltok(modulePath,1,"/"),1,"/"),1,"/"),-1,".");
                loaded = true;
            }
        }
    }
    return loaded;
}

void* hmExtension::getFunc(const string &func)
{
    if ((!loaded) || (!module))
        return NULL;
    return dlsym(module, func.c_str());
}

/*void* hmExtension::getFunc(string func)
{
    if ((!loaded) || (!module))
        return NULL;
    char *error;
    void *ptr;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return NULL;
    }
    return ptr;
}*/
/*int hmExtension::getFunc(int *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}
int hmExtension::getFunc(short *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}
int hmExtension::getFunc(bool *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}
int hmExtension::getFunc(double *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}
int hmExtension::getFunc(float *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}
int hmExtension::getFunc(long *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}
int hmExtension::getFunc(string *ptr, string func)
{
    if ((!loaded) || (!module))
        return 1;
    char *error;
    *(void **) (&ptr) = dlsym(module, func.c_str());
    if ((error = dlerror()) != NULL)
    {
        fputs(error, stderr);
        return 1;
    }
    return 0;
}*/

string hmExtension::getAPI()
{
    return API_VER;
}

bool hmExtension::isLoaded()
{
    return loaded;
}

void hmExtension::extensionInfo(const string &name, const string &author, const string &description, const string &version, const string &url)
{
    API_VER = API_VERSION;
    info.name = name;
    info.author = author;
    info.description = description;
    info.version = version;
    info.url = url;
}

hmInfo hmExtension::getInfo()
{
    return info;
}

string hmExtension::getPath()
{
    return modulePath;
}

string hmExtension::getExtension()
{
    return extensionName;
}

int hmExtension::hookPattern(const string &name, const string &pattern, const string &function)
{
    hmExtHook tmpHook;
    *(void **) (&tmpHook.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        tmpHook.name = name;
        tmpHook.ptrn = pattern;
        tmpHook.rptrn = regex(pattern);
        hooks.push_back(tmpHook);
        return hooks.size();
    }
    return -1;
}

int hmExtension::hookPattern(const string &name, const string &pattern, int (*func)(hmExtension&,hmExtHook,std::smatch))
{
    hmExtHook tmpHook;
    tmpHook.func = func;
    tmpHook.name = name;
    tmpHook.ptrn = pattern;
    tmpHook.rptrn = regex(pattern);
    hooks.push_back(tmpHook);
    return hooks.size();
}

int hmExtension::unhookPattern(const string &name)
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

hmConVar* hmHandle::createConVar(const string &name, const string &defaultValue, const string &description, short flags, bool hasMin, float min, bool hasMax, float max)
{
    hmGlobal *global = recallGlobal(NULL);
    hmConVar *exists = hmFindConVar(name);
    if (exists == nullptr)
    {
        hmConVar temp(name,defaultValue,description,flags,hasMin,min,hasMax,max);
        //global->conVars.push_back(temp);
        global->conVars.emplace(name,temp);
        conVarNames.push_back(name);
        //return &global->conVars.back();
        return hmFindConVar(name);
    }
    else
        return exists;
}

int hmHandle::hookConVarChange(hmConVar *cvar, const string &function)
{
    if (cvar == nullptr)
        return 1;
    hmConVarHook temp;
    *(void **) (&temp.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        cvar->hooks.push_back(temp);
        return 0;
    }
    return 2;
}

int hmHandle::hookConVarChange(hmConVar *cvar, int (*func)(hmConVar&,std::string,std::string))
{
    if (cvar == nullptr)
        return 1;
    hmConVarHook temp;
    temp.func = func;
    cvar->hooks.push_back(temp);
    return 0;
}

hmConVar* hmExtension::createConVar(const string &name, const string &defaultValue, const string &description, short flags, bool hasMin, float min, bool hasMax, float max)
{
    hmGlobal *global = recallGlobal(NULL);
    hmConVar *exists = hmFindConVar(name);
    if (exists == nullptr)
    {
        hmConVar temp(name,defaultValue,description,flags,hasMin,min,hasMax,max);
        //global->conVars.push_back(temp);
        global->conVars.emplace(name,temp);
        conVarNames.push_back(name);
        //return &global->conVars.back();
        return hmFindConVar(name);
    }
    else
        return exists;
}

int hmExtension::hookConVarChange(hmConVar *cvar, const string &function)
{
    if (cvar == nullptr)
        return 1;
    hmConVarHook temp;
    *(void **) (&temp.func) = dlsym(module, function.c_str());
    if (dlerror() == NULL)
    {
        cvar->hooks.push_back(temp);
        return 0;
    }
    return 2;
}

int hmExtension::hookConVarChange(hmConVar *cvar, int (*func)(hmConVar&,std::string,std::string))
{
    if (cvar == nullptr)
        return 1;
    hmConVarHook temp;
    temp.func = func;
    cvar->hooks.push_back(temp);
    return 0;
}

int hmHandle::totalCvars()
{
    return conVarNames.size();
}

int hmExtension::totalCvars()
{
    return conVarNames.size();
}

hmConVar::hmConVar()
{
    name = "";
}

hmConVar::hmConVar(const string &cname, const string &defVal, const string &description, short flag, bool hasMinimum, float minimum, bool hasMaximum, float maximum)
{
    name = cname;
    defaultValue = defVal;
    desc = description;
    flags = flag;
    hasMin = hasMinimum;
    min = minimum;
    hasMax = hasMaximum;
    max = maximum;
    value = defVal;
}

string hmConVar::getName()
{
    return name;
}

string hmConVar::getDesc()
{
    return desc;
}

string hmConVar::getDefault()
{
    return defaultValue;
}

void hmConVar::reset()
{
    setString(defaultValue);
}

string hmConVar::getAsString()
{
    return value;
}

bool hmConVar::getAsBool()
{
    if (lower(value) == "true")
        return true;
    if (lower(value) == "false")
        return false;
    regex ptrn("-?[0-9].*");
    if (regex_match(value,ptrn))
        return (bool)stoi(value);
    return false;
}

int hmConVar::getAsInt()
{
    regex ptrn("-?[0-9].*");
    if (regex_match(value,ptrn))
        return stoi(value);
    return 0;
}

float hmConVar::getAsFloat()
{
    regex ptrn("-?[0-9].*");
    if (regex_match(value,ptrn))
        return stof(value);
    return 0.0;
}

void hmConVar::setString(string newValue, bool wasSet)
{
    string oldValue = value;
    if ((flags & FCVAR_CONSTANT) == 0)
    {
        float temp;
        if (lower(newValue) == "true")
            temp = 1.0;
        else if (lower(newValue) == "false")
            temp = 0.0;
        else
        {
            regex ptrn("-?[0-9].*");
            if (regex_match(newValue,ptrn))
                temp = stof(newValue);
            else
                temp = 0.0;
        }
        if (hasMin)
        {
            if (temp < min)
            {
                if ((flags & FCVAR_GAMERULE) > 0)
                {
                    if (wasSet)
                        oldValue = newValue;
                    newValue = to_string((int)min);
                }
                else
                    newValue = to_string(min);
            }
        }
        if (hasMax)
        {
            if (temp > max)
            {
                if ((flags & FCVAR_GAMERULE) > 0)
                {
                    if (wasSet)
                        oldValue = newValue;
                    newValue = to_string((int)max);
                }
                else
                    newValue = to_string(max);
            }
        }
        value = newValue;
        if (newValue != oldValue)
        {
            if ((flags & FCVAR_GAMERULE) > 0)
                hmSendRaw("gamerule " + name + " " + value);
            if ((flags & FCVAR_NOTIFY) > 0)
                hmSendMessageAll("ConVar: " + name + " has changed to " + value);
            for (auto it = hooks.begin(), ite = hooks.end(); it != ite;++it)
            {
                if ((*it->func)(*this,oldValue,newValue))
                    break;
            }
        }
    }
    else if ((flags & FCVAR_GAMERULE) > 0)
    {
        if (newValue != value)
        {
            hmSendRaw("gamerule " + name + " " + value);
            if ((flags & FCVAR_NOTIFY) > 0)
                hmSendMessageAll("ConVar: " + name + " has changed to " + value);
        }
    }
}

void hmConVar::setBool(bool newValue)
{
    if (newValue)
        setString("true");
    else
        setString("false");
}

void hmConVar::setInt(int newValue)
{
    setString(to_string(newValue));
}

void hmConVar::setFloat(float newValue)
{
    setString(to_string(newValue));
}

hmGlobal *recallGlobal(hmGlobal *global)
{
    static hmGlobal *globe = global;
    return globe;
}

void hmSendRaw(string raw, bool output)
{
    int socket = recallGlobal(NULL)->hsSocket;
    if (socket)
    {
        if (output)
            hmOutQuiet(raw);
        raw += "\r";
        write(socket,raw.c_str(),raw.size());
    }
    else if (output)
        hmOutDebug("Error: No connection to the halfShell server . . .");
}

void hmServerCommand(const string &raw, bool output)
{
    if (raw.size() > 0)
        hmSendRaw("hs relay " + raw,output);
}

void hmReplyToClient(const string &client, const string &message)
{
    if ((client == "") || (client == "0") || (client == "#SERVER") || (!hmIsPlayerOnline(client)))
    {
        cout<<"[HM] "<<message<<endl;
        hmSendRaw("hs raw ::[PRINT] [HM] " + message,false);
    }
    else
    {
        int ver = mcVerInt(recallGlobal(NULL)->mcVer);
        string com = "tellraw ", pre = " [\"[HM] ", suf = "\"]";
        if (((ver <= 13003700) && (ver > 1000000)) || (ver < 107020))
        {
            com = "tell ";
            pre = " [HM] ";
            suf = "";
        }
        else
            hmSendRaw(com + stripFormat(client) + pre + message + suf,false);
    }
}

void hmReplyToClient(const hmPlayer &client, const string &message)
{
    hmReplyToClient(client.name,message);
}

void hmSendCommandFeedback(const string &client, const string &message)
{
    hmGlobal *global = recallGlobal(NULL);
    int ver = mcVerInt(global->mcVer);
    string com = "tellraw ", pre = " [\"[HM] ", suf = "\"]";
    if (((ver <= 13003700) && (ver > 1000000)) || (ver < 107020))
    {
        com = "tell ";
        pre = " [HM] ";
        suf = "";
    }
    cout<<"[HM] " + client + ": " + message<<endl;
    hmSendRaw("hs raw ::[PRINT] [HM] " + client + ": " + message,false);
    if (global->hsSocket)
    {
        string strippedClient = stripFormat(lower(client)), strippedTarget;
        for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
        {
            strippedTarget = stripFormat(lower(it->second.name));
            if (strippedTarget == strippedClient)
                hmSendRaw(com + strippedClient + pre + message + suf,false);
            else if (it->second.flags & FLAG_ADMIN)
                hmSendRaw(com + strippedTarget + pre + client + ": " + message + suf,false);
            else
                hmSendRaw(com + strippedTarget + pre + "ADMIN: " + message + suf,false);
        }
    }
    else
        hmOutDebug("Error: No connection to the halfShell server . . .");
}

void hmSendCommandFeedback(const hmPlayer &client, const string &message)
{
    hmSendCommandFeedback(client.name,message);
}

void hmSendMessageAll(const string &message)
{
    int ver = mcVerInt(recallGlobal(NULL)->mcVer);
    cout<<"[HM] " + message<<endl;
    if ((ver > 13003700) || ((ver >= 107020) && (ver < 1000000)))
        hmSendRaw("tellraw @a [\"[HM] " + message + "\"]",false);
    else
        hmSendRaw("say [HM] " + message,false);
    hmSendRaw("hs raw ::[GLOBAL] [HM] " + message,false);
}

bool hmIsPlayerOnline(string client)
{
    client = stripFormat(lower(client));
    hmGlobal *global = recallGlobal(NULL);
    /*for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
        if (stripFormat(lower(it->name)) == client)
            return true;*/
    auto c = global->players.find(client);
    if (c != global->players.end())
        return true;
    return false;
}

hmPlayer hmGetPlayerInfo(string client)
{
    client = stripFormat(lower(client));
    hmGlobal *global = recallGlobal(NULL);
    /*for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
        if (stripFormat(lower(it->name)) == client)
            return *it;*/
    auto c = global->players.find(client);
    if (c != global->players.end())
        return c->second;
    return hmPlayer();
}

unordered_map<std::string,hmPlayer>::iterator hmGetPlayerIterator(string client)
{
    client = stripFormat(lower(client));
    hmGlobal *global = recallGlobal(NULL);
    /*vector<hmPlayer>::iterator ite = global->players.end();
    for (vector<hmPlayer>::iterator it = global->players.begin();it != ite;++it)
        if (stripFormat(lower(it->name)) == client)
            return it;
    return ite;*/
    unordered_map<std::string,hmPlayer>::iterator c = global->players.find(client);
    return c;
}

hmPlayer* hmGetPlayerPtr(string client)
{
    client = stripFormat(lower(client));
    hmGlobal *global = recallGlobal(NULL);
    /*vector<hmPlayer>::iterator ite = global->players.end();
    for (vector<hmPlayer>::iterator it = global->players.begin();it != ite;++it)
        if (stripFormat(lower(it->name)) == client)
            return it;
    return ite;*/
    unordered_map<std::string,hmPlayer>::iterator c = global->players.find(client);
    if (c != global->players.end())
        return &c->second;
    return nullptr;
}

string hmGetPlayerUUID(string client)
{
    return hmGetPlayerPtr(stripFormat(lower(client)))->uuid;
}
string hmGetPlayerIP(string client)
{
    return hmGetPlayerPtr(stripFormat(lower(client)))->ip;
}

int hmGetPlayerFlags(string client)
{
    if (client == "#SERVER")
        return FLAG_ROOT;
    return hmGetPlayerPtr(stripFormat(lower(client)))->flags;
}

void hmOutQuiet(const string &text)
{
    if (!recallGlobal(NULL)->quiet)
    {
        cout<<"[<<] "<<text<<endl;
        hmSendRaw("hs raw ::[PRINT] [<<]" + text,false);
    }
}

void hmOutVerbose(const string &text)
{
    if (recallGlobal(NULL)->verbose)
    {
        cout<<"[..] "<<text<<endl;
        hmSendRaw("hs raw ::[PRINT] [..]" + text,false);
    }
}

void hmOutDebug(const string &text)
{
    if (recallGlobal(NULL)->debug)
    {
        cerr<<"[DEBUG] "<<text<<endl;
        hmSendRaw("hs raw ::[PRINT] [DEBUG]" + text,false);
    }
}

hmPlayer hmGetPlayerData(string client)
{
    hmGlobal *global = recallGlobal(NULL);
    string line;
    hmPlayer temp = { client, 0, "", "", 0, 0, "", 0, "", "" };
    client = stripFormat(lower(client));
    /*for (vector<hmAdmin>::iterator it = global->admins.begin(), ite = global->admins.end();it != ite;++it)
    {
        if (lower(it->client) == client)
        {
            temp.flags = it->flags;
            break;
        }
    }*/
    auto c = global->admins.find(client);
    if (c != global->admins.end())
        temp.flags = c->second.flags;
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
                    temp.join = stoi(gettok(line,2,"="));
                    break;
                }
                case 4: // quit time
                {
                    temp.quit = stoi(gettok(line,2,"="));
                    break;
                }
                case 5: // quit message
                {
                    temp.quitmsg = deltok(line,1,"=");
                    break;
                }
                case 6: // last death time
                {
                    temp.death = stoi(gettok(line,2,"="));
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

string stripFormat(const string &str)
{
    regex ptrn ("(§.)");
    return regex_replace(str,ptrn,"");
}

void hmLog(string data, int logType, string logFile)
{
    hmGlobal *global = recallGlobal(NULL);
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
    hmGlobal *global = recallGlobal(NULL);
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
                    //targList.push_back(global->players[rand() % int(global->players.size())]);
                    auto it = global->players.begin();
                    for (int r = rand() % int(global->players.size());r;--r,++it);
                    targList.push_back(it->second);
                    return 1;
                }
            }
        }
    }
    else if (filter & FILTER_FLAG)
    {
        if (target.at(0) == '!')
            c = 5;
        else
            c = 4;
        for (auto it = target.begin()+(c-4), ite = target.end();it != ite;++it)
            flags |= FLAGS[*it-97];
    }
    vector<hmPlayer> temp;
    //for (vector<hmPlayer>::iterator it = global->players.begin(), ite = global->players.end();it != ite;++it)
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
    {
        if ((filter & FILTER_UUID) && (it->second.uuid == target))
            targList.push_back(it->second);
        else if ((filter & FILTER_IP) && (it->second.ip == target))
            targList.push_back(it->second);
        else switch (c)
        {
            case 1:
            {
                if (it->first == client)
                    targList.push_back(it->second);
                break;
            }
            case 2:
            {
                if (it->first != client)
                    targList.push_back(it->second);
                break;
            }
            case 3:
            {
                targList.push_back(it->second);
                break;
            }
            case 4:
            {
                if (it->second.flags & flags)
                    targList.push_back(it->second);
                break;
            }
            case 5:
            {
                if ((it->second.flags & flags) == 0)
                    targList.push_back(it->second);
                break;
            }
            case 6:
            {
                if (it->first != client)
                    temp.push_back(it->second);
                break;
            }
            default:
            {
                if (isin(it->first,target))
                    targList.push_back(it->second);
            }
        }
    }
    if ((c == 6) && (temp.size() > 0))
        targList.push_back(temp[randint(0,9999999) % int(temp.size())]);
    return targList.size();
}

int hmProcessTargets(const hmPlayer &client, const string &target, vector<hmPlayer> &targList, int filter)
{
    return hmProcessTargets(client.name,target,targList,filter);
}

int hmProcessTargetsPtr(string client, string target, vector<hmPlayer*> &targList, int filter)
{
    int FLAGS[26] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554431 };
    hmGlobal *global = recallGlobal(NULL);
    if (!global->players.size())
        return 0;
    targList.clear();
    client = stripFormat(lower(client));
    target = stripFormat(lower(target));
    int c = 0, flags = 0;
    if (filter & FILTER_NAME)
    {
        if (target.at(0) == '@')
            return 0;
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
                    //targList.push_back(&global->players[rand() % int(global->players.size())]);
                    auto it = global->players.begin();
                    for (int r = rand() % int(global->players.size());r;--r,++it);
                    targList.push_back(&it->second);
                    return 1;
                }
            }
        }
    }
    else if (filter & FILTER_FLAG)
    {
        if (target.at(0) == '!')
            c = 5;
        else
            c = 4;
        for (auto it = target.begin()+(c-4), ite = target.end();it != ite;++it)
            flags |= FLAGS[*it-97];
    }
    vector<hmPlayer*> temp;
    //for (vector<hmPlayer>::iterator it = global->players.begin(), ite = global->players.end();it != ite;++it)
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
    {
        if ((filter & FILTER_UUID) && (it->second.uuid == target))
            targList.push_back(&it->second);
        else if ((filter & FILTER_IP) && (it->second.ip == target))
            targList.push_back(&it->second);
        else switch (c)
        {
            case 1:
            {
                if (it->first == client)
                    targList.push_back(&it->second);
                break;
            }
            case 2:
            {
                if (it->first != client)
                    targList.push_back(&it->second);
                break;
            }
            case 3:
            {
                targList.push_back(&it->second);
                break;
            }
            case 4:
            {
                if (it->second.flags & flags)
                    targList.push_back(&it->second);
                break;
            }
            case 5:
            {
                if ((it->second.flags & flags) == 0)
                    targList.push_back(&it->second);
                break;
            }
            case 6:
            {
                if (it->first != client)
                    temp.push_back(&it->second);
                break;
            }
            default:
            {
                if (isin(it->first,target))
                    targList.push_back(&it->second);
            }
        }
    }
    if ((c == 6) && (temp.size() > 0))
        targList.push_back(temp[randint(0,9999999) % int(temp.size())]);
    return targList.size();
}

int hmProcessTargetsPtr(const hmPlayer &client, const string &target, vector<hmPlayer*> &targList, int filter)
{
    return hmProcessTargetsPtr(client.name,target,targList,filter);
}

bool hmIsPluginLoaded(const string &nameOrPath, const string &version)
{
    hmGlobal *global = recallGlobal(NULL);
    for (auto it = global->pluginList.begin(), ite = global->pluginList.end();it != ite;++it)
    {
        if ((nameOrPath == it->name) || (nameOrPath == it->path))
        {
            if (version.size() > 0)
            {
                if (version == it->version)
                    return true;
            }
            else
                return true;
        }
    }
    return false;
}

string hmGetPluginVersion(const string &nameOrPath)
{
    hmGlobal *global = recallGlobal(NULL);
    for (auto it = global->pluginList.begin(), ite = global->pluginList.end();it != ite;++it)
        if ((nameOrPath == it->name) || (nameOrPath == it->path))
            return it->version;
    return "";
}

int hmAddConsoleFilter(const string &name, const string &ptrn, short blocking, int event)
{
    hmConsoleFilter temp;
    hmGlobal *global = recallGlobal(NULL);
    temp.filter = regex(ptrn);
    temp.event = event;
    if (blocking & HM_BLOCK_OUTPUT)
        temp.blockOut = true;
    if (blocking & HM_BLOCK_EVENT)
        temp.blockEvent = true;
    if (blocking & HM_BLOCK_HOOK)
        temp.blockHook = true;
    temp.name = name;
    global->conFilter->push_back(temp);
    return global->conFilter->size();
}

int hmRemoveConsoleFilter(const string &name)
{
    hmGlobal *global = recallGlobal(NULL);
    for (auto it = global->conFilter->begin(), ite = global->conFilter->end();it != ite;)
    {
        if (it->name == name)
            it = global->conFilter->erase(it);
        else
            ++it;
    }
    return global->conFilter->size();
}

int hmWritePlayerDat(string client, const string &data, const string &ignore, bool ifNotPresent)
{
    string filename = "./halfMod/userdata/" + stripFormat(lower(client)) + ".dat";
    fstream file (filename,ios_base::in);
    string line, lines = data;
    if (file.is_open())
    {
        while (getline(file,line))
        {
            if ((ifNotPresent) && (line == data))
                return -1;
            if (!istok(ignore,gettok(line,1,"="),"="))
                lines = appendtok(lines,line,"\n");
        }
        file.close();
    }
    file.open(filename,ios_base::out|ios_base::trunc);
    if (file.is_open())
    {
        file<<lines;
        file.close();
    }
    else
        return 1;
    return 0;
}

hmConVar *hmFindConVar(const string &name)
{
    hmGlobal *global = recallGlobal(NULL);
    /*for (auto it = global->conVars.begin(), ite = global->conVars.end();it != ite;++it)
    {
        if (it->getName() == name)
            return &*it;
    }*/
    auto c = global->conVars.find(name);
    if (c != global->conVars.end())
        return &c->second;
    return nullptr;
}

unordered_map<string,hmConVar>::iterator hmFindConVarIt(const string &name)
{
    hmGlobal *global = recallGlobal(NULL);
    /*auto ite = global->conVars.end();
    for (auto it = global->conVars.begin();it != ite;++it)
    {
        if (it->getName() == name)
            return it;
    }
    return ite;*/
    auto c = global->conVars.find(name);
    return c;
}

int hmResolveFlag(char flag)
{
    if (!isalpha(flag))
        return 0;
    static int FLAGS[26] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554431 };
    return FLAGS[tolower(flag)-97];
}

int hmResolveFlags(const string &flags)
{
    int out = 0;
    for (auto it = flags.begin(), ite = flags.end();it != ite;++it)
        out |= hmResolveFlag(*it);
    return out;
}

