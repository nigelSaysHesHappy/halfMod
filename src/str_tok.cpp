// str_tok.h written by NIGathan //Note: Needs a new name...
// Allows functionality of useful mIRC functions in C++ and more.
// Version 1.9d - Sun Oct  2 16:56:48 CDT 2016
// Added readloadedini_legacy() Tue Jun  6 00:46:58 MDT 2017
// Version 1.9e - Sun Oct  8 07:18:24 MDT 2017
// Version 1.9f - Tue Oct 10 05:22:26 MDT 2017
// Fixed overflows with strleft() and strright()

#include <iostream>
#include <string>
#include <locale>
#include <fstream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <sys/types.h>
//#include <regex.h>
#include "str_tok.h"

#ifdef STR_TOK_SS
#include <sstream>
#endif

using namespace std;

int randint(int lo, int hi)
{
    static bool seeded = false;
    if (!seeded) srand(time(NULL)), seeded = true;
    int i = (lo + rand()) % hi;
    if (i < lo) i += lo;
    return i;
}

int str2int(string str)
{
    return atoi(str.c_str());
    /*
    string ret;
    int i, r = 0, neg = 1;
    if (str[0] == '-')
    {
        neg = -1;
        str.assign(str,1,str.size());
    }
    for (int x = 0;x < int(str.size());x++)
    {
        i = (int)str[x];
        if ((i < 58) && (i > 47)) ret = ret + str[x];
        else break;
    }
    i = ret.size();
    for (int x = 0, z;x < i;x++)
    {
        z = 1;
        for (int y = (i-x)-1;y;y--) z = z * 10;
        r = r + (((int)ret[x] - 48) * z);
    }
    return (r*neg);
    */
}

float str2float(string str)
{
    return atof(str.c_str());
    /*
    string ret;
    int i, a, y, dec = 0;
    float c, z, r = 0.0f;
    for (int x = 0;x < int(str.size());x++)
    {
        a = (int)str[x];
        if ((a < 58) && (a > 47)) ret = ret + str[x];
        else if ((a == 46) && (!dec))
        {
            ret = ret + str[x];
            dec = x;
        }
        else break;
    }
    i = ret.size();
    if (!dec) dec = i;
    for (int x = 0;x < i;x++)
    {
        if (x == dec) continue;
        z = 1.0;
        if (x < dec)
        {
            for (y = (dec-x)-1;y;y--) z = z * 10.0f;
        }
        else for (y = ((i-dec)-x)+1;y;y--) z = z / 10.0f;
        c = (int)ret[x] - 48.0f;
        r = r + (c * z);
    }
    return r;
    */
}

string int2str(int n)
{
    //Depreciated.
    //Use 'string data2str(format...)'
    //Example:
    //        data2str("%i",420)
    int len[10], x, z, neg = 0;
    double y;
    if (n < 0)
    {
        neg = 1;
        n = n*-1;
    }
    for (len[0] = 9;len[0];len[0]--)
    {
        y = pow(10,double(len[0]));
        if (n >= int(y)) break;
    }
    len[0]++;
    for (x = 1;x < len[0];x++)
    {
        if (!n)
        {
            len[x] = 0;
            continue;
        }
        y = pow(10,double(len[0]-x));
        for (z = int(y);n > z;z += int(y)) {}
        if ((z > y) && (z != n)) len[x] = (z-int(y))/int(y), n -= z-int(y);
        else len[x] = z/int(y), n -= z;
    }
    for (len[x] = 0;len[x] < n;len[x]++) {}
    if (n - len[x] != 0) return ""; //error
    string ret;
    for (int i = 1;i <= x;i++) ret = ret + char(len[i]+48);
    if (neg == 1)
    {
        ret = "-" + ret;
    }
    return ret;
}

int len(string text)
{
    return text.size();
}

int numtok(string tokens, string delim)
{
    if (tokens.size() == 0) return 0;
	int y = 1;
    for (int x = 0; x < int(tokens.size()); x++)
    {
        if ((tokens.compare(x,1,delim) == 0) && (tokens.compare(x+1,1,delim) != 0) && (x+1 < int(tokens.size()))) y++;
    }
	return y;
}

string gettok(string tokens, int tok, string delim)
{
     int x = 1, y = 0, z = delim.size(), c = numtok(tokens,delim);
     string ret;
     if (tok < 0) tok = c+tok+1;
     //while (ret.size() < 1)
     //{
         for (int a = 0; a < int(tokens.size()); a++)
         {
             if (tokens.compare(a,z,delim) == 0) x++;
             if (x == tok) y = x;
             if (y > 0)
             {
                 if (x > y) break;
                 ret.append(tokens,a,1);
             }
         }
         //if (tok >= c) break;
         //tok++;
     //}
     if (ret.compare(0,z,delim) == 0) return ret.erase(0,z);
     else return ret;
}
/*
int isin(string text, string subtext, int pos = -1)
{
    int x, b = 0;
    for (x = 0; x < int(text.size()); x++)
    {
             if (text.compare(x,subtext.size(),subtext,0,subtext.size()) == 0)
             {
                b++;
                if (b >= pos)
                {
                    if (pos == 0) continue;
                    else break;
                }
             }
    }
    if (pos == 0) return b;
    if (b == pos) return x;
    if ((b) && (pos < 0)) return 1;
    return -1;
}
*/

int isin(string text, string subtext, int pos)
{
    int z = 0;
    for (int x = 0, s = subtext.size(), y = text.size();x < y;x++)
    {
        if ((x+s) > y) break;
        if (text.compare(x,s,subtext) == 0)
        {
            z++;
            if ((pos < 0) || (pos == z)) return x;
        }
    }
    if (pos == 0) return z;
    return -1;
}
/*
int isin(string text, string subtext, int pos)
{
    int a = 0, x, b = 0;
    for (x = 0; x < int(text.size()); x++)
    {
             if (text.compare(x,1,subtext,0,1) == 0)
             {
                a = 1;
                for (int y = 1; y < int(subtext.size()); y++)
                {
                    if (a == 0) break;
                    if (text.compare(x+y,1,subtext,y,1) != 0) a = 0;
                }
                if (a == 1) b++;
                if (a == 1 && b >= pos)
                {
                      if (pos == 0) continue;
                      else break;
                }
             }
    }
    if (pos == 0) return b;
    if (b == pos)
    {
            if (a == 1) return x;
            else return -1;
    }
    else if (pos > 0) return -1;
    else return a;
}
*/
string strremove(string text, string subtext)
{
    while (text.find(subtext) != string::npos) text.erase(text.find(subtext),subtext.size());
    return text;
}

string getqtok(string tokens, int tok, string delim)
{
     int x = 1, y = 0, z = delim.size(), par = 0, pars = isin(tokens,"\"",0), para = pars % 2, parb = pars-para, c = numtok(tokens,delim);
     string ret;
     if (tok < 0) tok = c+tok+1;
     //while (ret.size() < 1)
     //{
         for (int a = 0; a < int(tokens.size()); a++)
         {
             if (tokens.compare(a,1,"\"") == 0)
             {
                 par++;
                 if ((par < pars) || ((par == pars) && (!para))) continue;
             }
             if ((tokens.compare(a,z,delim) == 0) && ((par % 2 == 0) || (par > parb))) x++;
             if (x == tok) y = x;
             if (y > 0)
             {
                  if (x > y) break;
                  ret.append(tokens,a,1);
             }
         }
         //if (tok >= c) break;
         //tok++;
     //}
     if (ret.compare(0,z,delim) == 0) return ret.erase(0,z);
     else return ret;
}

bool iswm(string text, string subtext)
{
    int a = -1, b = numtok(subtext,"*"), r = 0, l = 0;
    string str;
    for (int y = 1; y <= b; y++)
    {
        if (l > 0) { str = gettok(gettok(subtext,y-1,"*"),l,"?"); l--; }
        str = gettok(subtext,y,"*");
        if (str.compare(0,1,"?") == 0)
        {
           str.erase(0,1);
           a++;
        }
        else if (isin(str,"?",1) > 0)
        {
             if (str.compare(str.size()-2,1,"?") == 0) a++;
             else l++;
             str = gettok(str,1,"?");
        }
        else l = 0;
        r = 0;
        if (y == 1 && subtext.compare(0,1,"*") != 0 && a == -1)
        {
              for (int s = 0; s < int(str.size())*-1; s--)
              {
                  if (text.compare(s,1,str,s,1) != 0)
                  {
                     r = 1;
                     break;
                  }
              }
              if (r == 1) break;
        }
        if (str.size() == 0) continue;
        r = 0;
        for (int g = 1; g <= isin(text,str,0); g++)
        {
            if (a < isin(text,str,g))
            {
               a = isin(text,str,g)+str.size();
               r = 2;
               break;
            }
            if (g == isin(text,str,0)) r = 1;
        }
        if (r != 2)
        {
              r = 1;
              break;
        }
    }
    if (r == 1) return false;
    else return true;
}

bool stringisalpha(string text)
{
    for (int x = 0; x < int(text.size()); x++)
    {
        if (!isalpha(text[x])) return false;
    }
    return true;
}

bool stringisalnum(string text)
{
    for (int x = 0; x < int(text.size()); x++)
    {
        if (!isalnum(text[x])) return false;
    }
    return true;
}

bool stringislower(string text)
{
    for (int x = 0; x < int(text.size()); x++)
    {
        if (!islower(text[x]) && isalpha(text[x])) return false;
    }
    return true;
}

bool stringisupper(string text)
{
    for (int x = 0; x < int(text.size()); x++)
    {
        if (!isupper(text[x]) && isalpha(text[x])) return false;
    }
    return true;
}

bool stringisnum(string text)
{
     if (text.compare(0,1,"-") == 0) text.erase(0,1);
     for (int x = 0; x < int(text.size()); x++)
     {
         if (!isdigit(text[x])) return false;
     }
     return true;
}

bool stringisnum(string text, int lo)
{
     int num, neg;
     if (text.compare(0,1,"-") == 0) { text.erase(0,1); neg = 1; }
     if (!stringisnum(text)) return false;
     #ifdef STR_TOK_SS
     stringstream(text) >> num;
     #else
     num = str2int(text);
     #endif
     if (neg == 1) num = num*-1;
     if (num >= lo) return true;
     return false;
}
bool stringisnum(string text, int lo, int hi)
{
     int num, neg;
     if (text.compare(0,1,"-") == 0) { text.erase(0,1); neg = 1; }
     if (!stringisnum(text)) return false;
     #ifdef STR_TOK_SS
     stringstream(text) >> num;
     #else
     num = str2int(text);
     #endif
     if (neg == 1) num = num*-1;
     if (lo <= num && num <= hi) return true;
     return false;
}

bool isletter(string text)
{
     if ((int(text.size()) == 1) && (isalpha(text[0]))) return true;
     return false;
}

bool isletter(string text, string subtext)
{
     if ((isletter(text)) && (isin(subtext,text))) return true;
     return false;
}

//Rewrite upper() and lower()
/*
string upper(string text)
{
    string ret, t;
    for (int x = 0; x < int(text.size()); x++)
    {
        if (islower(text[x])) { t = toupper(text[x]); ret.append(t); }
        else { t = text[x]; ret.append(t); }
    }
    return ret;
}
*/
string upper(string text)
{
    for (int i = 0, j = int(text.size()); i < j; i++) text[i] = toupper(text[i]);
    return text;
}

string lower(string text)
{
    for (int i = 0, j = int(text.size()); i < j; i++) text[i] = tolower(text[i]);
    return text;
}

/*
string lower(string text)
{
    string ret, t;
    for (int x = 0; x < int(text.size()); x++)
    {
        if (isupper(text[x])) { t = tolower(text[x]); ret.append(t); }
        else { t = text[x]; ret.append(t); }
    }
    return ret;
}
*/

int findtok(string tokens, string token, int tok, string delim)
{
	int x, a = 0;
	for (x = 1; x <= numtok(tokens,delim); x++)
	{
        if (token.compare(gettok(tokens,x,delim)) == 0) a++;
        if ((a == tok) && (tok > 0)) break;
	}
	if (tok == 0) return a;
	if (a == tok) return x;
	else return 0;
}

string wildtok(string tokens, string token, int tok, string delim)
{
	int a = 0;
	string str, ret = tokens;
	for (int x = 1; x <= numtok(tokens,delim); x++)
	{
        str = gettok(tokens,x,delim);
        if (iswm(str,token)) a++;
        if (a == tok) break;
	}
    if (a == tok) return str;
	else return ret;
}

string delwildtok(string tokens, string token, int tok, string delim)
{
	int a = 0;
	string str, ret;
	for (int x = 1; x <= numtok(tokens,delim); x++)
	{
        	str = gettok(tokens,x,delim);
	        if (iswm(str,token)) a++;
        	if (a == tok)
		{
			ret = deltok(tokens,x,delim);
			break;
		}
	}
	return ret;
}

bool istok(string text, string token, string delim)
{
    if (findtok(lower(text),lower(token),1,delim) > 0) return true;
    else return false;
}

bool istokcs(string text, string token, string delim)
{
    if (findtok(text,token,1,delim) > 0) return true;
    else return false;
}

string addtok(string text, string token, string delim)
{
    if (!istok(text,token,delim))
    {
        if (text.size() >= 1)
	{
		if (text.compare(text.size()-delim.size(),delim.size(),delim) == 0)
			text = text + token;
		else text = text + delim + token;
	}
        else text = token;
    }
    return text;
}

string deltok(string text, int tok, string delim)
{
    string ret;
    int a = numtok(text,delim);
    if ((tok > a) || (tok < a*-1) || (tok == 0)) return text;
    if (tok < 0) tok = a+tok+1;
    for (int x = 1; x <= a; x++) if (x != tok) ret = ret + delim + gettok(text,x,delim);
    if (ret.compare(0,delim.size(),delim) == 0) return ret.erase(0,delim.size());
    else return ret;
}

string instok(string text, string token, int tok, string delim)
{
    string ret;
	if (text.compare(0,delim.size(),delim) == 0) text = text.erase(0,delim.size());
    int a = numtok(text,delim);
    if ((tok > a) || (tok < a*-1) || (tok == 0)) return text;
    if (tok < 0) tok = a+tok+1;
    for (int x = 1; x <= a; x++)
    {
        if (x == tok) ret = ret + delim + token + delim + gettok(text,x,delim);
        else ret = ret + delim + gettok(text,x,delim);
    }
    if (ret.compare(0,delim.size(),delim) == 0) return ret.erase(0,delim.size());
    else return ret;
}

string matchtok(string text, string token, int tok, string delim)
{
    int a = numtok(text,delim), y = 0, x;
    if ((tok > a) || (tok == 0)) return "";
    for (x = 1; x <= a; x++)
    {
        if (isin(gettok(text,x,delim),token) == 1) y++;
        if (y == tok) break;
    }
    if (y == tok) return gettok(text,x,delim);
    else return "";
}

string puttok(string text, string token, int tok, string delim)
{
    string ret;
    int a = numtok(text,delim);
    if ((tok > a) || (tok == 0) || (tok < a*-1)) return text;
    if (tok < 0) tok = a+tok+1;
    for (int x = 1; x <= a; x++)
    {
        if (x == tok) ret = ret + delim + token;
        else ret = ret + delim + gettok(text,x,delim);
    }
    if (ret.compare(0,delim.size(),delim) == 0) return ret.erase(0,delim.size());
    else return ret;
}

string remtok(string text, string token, int tok, string delim)
{
    int a = findtok(text,token,0,delim);
    if ((tok > a) || (tok < a*-1) || (a == 0)) return text;
    if (tok < 0) tok = a+tok+1;
    if ((tok > 0) || (a == 1)) return deltok(text,findtok(text,token,tok > 0 ? tok : 1,delim),delim);
    for (int x = 1; x <= a; x++) text = deltok(text,findtok(text,token,1,delim),delim);
    return text;
}

string reptok(string text, string token, string stoken, int tok, string delim)
{
    int a = findtok(text,token,0,delim);
    if ((tok > a) || (tok < a*-1) || (a == 0)) return text;
    if (tok < 0) tok = a+tok+1;
    if ((tok > 0) || (a == 1)) return puttok(text,stoken,findtok(text,token,tok > 0 ? tok : 1,delim),delim);
    for (int x = 1; x <= a; x++) text = puttok(text,stoken,findtok(text,token,1,delim),delim);
    return text;
}

string sorttok(string text, string delim)
{
    string ret, z;
    for (int x = numtok(text,delim), r;x;x--)
    {
        r = 0;
        z = gettok(text,x,delim);
        for (int y = numtok(ret,delim);y;y--) if (z.compare(gettok(ret,y,delim)) <= 0) r = y;
        ret = ((r) ? (instok(ret,z,r,delim)) : (((ret.size() > 0) ? (ret + delim) : ("")) + z));
    }
    return ret;
}

string strreplace(string text, string find, string replace)
{
    int flen = find.size(), rlen = replace.size();
    if (!isin(text,find)) return text;
    for (int x = 0; x < int(text.size()); x++)
    {
        if (text.compare(x,flen,find) == 0)
        {
            text.erase(x,flen);
            text.insert(x,replace);
            x+=rlen-1;
        }
    }
    return text;
}

string appendtok(string text, string token, string delim)
{
	if (text.size() >= 1)
	{
		if (text.compare(text.size()-delim.size(),delim.size(),delim) == 0)
			text = text + token;
		else text = text + delim + token;
	}
	else text = token;
	return text;
}

string randtok(string text, string delim)
{
    if (numtok(text,delim) <= 1) return text;
    return gettok(text,randint(1,numtok(text,delim)),delim);
}

string randomizetok(string text, string delim)
{
    if (numtok(text,delim) <= 1) return text;
    string ret;
    int x;
    while (numtok(text,delim) > 0)
    {
        x = randint(1,numtok(text,delim));
        ret = appendtok(ret,gettok(text,x,delim),delim);
        text = deltok(text,x,delim);
    }
    return ret;
}

string strleft(string text, int n)
{
    if ((n >= int(text.size())) || (n*-1 >= int(text.size())))
        return text;
    string ret;
    if (n < 0) ret.append(text,0,text.size()+n);
    else ret.append(text,0,n);
    return ret;
}

string strright(string text, int n)
{
    if ((n >= int(text.size())) || (n*-1 >= int(text.size())))
        return text;
    string ret;
    if (n < 0) ret.append(text,n*-1,text.size()+n);
    else ret.append(text,text.size()-n,n);
    return ret;
}

string strmid(string text, int start, int len)
{
    string ret;
    if (start > int(text.size())) return ret;
    if (len < 0)
    	len = text.size()-start+len;
    ret.append(text,start,len);
    return ret;
}

string strrep(string text, int rep)
{
    string ret;
    for (int x = 1; x <= rep; x++) ret.append(text);
    return ret;
}

string readini(string loc, string section, string item, int n)
{
    ifstream file (loc.c_str());
    string line, ret;
    int m = 1, i = 0, c = -1;
    if (!file.is_open()) return "";
    while (!file.eof())
    {
        getline(file,line);
        if ((line.size() < 1) || (((int)line[0] == 13) && (line.size() == 1))) continue;
        if (iswm(line,"[*]")) i++;
        if (c == i)
        {
            if (gettok(line,1,"=") == item)
            {
                if (m == n)
                {
                    ret = line.erase(0,isin(line,"=",1)+1);
                    break;
                }
                m++;
            }
        }
        if ((iswm(line,"[*]")) && (strmid(line,1,line.size()-2) == section)) c = i;
    }
    file.close();
    return ret;
}
string readloadedini(fstream &file, string section, string item, int n)
{
       /*****************************************************************************************************************************
       ** string readloadedini(fstream &file, string section, string item, int n = 0);                                             **
       **                                                                                                                          **
       ** fstream &file   - The fstream file to read from.                                                                         **
       ** string section  - The section of the ini to look in (the word inside the square brackets).                               **
       ** string item     - The item inside the section to find.                                                                   **
       ** int n           - This will allow you to have multiple items of the same name in a section. This parameter is optional.  **
       **                                                                                                                          **
       ** Returns a string of the nth matching item in the section of an ini file.                                                 **
       **                                                                                                                          **
       **                                   Based off the $readini identifier in mIRC scripting.                                   **
       **                                                   Written by NIGathan                                                    **
       **                                               Contact: nigathan@justca.me                                                **
       *****************************************************************************************************************************/
    
    section = lower(section);
    item = lower(item);
    string line, ret;
    int m = 0, i = 0, c = -1, t = 0;
    if ((!file.is_open()) || (file.fail())) return "";
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if ((line.size() < 1) || (((int)line[0] == 13) && (line.size() == 1))) continue;
        if (((int)line[line.size()-1] == 13) || ((int)line[line.size()-1] == 10)) line.erase(line.size()-1);
        if ((line.substr(0,1) == "[") && (line.substr(line.size()-1,1) == "]")) i++;
        if (c == i)
        {
            t = line.find('=',0);
            if (lower(line.substr(0,t)) == item)
            {
                if (m == n)
                {
                    ret = line.erase(0,t+1);
                    break;
                }
                m++;
            }
        }
        if ((line[0] == '[') && (line[line.size()-1] == ']') && (lower(line.substr(1,line.size()-2)) == section)) c = i;
    }
    return ret;
}

string readloadedini_legacy(fstream &file, string section, string item, int n)
{
    section = lower(section);
    item = lower(item);
    string line, ret;
    int m = 1, i = 0, c = -1;
    if (!file.is_open()) return "";
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if (iswm(line,"[*]")) i++;
        if (c == i)
        {
            if (lower(gettok(line,1,"=")) == item)
            {
                if (m == n)
                {
                    ret = line.erase(0,isin(line,"=",1)+1);
                    break;
                }
                m++;
            }
        }
        if ((iswm(line,"[*]")) && (lower(strmid(line,1,line.size()-2)) == section)) c = i;
    }
    return ret;
}

/*
int writeloadedini(fstream &file, string section, string item, string info, int n = 1)
{
    string line, ret, newline = item + "=" + info;
    section = lower(section);
    item = lower(item);
    int m = 1, i = 0, c = -1, end = 0;
    if (!file.is_open()) return "";
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if (iswm(line,"[*]")) i++;
        if (c == i)
        {
            if (lower(gettok(line,1,"=")) == item)
            {
                if (m == n)
                {
                    //need something to make it erase this line only, and not overwrite the rest of the file
                    file.seekp(file.tellg() - line.size());
                    file << newline;
                    end = 1;
                    break;
                }
                m++;
            }
        }
        if ((iswm(line,"[*]")) && (lower(strmid(line,1,line.size()-3)) == section)) c = i;
        if ((c > 0) && (i > c)) 
    }
    if (end) return 0;
    
    return ret.erase(ret.size()-1);
}
*/
int writeloadedini(fstream &file, string section, string item, string info, int n)
{
    string line[0], newline = item + "=" + info, tline;
    section = lower(section);
    item = lower(item);
    int m = 1, i = 0, c = -1, x = 0, b = 0;
    if (!file.is_open()) return 1;
    file.seekg(0,ios::beg);
    file.clear();
    for (x = 0;!file.eof();x++)
    {
        getline(file,line[x]);
        if (iswm(line[x],"[*]")) i++;
        if ((c > 0) && (c+1 == i) && (b == 0))
        {
            //return 1;
            tline = line[x];
            line[x] = newline;
            x++;
            line[x] = tline;
            b++;
        }
        if ((c == i) && (b == 0))
        {
            if (lower(gettok(line[x],1,"=")) == item)
            {
                //return 1;
                if (m == n)
                {
                    //return 2;
                    line[x].assign(newline);
                    //return 2;
                    b++;
                    //return 2;
                }
                else m++;
            }
            //return 3;
        }
        if ((iswm(line[x],"[*]")) && (lower(strmid(line[x],1,line[x].size()-3)) == section)) return 3;//c = i;
        return 2;
    }
    return 3;
    x++;
    file.seekp(0);
    for (int y = 0;y < x;y++) file << line[y];
    if (file.fail()) return 1;
    return 0;
}

/*
#ifdef STR_TOK_SS
string ini_info(fstream &file, int N, int n = -1)
{
    if (!file.is_open()) return "";
    string line, sec;
    int m = 0, i = 0;
    stringstream ret;
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if ((m > 0) && (N > 0))
        {
            if (N == m)
            {
                if (n < 0) return sec;
                else i++;
                if ((i == n) && (!iswm(line,"[*]"))) return gettok(line,1,"=");
            }
            else if (m > N)
            {
                if (n > i) return "";
                ret << i;
                return ret.str();
            }
        }
        if (iswm(line,"[*]"))
        {
            if ((N == m) && (i > 0)) i--;
            m++;
            sec = strmid(line,1,line.size()-2);
        }
    }
    if (n == 0)
    {
        ret << i;
        return ret.str();
    }
    if (N == 0)
    {
        ret << m;
        return ret.str();
    }
    return "";
}
#endif
*/

string ini_info(fstream &file, int N, int n)
{
    if (!file.is_open()) return "";
    string line, sec;
    int m = 0, i = 0;
    if ((!file.is_open()) || (file.fail())) return "";
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if ((line.size() < 1) || (((int)line[0] == 13) && (line.size() == 1))) continue;
        if ((m > 0) && (N > 0))
        {
            if (N == m)
            {
                if (n < 0) return sec;
                else i++;
                if ((i == n) && (!iswm(line,"[*]"))) return gettok(line,1,"=");
            }
            else if (m > N) return "";
        }
        if (iswm(line,"[*]"))
        {
            if ((N == m) && (i > 0)) i--;
            m++;
            sec = strmid(line,1,line.size()-2);
        }
    }
    return "";
}

int iini_info(fstream &file, int N, int n)
{
    if (!file.is_open()) return 0;
    string line;
    int m = 0, i = 0;
    if ((!file.is_open()) || (file.fail())) return 0;
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if ((line.size() < 1) || (((int)line[0] == 13) && (line.size() == 1))) continue;
        if ((m > 0) && (N > 0))
        {
            if (N == m)
            {
                if (n < 0) return 0;
                else i++;
            }
            else if (m > N)
            {
                if (n > i) return 0;
                return i;
            }
        }
        if (iswm(line,"[*]"))
        {
            if ((N == m) && (i > 0)) i--;
            m++;
        }
    }
    if (n == 0) return i;
    if (N == 0) return m;
    return 0;
}
/*
string nospace(string text)
{
    while (text.compare(0,1," ")) {
          text.erase(0,1);
          }
    while (text.compare(text.size()-1,1," ")) {
          text.erase(text.size()-1,1);
          }
    return text;
}
*/
string nospace(string text)
{
    bool s = true;
    for (int i = 0;i < int(text.size());i++)
    {
        if (int(text.at(i)) == 32)
        {
            if ((s) || (i+1 == int(text.size())))
            {
                text.erase(i,1);
                i--;
            }
            else s = true;
        }
        else s = false;
    }
    return text;
}

int asc(char s)
{
    return (int)s;
}

string chr(int a)
{
    string shit = shit + char(a);
    //string shits = shit;
    //shits.erase(1,shits.size());
    return shit;
}

/*
int iini_name(fstream &file, string section, string n, int r = 1)
{
    if (!file.is_open()) return 0;
    string line;
    int m = 1, i = 0, t = 1, N, c = -1;
    if (stringisnum(n)) N = str2int(n);
    else N = -1;
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if (file.eof()) break;
        if (line.size() < 1) continue;
        if (iswm(line,"[*]")) i++;
        if (c == i)
        {
            if ((m == N) || ((N < 0) && (gettok(line,1,"=") == n)))
            {
                if (t == r) return m;
                else t++;
            }
            m++;
        }
        if ((iswm(line,"[*]")) && (lower(strmid(line,1,line.size()-3)) == lower(section))) c = i;
    }
    if (N == 0) return m;
    return 0;
}
*/
int iini_name(fstream &file, string section, string n)
{
    if (!file.is_open()) return 0;
    string line;
    int m = 0, i = 0, c = -1;
    if ((!file.is_open()) || (file.fail())) return 0;
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if ((line.size() < 1) || (((int)line[0] == 13) && (line.size() == 1))) continue;
        if (iswm(line,"[*]")) i++;
        if (c == i)
        {
            m++;
            if (n == gettok(line,1,"=")) return m;
        } 
        if ((iswm(line,"[*]")) && (lower(strmid(line,1,line.size()-3)) == lower(section))) c = i;
    }
    return m;
}
string ini_name(fstream &file, string section, int N)
{
    if (!file.is_open()) return "";
    section = lower(section);
    string line;
    int m = 0, i = 0, c = -1;
    if ((!file.is_open()) || (file.fail())) return "";
    file.seekg(0,ios::beg);
    file.clear();
    while (!file.eof())
    {
        getline(file,line);
        if ((line.size() < 1) || (((int)line[0] == 13) && (line.size() == 1))) continue;
        if (iswm(line,"[*]")) i++;
        if (c == i)
        {
            m++;
            if (m == N) return gettok(line,1,"=");
        }
        if ((iswm(line,"[*]")) && (lower(strmid(line,1,line.size()-3)) == section)) c = i;
    }
    return "";
}

string dec2frac(float x)
{
    string s = data2str("%f",x);
    if (int(x) == x) return gettok(s,1,".");
    while (strright(s,1) == "0") s = strleft(s,-1);
    s = gettok(s,2,".");
    double t = pow(10,double(s.size()));
    int l = int(x), den = int(t), num = str2int(s);
    float n, d, z;
    //cout<<l<<":"<<den<<":"<<num<<":"<<s<<":"<<f<<":"<<s.size()<<"\n";
    while (1)
    {
        for (int i = 9;i > 1;i--)
        {
            n = (float(num) / float(i));
            d = (float(den) / float(i));
            //cout<<i<<":"<<n<<":"<<d<<"\n";
            if ((int(n) == n) && (int(d) == d))
            {
                num = int(n);
                den = int(d);
                //cout<<".\n";
            }
        }
        z = (float(den) / float(num));
        if ((int(z) != z) || (z == float(den))) break;
    }
    //cout<<".";
    if (l) num = (num + (l * den));
    //cout<<".\n";
    return data2str("%i/%i",num,den);
}

string chr2str(char *a)
{
    return a;
}
const char *str2chr(string str)
{
     return str.c_str();
}

void inverse(bool &b)
{
    if (!b) b = true;
    else b = false;
}

/*char *regexp (char *string, char *patrn, int *begin, int *end) {     
        int i, w=0, len;                  
        char *word = NULL;
        regex_t rgT;
        regmatch_t match;
        regcomp(&rgT,patrn,REG_EXTENDED);
        if ((regexec(&rgT,string,1,&match,0)) == 0) {
                *begin = (int)match.rm_so;
                *end = (int)match.rm_eo;
                len = *end-*begin;
                //word=malloc(len+1);
                for (i=*begin; i<*end; i++) {
                        word[w] = string[i];
                        w++; }
                word[w]=0;
        }
        regfree(&rgT);
        return word;
}*/

