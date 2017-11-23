// str_tok.h written by NIGathan //Note: Needs a new name...
// Allows functionality of useful mIRC functions in C++ and more.
// Version 1.9d - Sun Oct  2 16:56:48 CDT 2016
// Added readloadedini_legacy() Tue Jun  6 00:46:58 MDT 2017
// Version 1.9e - Sun Oct  8 07:18:24 MDT 2017

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
int numtok(string tokens, string delim);
string gettok(string tokens, int tok, string delim);
int isin(string text, string subtext, int pos = 0);
string strremove(string text, string subtext);
string getqtok(string tokens, int tok, string delim);
bool iswm(string text, string subtext);
bool stringisalpha(string text);
bool stringisalnum(string text);
bool stringislower(string text);
bool stringisupper(string text);
bool stringisnum(string text);
bool stringisnum(string text, int lo);
bool stringisnum(string text, int lo, int hi);
bool isletter(string text);
bool isletter(string text, string subtext);
string upper(string text);
string lower(string text);
int findtok(string tokens, string token, int tok, string delim);
string wildtok(string tokens, string token, int tok, string delim);
string delwildtok(string tokens, string token, int tok, string delim);
bool istok(string text, string token, string delim);
bool istokcs(string text, string token, string delim);
string addtok(string text, string token, string delim);
string deltok(string text, int tok, string delim);
string instok(string text, string token, int tok, string delim);
string matchtok(string text, string token, int tok, string delim);
string puttok(string text, string token, int tok, string delim);
string remtok(string text, string token, int tok, string delim);
string sorttok(string text, string delim);
string reptok(string text, string token, string stoken, int tok, string delim);
string strreplace(string text, string find, string replace);
string appendtok(string text, string token, string delim);
string randtok(string text, string delim);
string randomizetok(string text, string delim);
string strleft(string text, int n);
string strright(string text, int n);
string strmid(string text, int start, int len);
string strrep(string text, int rep);
string readini(string loc, string section, string item, int n = 1);
string readloadedini(fstream &file, string section, string item, int n = 1);
string readloadedini_legacy(fstream &file, string section, string item, int n = 1);
int writeloadedini(fstream &file, string section, string item, string info, int n = 1);
string ini_info(fstream &file, int N, int n = -1);
int iini_info(fstream &file, int N, int n = -1);
string nospace(string text);
int asc(char s);
string chr(int a);
int iini_name(fstream &file, string section, string n = "0");
string ini_name(fstream &file, string section, int N);
string dec2frac(float x);
string chr2str(char *a);
#define data2str(format...) ({ char __str[1000]; sprintf(__str , ##format); chr2str(__str); })
const char *str2chr(string str);
void inverse(bool &b);
//#define chr2str(char *a) ({ return string(a); })
//char *regexp (char *string, char *patrn, int *begin, int *end);

#endif
