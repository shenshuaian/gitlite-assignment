#include "../include/Commit.h"
#include "../include/Utils.h"

Commit::Commit(const std::string& msg, long long time)
    : message(msg), timestamp(time) {
}

std::string Commit::serialize() const
{
    std::ostringstream out;
    out<<"tree\n";
    for(auto kv:files)
    {
        out<<kv.first<<" "<<kv.second<<'\n';
    }
    for(auto it:parents)
    {
        out<<"parent"<<" "<<it<<"\n";
    }
    out<<"time"<<" "<<timestamp<<"\n";
    out<<"message"<<" "<<message;
    return out.str();
}

Commit Commit::deserialize(const std::string& text)
{
    Commit c;
    std::istringstream in(text);
    std::string line;
    std::getline(in,line);
    while(std::getline(in,line))
    {
        if(line.compare(0,7,"parent ")==0)
        {
            c.parents.push_back(line.substr(7));
        }
        else if(line.compare(0,5,"time ")==0)
        {
            c.timestamp=std::stoll(line.substr(5));
        }
        else if(line.compare(0,8,"message ")==0)
        {
            c.message=line.substr(8);
            std::ostringstream rest;
            rest<<in.rdbuf();
            std::string tail=rest.str();
            if(!tail.empty()) c.message+="\n"+tail;
            break;
        }
        else
        {
            size_t pos=line.rfind(' ');
            if(pos!=std::string::npos)
            {
                c.files[line.substr(0,pos)]=line.substr(pos+1);
            }
        }
    }
    return c;
}

std::string Commit::id() const
{
    return Utils::sha1(serialize());
}

bool Commit::tracks(const std::string& filename) const
{
    return files.count(filename);
}

std::string Commit::blobOf(const std::string& filename) const
{
    auto it=files.find(filename);
    if(it==files.end())
    return "";
    else
    return it->second;
}
