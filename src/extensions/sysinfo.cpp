#include "halfmod.h"
#include <ctime>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <sys/vtimes.h>
using namespace std;

#define VERSION "0.1"

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

void init(){
    FILE* file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while(fgets(line, 128, file) != NULL){
        if (strncmp(line, "processor", 9) == 0) numProcessors++;
    }
    fclose(file);
    if (numProcessors == 0)
        numProcessors = 1;
}

long parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    long i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = stol(string(p));
    return i;
}

extern "C" {

int onExtensionLoad(hmExtension &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.extensionInfo("System Info",
                         "nigel",
                         "Get memory/CPU/etc info about the system.",
                         VERSION,
                         "http://system.info.justca.me/through/an/extension");
    init();
    return 0;
}

int getMCPID()
{
    hmGlobal *global = recallGlobal(NULL);
    if ((global->hsSocket == -1) || (global->mcScreen.size() < 1))
        return -1;
    int pid = -1;
    system("screen -ls > screen.pid");
    //ifstream file ("screen.pid");
    FILE* file;
    file = fopen("screen.pid", "r");
    char line[128];
    string buf;
    regex ptrn ("\\s*([0-9]+)\\." + global->mcScreen + "\\s");
    smatch ml;
    while (fgets(line,128,file) != NULL)
    {
        buf = line;
        if (regex_search(buf,ml,ptrn))
        {
            pid = stoi(ml[1].str());
            break;
        }
    }
    fclose(file);
    remove("screen.pid");
    return pid+5;
}

double getCurrentValue(){
    struct tms timeSample;
    clock_t now;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        percent = (timeSample.tms_stime - lastSysCPU) +
            (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        //percent /= numProcessors;
        percent *= 100;
    }
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return percent;
}

// returns the average total CPU usage since the last call or since boot if the first call
double getCPUSince()
{
    FILE* file;
    char line[128];
    static long pastCPU = 0;
    long presentCPU;
    static time_t pastTime = 0;
    time_t curTime = time(NULL);
    double percent;
    file = fopen("/proc/stat", "r");
    smatch ml;
    regex ptrn ("cpu\\s+[0-9]+ [0-9]+ [0-9]+ ([0-9]+) ");
    string buf;
    while (fgets(line,128,file) != NULL)
    {
        buf = line;
        if (regex_search(buf,ml,ptrn))
        {
            presentCPU = stol(ml[1].str());
            if (pastTime == 0)
            {
                FILE* file2;
                file2 = fopen("/proc/uptime", "r");
                fgets(line,128,file2);
                fclose(file2);
                pastTime = curTime;
                pastTime -= stol(string(line));
            }
            break;
        }
    }
    fclose(file);
    if (curTime == pastTime)
        percent = 100.0-(double(presentCPU-pastCPU)/double(numProcessors));
    else
        percent = 100.0-(double(presentCPU-pastCPU)/double(numProcessors*(curTime-pastTime)));
    pastCPU = presentCPU;
    pastTime = curTime;
    return percent;
}

long getTotalMemoryFree(long &totalMem)
{
    FILE* file;
    long freeMem;
    char line[128];
    smatch ml;
    regex ptrnA ("MemTotal:\\s+([0-9]+) .*");
    regex ptrnB ("MemFree:\\s+([0-9]+) .*");
    file = fopen("/proc/meminfo", "r");
    string buf;
    while (fgets(line,128,file) != NULL)
    {
        buf = line;
        if (regex_search(buf,ml,ptrnA))
            totalMem = stol(ml[1].str());
        else if (regex_search(buf,ml,ptrnB))
            freeMem = stol(ml[1].str());
    }
    fclose(file);
    return freeMem;
}

long getCurrentMemUsage()
{ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    long result = -1;
    char line[128];
    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

long getPIDMemUsage(int pid)
{ //Note: this value is in KB!
    FILE* file = fopen(string("/proc/" + to_string(pid) + "/status").c_str(), "r");
    long result = -1;
    char line[128];
    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

long getUptime()
{
    FILE* file;
    char line[64];
    file = fopen("/proc/uptime", "r");
    fgets(line,64,file);
    fclose(file);
    return stol(string(line));
}

}

