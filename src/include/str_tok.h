// str_tok.h written by NIGathan //Note: Needs a new name...
// Allows functionality of useful mIRC functions in C++ and more.
// Version 1.9d - Sun Oct  2 16:56:48 CDT 2016
// Added readloadedini_legacy() Tue Jun  6 00:46:58 MDT 2017
// Version 1.9e - Sun Oct  8 07:18:24 MDT 2017
// Passed strings as ref where applicable
// Version 1.9f - Sun May 27 23:57:29 MDT 2018

#ifndef str_tok
#define str_tok

#define XRGB(r,g,b)		((r) | ((g)<<8) | ((b)<<16) | (0xff<<24))
#define XRGBA(r,g,b,a)	((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))

using namespace std;

int randint(int lo, int hi);
int str2int(string str);
float str2float(string str);
string int2str(int n);
int len(string text);
int numtok(const string &tokens, const string &delim);
string gettok(const string &tokens, int tok, const string &delim);
int isin(const string &text, const string &subtext, int pos = 0);
string strremove(string text, const string &subtext);
int numqtok(const string &tokens, const string &delim);
string getqtok(const string &tokens, int tok, const string &delim);
bool iswm(const string &text, const string &subtext);
bool stringisalpha(const string &text);
bool stringisalnum(const string &text);
bool stringislower(const string &text);
bool stringisupper(const string &text);
bool stringisnum(string text);
bool stringisnum(string text, int lo);
bool stringisnum(string text, int lo, int hi);
bool isletter(const string &text);
bool isletter(const string &text, const string &subtext);
string upper(string text);
string lower(string text);
int findtok(const string &tokens, const string &token, int tok, const string &delim);
string wildtok(const string &tokens, const string &token, int tok, const string &delim);
string delwildtok(const string &tokens, const string &token, int tok, const string &delim);
bool istok(const string &text, const string &token, const string &delim);
bool istokcs(const string &text, const string &token, const string &delim);
string addtok(string text, const string &token, const string &delim);
string deltok(const string &text, int tok, const string &delim);
string instok(string text, const string &token, int tok, const string &delim);
string matchtok(const string &text, const string &token, int tok, const string &delim);
string puttok(const string &text, const string &token, int tok, const string &delim);
string remtok(string text, const string &token, int tok, const string &delim);
string reptok(string text, const string &token, const string &stoken, int tok, const string &delim);
string sorttok(const string &text, const string &delim);
string strreplace(string text, const string &find, const string &replace);
string appendtok(string text, const string &token, const string &delim);
string randtok(const string &text, const string &delim);
string randomizetok(string text, const string &delim);
string strleft(const string &text, int n);
string strright(const string &text, int n);
string strmid(const string &text, int start, int len);
string strrep(const string &text, int rep);
string readini(const string &loc, const string &section, const string &item, int n = 1);
string readloadedini(fstream &file, string section, string item, int n = 1);
string readloadedini_legacy(fstream &file, string section, string item, int n = 1);
int writeloadedini(fstream &file, string section, string item, string info, int n = 1);
string ini_info(fstream &file, int N, int n = -1);
int iini_info(fstream &file, int N, int n = -1);
string nospace(string text);
int asc(char s);
string chr(int a);
int iini_name(fstream &file, const string &section, const string &n = "0");
string ini_name(fstream &file, string section, int N);
string dec2frac(float x);
string chr2str(char *a);
#define data2str(format...) ({ char __str[1000]; sprintf(__str , ##format); chr2str(__str); })
const char *str2chr(const string &str);
void inverse(bool &b);
//#define chr2str(char *a) ({ return string(a); })
//char *regexp (char *string, char *patrn, int *begin, int *end);

#endif
