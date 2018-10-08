#include "pcre2_halfwrap.h"

pcre2w::regex::regex(const char *re) { compile((PCRE2_SPTR)re); }
pcre2w::regex::regex(const unsigned char *re) { compile(re); }
pcre2w::regex::regex(const std::string &re) { compile((PCRE2_SPTR)re.c_str()); }
pcre2w::regex::regex(const regex &other) { code = pcre2_code_copy(other.code); pcre2_jit_compile(code,PCRE2_JIT_COMPLETE); }
pcre2w::regex::~regex() { if (code != nullptr) pcre2_code_free(code); }
int pcre2w::regex::set(const char *re) { return compile((PCRE2_SPTR)re); }
int pcre2w::regex::set(const unsigned char *re) { return compile(re); }
int pcre2w::regex::set(const std::string &re) { return compile((PCRE2_SPTR)re.c_str()); }
pcre2w::regex& pcre2w::regex::operator= (const char *re) { compile((PCRE2_SPTR)re); return *this; }
pcre2w::regex& pcre2w::regex::operator= (const unsigned char *re) { compile(re); return *this; }
pcre2w::regex& pcre2w::regex::operator= (std::string re) { compile((PCRE2_SPTR)re.c_str()); return *this; }
pcre2w::regex& pcre2w::regex::operator= (const regex &other) { code = pcre2_code_copy(other.code); pcre2_jit_compile(code,PCRE2_JIT_COMPLETE); return *this; }
int pcre2w::regex::compile(PCRE2_SPTR re)
{
    if (code != nullptr)
        pcre2_code_free(code);
    int errorcode;
    PCRE2_SIZE erroroffset;
    code = pcre2_compile(re,PCRE2_ZERO_TERMINATED,0,&errorcode,&erroroffset,NULL);
    pcre2_jit_compile(code,PCRE2_JIT_COMPLETE);
    return errorcode;
}
pcre2w::smatch_data::smatch_data(const std::string &st, size_t p) { s = st; pos = p; }
pcre2w::smatch_data::smatch_data(const smatch_data &other) { s = other.s; pos = other.pos; }
pcre2w::smatch_data& pcre2w::smatch_data::operator= (const smatch_data &other) { s = other.s; pos = other.pos; return *this; }
std::string pcre2w::smatch_data::str() { return s; }
size_t pcre2w::smatch_data::position() { return pos; }
size_t pcre2w::smatch_data::length() { return s.size(); }
void pcre2w::smatch_data::clear() { s.clear(); pos = 0; }
std::string pcre2w::smatch_data::operator() () { return s; }
std::vector<pcre2w::smatch_data>::iterator pcre2w::smatch::begin() { return capture.begin(); }
std::vector<pcre2w::smatch_data>::iterator pcre2w::smatch::end() { return capture.end(); }
std::vector<pcre2w::smatch_data>::reverse_iterator pcre2w::smatch::rbegin() { return capture.rbegin(); }
std::vector<pcre2w::smatch_data>::reverse_iterator pcre2w::smatch::rend() { return capture.rend(); }
std::vector<pcre2w::smatch_data>::const_iterator pcre2w::smatch::cbegin() { return capture.cbegin(); }
std::vector<pcre2w::smatch_data>::const_iterator pcre2w::smatch::cend() { return capture.cend(); }
std::vector<pcre2w::smatch_data>::const_reverse_iterator pcre2w::smatch::crbegin() { return capture.crbegin(); }
std::vector<pcre2w::smatch_data>::const_reverse_iterator pcre2w::smatch::crend() { return capture.crend(); }
pcre2w::smatch::smatch(const smatch &other)
{
    capture.clear();
    capture.reserve(other.capture.size());
    prefix = other.prefix;
    suffix = other.suffix;
    for (auto it = other.capture.begin(), ite = other.capture.end();it != ite;++it)
        capture.push_back(*it);
}
pcre2w::smatch& pcre2w::smatch::operator= (const smatch &other)
{
    capture.clear();
    capture.reserve(other.capture.size());
    prefix = other.prefix;
    suffix = other.suffix;
    for (auto it = other.capture.begin(), ite = other.capture.end();it != ite;++it)
        capture.push_back(*it);
    return *this;
}
void pcre2w::smatch::populate(PCRE2_SPTR subject, pcre2_match_data *ml, int rc) 
{
    clear();
    if (rc > 0)
    {
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(ml);
        size_t offset = ovector[0];
        std::string sub = (char *)subject;
        if (offset)
            prefix = { sub.substr(0,offset), offset };
        size_t start;
        size_t length;
        for (int i = 0;i < rc;i++)
        {
            start = ovector[2*i];
            length = ovector[2*i+1] - ovector[2*i];
            capture.push_back({sub.substr(start,length),start});
            //printf("%2d: %.*s\n", i, (int)substring_length, (char *)substring_start);
        }
        if (start+length < sub.size())
            suffix = { sub.substr(start+length), start+length };
    }
}
pcre2w::smatch_data& pcre2w::smatch::operator[] (size_t n) { return capture.at(n); }
size_t pcre2w::smatch::size() { return capture.size(); }
void pcre2w::smatch::clear() { prefix.clear(); suffix.clear(); capture.clear(); }

int pcre2w::regex_search(const unsigned char *subject, pcre2w::smatch &results, const pcre2w::regex &re, bool with)
{
    pcre2_match_data *ml = pcre2_match_data_create_from_pattern(re.code, NULL);
    int rc = pcre2_jit_match(re.code,subject,strlen((char *)subject),0,0,ml,NULL);
    if (with)
        results.populate(subject,ml,rc);
    if (rc < 0)
        rc = 0;
    pcre2_match_data_free(ml);
    return rc;
}
int pcre2w::regex_search(const std::string &subject, pcre2w::smatch &results, const pcre2w::regex &re)
{
    return pcre2w::regex_search((const unsigned char*)subject.c_str(),results,re);
}
int pcre2w::regex_search(const char *subject, pcre2w::smatch &results, const pcre2w::regex &re)
{
    return pcre2w::regex_search((const unsigned char*)subject,results,re);
}

int pcre2w::regex_search(const unsigned char *subject, const pcre2w::regex &re)
{
    pcre2w::smatch results;
    return pcre2w::regex_search(subject,results,re,false);
}
int pcre2w::regex_search(const std::string &subject, const pcre2w::regex &re)
{
    return pcre2w::regex_search((const unsigned char*)subject.c_str(),re);
}
int pcre2w::regex_search(const char *subject, const pcre2w::regex &re)
{
    return pcre2w::regex_search((const unsigned char*)subject,re);
}

int pcre2w::regex_match(const unsigned char *subject, pcre2w::smatch &results, const pcre2w::regex &re)
{
    int rc = pcre2w::regex_search(subject,results,re);
    if (results.prefix.length()+results.suffix.length() > 0)
    {
        rc = 0;
        results.clear();
    }
    return rc;
}
int pcre2w::regex_match(const std::string &subject, pcre2w::smatch &results, const pcre2w::regex &re)
{
    return pcre2w::regex_match((const unsigned char*)subject.c_str(),results,re);
}
int pcre2w::regex_match(const char *subject, pcre2w::smatch &results, const pcre2w::regex &re)
{
    return pcre2w::regex_match((const unsigned char*)subject,results,re);
}

int pcre2w::regex_match(const unsigned char *subject, const pcre2w::regex &re)
{
    pcre2w::smatch results;
    return pcre2w::regex_match(subject,results,re);
}
int pcre2w::regex_match(const std::string &subject, const pcre2w::regex &re)
{
    return pcre2w::regex_match((const unsigned char*)subject.c_str(),re);
}
int pcre2w::regex_match(const char *subject, const pcre2w::regex &re)
{
    return pcre2w::regex_match((const unsigned char*)subject,re);
}

