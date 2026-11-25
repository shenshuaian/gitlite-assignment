#include "../include/Index.h"
#include "../include/Repository.h"
#include "../include/Utils.h"

#include <sstream>
#include <string>

void Index::save() const
{
    std::ostringstream out;
    for(auto x:stagedAdd)
    {
        out<<"A "<<x.first<<" "<<x.second<<"\n";
    }

    for(auto x:stagedRemove)
    {
        out<<"D "<<x<<"\n";
    }
    Utils::writeContents(Repository::indexPath(), out.str());
}

Index Index::load()
{
    Index index;
    if(!Utils::isFile(Repository::indexPath()))
    {
        return index;
    }

    std::string text=Utils::readContentsAsString(Repository::indexPath());
    std::istringstream in(text);
    std::string line;
    while(std::getline(in,line))
    {
        if(!line.empty()&&line.back()=='\r')
        {
            line.pop_back();
        }
        if(line.size()<2)
        {
            continue;
        }
        if(line[0]=='A'&&line[1]==' ')
        {
            std::string rest=line.substr(2);
            size_t pos=rest.rfind(' ');
            if(pos!=std::string::npos)
            {
                index.stagedAdd[rest.substr(0,pos)]=rest.substr(pos+1);
            }
        }
        else if(line[0]=='D'&&line[1]==' ')
        {
            index.stagedRemove.insert(line.substr(2));
        }
    }
    return index;
}

bool Index::empty() const
{
    return stagedAdd.empty()&&stagedRemove.empty();
}

bool Index::isStagedAdd(const std::string& filename) const
{
    return stagedAdd.count(filename);
}

bool Index::isStagedRemove(const std::string& filename) const
{
    return stagedRemove.count(filename);
}

void Index::clear()
{
    stagedAdd.clear();
    stagedRemove.clear();
}
