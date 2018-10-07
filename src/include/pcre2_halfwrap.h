#ifndef PCRE2_HALFWRAP
#define PCRE2_HALFWRAP

#include <string>
#include <cstring>
#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace pcre2w
{
    class regex
    {
        public:
            regex(const char *re);
            regex(const unsigned char *re);
            regex(const std::string &re);
            regex(const regex &other);
            regex() {}
            ~regex();
            
            pcre2_code *code = nullptr;
            
            int set(const char *re);
            int set(const unsigned char *re);
            int set(const std::string &re);
            
            regex& operator= (const char *re);
            regex& operator= (const unsigned char *re);
            regex& operator= (std::string re);
            regex& operator= (const regex &other);
            
        private:
            int compile(PCRE2_SPTR re);
    };
    
    class smatch_data
    {
        private:
            std::string s;
            size_t pos;
        public:
            smatch_data() {}
            smatch_data(const std::string &st, size_t p);
            std::string str();
            size_t position();
            size_t length();
            void clear();
            std::string operator() ();
    };
            
    class smatch
    {
        public:
            smatch() {}
            smatch_data prefix, suffix;
            std::vector<smatch_data> capture;
            std::vector<smatch_data>::iterator begin();
            std::vector<smatch_data>::iterator end();
            std::vector<smatch_data>::reverse_iterator rbegin();
            std::vector<smatch_data>::reverse_iterator rend();
            std::vector<smatch_data>::const_iterator cbegin();
            std::vector<smatch_data>::const_iterator cend();
            std::vector<smatch_data>::const_reverse_iterator crbegin();
            std::vector<smatch_data>::const_reverse_iterator crend();
            void populate(PCRE2_SPTR subject, pcre2_match_data *ml, int rc);
            smatch_data& operator[] (size_t n);
            size_t size();
            void clear();
    };
    
    int regex_search(const unsigned char *subject, smatch &results, const regex &re, bool with = true);
    int regex_search(const std::string &subject, smatch &results, const regex &re);
    int regex_search(const char *subject, smatch &results, const regex &re);
    int regex_search(const unsigned char *subject, const regex &re);
    int regex_search(const std::string &subject, const regex &re);
    int regex_search(const char *subject, const regex &re);
    int regex_match(const unsigned char *subject, smatch &results, const regex &re);
    int regex_match(const std::string &subject, smatch &results, const regex &re);
    int regex_match(const char *subject, smatch &results, const regex &re);
    int regex_match(const unsigned char *subject, const regex &re);
    int regex_match(const std::string &subject, const regex &re);
    int regex_match(const char *subject, const regex &re);
}

#endif

