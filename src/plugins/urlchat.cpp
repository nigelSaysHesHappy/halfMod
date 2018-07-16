#include <thread>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.1.6"

void generateScript();
void* urlLookup(void *vrl);
string html2txt(string text);

bool url_enabled = true;

static void (*addConfigButtonCallback)(string,string,int,std::string (*)(std::string,int,std::string,std::string));

string toggleButton(string name, int socket, string ip, string client)
{
    if (url_enabled)
    {
        hmFindConVar("urlchat_enabled")->setBool(false);
        return "URL's will no longer appear in chat . . .";
    }
    hmFindConVar("urlchat_enabled")->setBool(true);
    return "URL's will now appear in chat . . .";
}

extern "C" {

int onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo(  "URLChat",
                        "nigel",
                        "Clickable URL's in chat",
                        VERSION,
                        "http://clicking.in.chat.makes.me.justca.me/to/the/url" );
    handle.hookPattern("url","(https?://[^ ]{3,})","urlParse");
    handle.hookConVarChange(handle.createConVar("urlchat_enabled","true","Enable the URL Chat plugin",0,true,0.0,true,1.0),"cvarChange");
    for (auto it = global->extensions.begin(), ite = global->extensions.end();it != ite;++it)
    {
        if (it->getExtension() == "webgui")
        {
            *(void **) (&addConfigButtonCallback) = it->getFunc("addConfigButtonCallback");
            (*addConfigButtonCallback)("toggleUrlChat","Toggle URL Chat",FLAG_CVAR,&toggleButton);
        }
    }
    remove("./halfMod/plugins/url.out");
    generateScript();
    return 0;
}

int cvarChange(hmConVar &cvar, string oldVal, string newVal)
{
    url_enabled = cvar.getAsBool();
    return 0;
}

int urlParse(hmHandle &handle, hmHook hook, smatch args)
{
    if (url_enabled)
    {
        string url = args[1].str();
        pthread_t new_url;
        pthread_create(&new_url,NULL,urlLookup,(void*)new string(move(url)));
    }
    return 0;
}

}

inline bool exists(const string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

void generateScript()
{
    if (exists("./halfMod/plugins/urlhead.sh"))
        return;
    ofstream file ("./halfMod/plugins/urlhead.sh");
    if (file.is_open())
    {
        file<<"#!/bin/bash\n\
if [[ \"$@\" != \"\" ]]; then\n\
    if [[ `curl -I -L -s \"$@\" | grep -i --after-context=20 -E '^HTTP\\/[0-9]\\.?[0-9]? 200' | grep -i '^Content-Type:' | grep -i 'text\\/html'` ]]; then\n\
        out=`curl -L -s \"$@\" | grep -i --after-context=10 '<title>' | tr -d \"\\n\\r\" | sed -e 's/.*<title>\\s*\\([^<]*\\)<\\/title>.*/\\1/Ig'`\n\
        if [[ \"$out\" == \"\" ]]; then\n\
            echo \"$@\"\n\
            echo \"$@\" >> urlhead_error.log\n\
        else\n\
            echo \"$out\"\n\
        fi\n\
    else\n\
        echo \"$@\"\n\
    fi\n\
fi";
        file.close();
    }
}

void* urlLookup(void *vrl)
{
    //string *url = (string*)vrl;
    string url(move(*((string*)vrl)));
    while (exists("./halfMod/plugins/url.out"))
        sleep(1);
    fstream file;
    file.open("./halfMod/plugins/url.out",ios_base::out);
    while (!file.is_open())
    {
        sleep(1);
        file.open("./halfMod/plugins/url.out",ios_base::out);
    }
    file.close();
    system(string("/bin/bash ./halfMod/plugins/urlhead.sh \"" + url + "\" > ./halfMod/plugins/url.out").c_str());
    file.open("./halfMod/plugins/url.out",ios_base::in);
    if (file.is_open())
    {
        string title;
        getline(file,title);
        title = nospace(title);
        //hmSendMessageAll("\",{\"text\":\"[Header] \",\"color\":\"dark_aqua\"},{\"text\":\"" + html2txt(title) + "\",\"color\":\"blue\",\"underlined\":true,\"clickEvent\":{\"action\":\"open_url\",\"value\":\"" + url + "\"},\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"" + url + "\"}]}}},\"");
        hmSendRaw("tellraw @a [\"[HM] \",{\"text\":\"[Header] \",\"color\":\"dark_aqua\"},{\"text\":\"" + html2txt(title) + "\",\"color\":\"blue\",\"underlined\":true,\"clickEvent\":{\"action\":\"open_url\",\"value\":\"" + url + "\"},\"hoverEvent\":{\"action\":\"show_text\",\"value\":{\"text\":\"\",\"extra\":[{\"text\":\"" + url + "\"}]}}}]",false);
        file.close();
    }
    remove("./halfMod/plugins/url.out");
    delete (string*)vrl;
    pthread_exit(NULL);
}

#define MAX_CODES 16
string ampCodes[MAX_CODES] = {
    "exclamation",
    "quot",
    "amp",
    "apos",
    "add",
    "lt",
    "equal",
    "gt",
    "OElig",
    "oelig",
    "Scaron",
    "scaron",
    "Yuml",
    "fnof",
    "circ",
    "tilde"
};

string codeValues[MAX_CODES] = { "!","\"","&","'","+","<","=",">","Œ","œ","Š","š","Ÿ","ƒ","ˆ","˜" };

#define MAX_CHAR_CODES 96
string charCodes[MAX_CHAR_CODES] = {
    "nbsp",
    "iexcl",
    "cent",
    "pound",
    "curren",
    "yen",
    "brvbar",
    "sect",
    "uml",
    "copy",
    "ordf",
    "laquo",
    "not",
    "shy",
    "reg",
    "macr",
    "deg",
    "plusmn",
    "sup2",
    "sup3",
    "acute",
    "micro",
    "para",
    "middot",
    "cedil",
    "sup1",
    "ordm",
    "raquo",
    "frac14",
    "frac12",
    "frac34",
    "iquest",
    "Agrave",
    "Aacute",
    "Acirc",
    "Atilde",
    "Auml",
    "Aring",
    "AElig",
    "Ccedil",
    "Egrave",
    "Eacute",
    "Ecirc",
    "Euml",
    "Igrave",
    "Iacute",
    "Icirc",
    "Iuml",
    "ETH",
    "Ntilde",
    "Ograve",
    "Oacute",
    "Ocirc",
    "Otilde",
    "Ouml",
    "times",
    "Oslash",
    "Ugrave",
    "Uacute",
    "Ucirc",
    "Uuml",
    "Yacute",
    "THORN",
    "szlig",
    "agrave",
    "aacute",
    "acirc",
    "atilde",
    "auml",
    "aring",
    "aelig",
    "ccedil",
    "egrave",
    "eacute",
    "ecirc",
    "euml",
    "igrave",
    "iacute",
    "icirc",
    "iuml",
    "eth",
    "ntilde",
    "ograve",
    "oacute",
    "ocirc",
    "otilde",
    "ouml",
    "divide",
    "oslash",
    "ugrave",
    "uacute",
    "ucirc",
    "uuml",
    "yacute",
    "thorn",
    "yuml"
};

string html2txt(string text)
{
    regex ptrn ("&(.+?);");
    smatch ml;
    string code;
    while (regex_search(text,ml,ptrn))
    {
        code = ml[1].str();
        if (code[0] == '#')
        {
            code.erase(0,1);
            code = char(stoi(code));
            text = regex_replace(text,regex(ml[0].str()),code);
        }
        else
        {
            for (int i = 0;i < MAX_CODES;i++)
            {
                if (ampCodes[i] == code)
                {
                    code = codeValues[i];
                    break;
                }
            }
            if (code.size() > 1)
            {
                for (int i = 0;i < MAX_CHAR_CODES;i++)
                {
                    if (charCodes[i] == code)
                    {
                        code = char(i+160);
                        break;
                    }
                }
            }
            if (code.size() > 1)
                code = "?";
            text = regex_replace(text,regex(ml[0].str()),code);
        }
    }
    return text;
}

