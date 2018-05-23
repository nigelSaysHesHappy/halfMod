#ifndef _NBTMAP_
#define _NBTMAP_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class NBTCompound
{
    public:
        NBTCompound();
        NBTCompound(std::string nbt_data);
        std::string& operator[] (const std::string &key);
        friend std::ostream& operator<< (std::ostream& stream, NBTCompound &comp);
        void parse(std::string nbt_data = "");
        const std::string get(const std::string &key = "");
        void set(const std::string &key, std::string value);
        std::unordered_map<std::string,std::string>::iterator begin();
        std::unordered_map<std::string,std::string>::iterator end();
        std::unordered_map<std::string,std::string>::const_iterator cbegin();
        std::unordered_map<std::string,std::string>::const_iterator cend();
        size_t size();
        size_t erase(const std::string &key);
    private:
        std::string data;
        std::unordered_map<std::string,std::string> nbt;
};

class NBTList
{
    public:
        NBTList();
        NBTList(std::string nbt_data);
        std::string& operator[] (size_t n);
        friend std::ostream& operator<< (std::ostream& stream, NBTList &list);
        void parse(std::string nbt_data = "");
        const std::string get(int key);
        std::string getList();
        std::vector<std::string>::iterator begin();
        std::vector<std::string>::iterator end();
        std::vector<std::string>::reverse_iterator rbegin();
        std::vector<std::string>::reverse_iterator rend();
        std::vector<std::string>::const_iterator cbegin();
        std::vector<std::string>::const_iterator cend();
        std::vector<std::string>::const_reverse_iterator crbegin();
        std::vector<std::string>::const_reverse_iterator crend();
        size_t size();
        size_t erase(size_t pos);
        size_t erase(size_t start, size_t len);
        std::vector<std::string>::iterator erase(std::vector<std::string>::iterator pos);
        std::vector<std::string>::iterator erase(std::vector<std::string>::iterator first, std::vector<std::string>::iterator last);
        void push_back(std::string item);
        void clear();
    private:
        std::string data;
        std::vector<std::string> nbt;
};

#endif

