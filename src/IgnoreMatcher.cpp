#include "../include/IgnoreMatcher.h"
#include "../include/Utils.h"       

#include <sstream>                   
#include <string>
IgnoreMatcher IgnoreMatcher::loadFromWorkingDir()
{
IgnoreMatcher matcher;
const std::string path = ".gitliteignore";
if(!Utils::isFile(path))
{
    return matcher;
}
else
{
    std::string text=Utils::readContentsAsString(path);
    std::istringstream in(text);
    std::string line;
    while(std::getline(in,line))
    {
    if(!line.empty()&&line.back()=='\r')
    {
        line.pop_back();
    }
    Rule rule;
    if(parseRule(line,rule))
    {
        matcher.rules.push_back(rule);
    }
    }
    return matcher;
}
}

bool IgnoreMatcher::parseRule(const std::string& line, Rule& rule)
{
    rule.tokens.clear();
    rule.negate=false;
    if(line.empty())
    return false;
    if(line[0]=='#')
    return false;
    size_t i=0;
    if(line[0]=='!')
    {
        rule.negate=true;
        i=1;
    }
    if(i<line.size()&&line[i]=='/')
    {++i;}
    while(i<line.size())
    {
        char ch=line[i];
        Token token;
        if(ch=='\\')
        {
            token.kind='c';
            if(i+1<line.size())
            {
                token.ch=line[i+1];
                i+=2;
            }
            else
            {token.ch='\\';
            i+=1;}
        }
        else if(ch=='*')
        {
            token.kind='*';
            token.ch='*';
            i++;
        }
        else if(ch=='?')
        {
            token.kind='?';
            token.ch='?';
            i++;
        }
        else
        {
            token.kind='c';
            token.ch=line[i];
            i++;
        }
        rule.tokens.push_back(token);
    }
    return !rule.tokens.empty();
}

bool IgnoreMatcher::matchTokens(const std::vector<Token>& t, size_t ti,
                                const std::string& s, size_t si)
{
    if(ti==t.size())
    {
        return si==s.size();
    }
    else
    {
        Token token=t[ti];
        if(token.kind=='*')
        {
            for(size_t i=si;i<=s.size();i++)
            {
                if(matchTokens(t,ti+1,s,i))
                return true;
            }
            return false;
        }
        if(si>=s.size())
        return false;
        if(token.kind=='?')
        {
            return matchTokens(t,ti+1,s,si+1);
        }
        if(token.kind=='c')
        {
            if(token.ch==s[si])
            return matchTokens(t,ti+1,s,si+1);
            else
            return false;
        }
        return false;
    }
}

bool IgnoreMatcher::matchRule(const Rule& rule, const std::string& name) 
{
    return matchTokens(rule.tokens,0,name,0);
}

bool IgnoreMatcher::isIgnored(const std::string& filename) const
{
    bool ignored=false;
    for(size_t i=0;i<rules.size();i++)
    {
        if(matchRule(rules[i],filename))
        {
            ignored=!rules[i].negate;
        }
    }
    return ignored;
}

