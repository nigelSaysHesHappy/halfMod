#include <iostream>
#include <string>
#include <regex>
#include "halfmod.h"
using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		cout<<"Usage: "<<argv[0]<<" path/to/plugin.so"<<endl;
		return 1;
	}
	hmGlobal global;
	recallGlobal(&global);
	hmHandle test(argv[1],&global);
	if (test.isLoaded())
	{
		hmInfo info = test.getInfo();
		cout<<"Plugin information:"<<endl<<"Name: "<<info.name<<endl<<"Author: "<<info.author<<endl<<"Description: "<<info.description<<endl<<"Version: "<<info.version<<endl<<"URL: "<<info.url<<endl;
	}
	else
	{
		cout<<endl<<"Unable to load plugin: \""<<argv[1]<<"\". Be sure to include the full path."<<endl;
		return 1;
	}
	return 0;
}

