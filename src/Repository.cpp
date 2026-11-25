#include "../include/Repository.h"
#include "../include/Utils.h"
#include "../include/ObjectStore.h"
#include <dirent.h>  

std::string Repository::getGitliteDir() {
    // TODO: 返回元数据目录名（即 ".gitlite"）
    return ".gitlite";
}
std::string Repository::refsDir()
{
   return Utils::join(getGitliteDir(),"refs"); 
}
std::string Repository::headsDir()
{
    return Utils::join(refsDir(),"heads");
}
std::string Repository::tagsDir()
{
    return Utils::join(refsDir(),"tags");
}
std::string Repository::remotesDir()
{
    return Utils::join(refsDir(),"remotes");
}
std::string Repository::objectsDir()
{
    return Utils::join(getGitliteDir(),"objects");
}
std::string Repository::blobsDir()
{
    return Utils::join(objectsDir(),"blobs");
}
std::string Repository::commitsDir()
{
    return Utils::join(objectsDir(),"commits");
}
std::string Repository::headPath()
{
    return Utils::join(getGitliteDir(),"HEAD");
}
std::string Repository::indexPath()
{
    return Utils::join(getGitliteDir(),"index");
}
std::string Repository::branchPath(const std::string& branch)
{
    return Utils::join(headsDir(),branch);
}
std::string Repository::tagPath(const std::string& tag)
{
    return Utils::join(tagsDir(),tag);
}

bool Repository::exists()
{
    return Utils::isDirectory(getGitliteDir());
}

std::string Repository::readHeadBranch()
{
    if(!Utils::isFile(headPath()))   
    return "";
    else
    {
        std::string text=Utils::readContentsAsString(headPath());
        std::string prefix = "ref: refs/heads/";
        if(text.compare(0,prefix.size(),prefix)!=0)
        return "";
        else
        {
            std::string name=text.substr(prefix.size());
            while(!name.empty()&&(name.back()=='\n'||name.back()=='\r'))
            {
                name.pop_back();
            }
            return name;
        }
    }
}
void Repository::writeHeadBranch(const std::string& branch)
{
    Utils::writeContents(headPath(),"ref: refs/heads/"+branch);
}


bool Repository::branchExists(const std::string& branch)
{
    return Utils::isFile(branchPath(branch));
}

std::string Repository::readBranchHead(const std::string& branch)
{
    if(!branchExists(branch))
    return "";
    else
    {
        std::string id=Utils::readContentsAsString(branchPath(branch));
        while(!id.empty()&&(id.back()=='\n'||id.back()=='\r'))
        {
            id.pop_back();
        }
        return id;
    }
}
void Repository::writeBranchHead(const std::string& branch, const std::string& commitId)
{
    Utils::writeContents(branchPath(branch),commitId);
}
std::string Repository::currentCommitId()
{
    return readBranchHead(readHeadBranch());  
}

std::vector<std::string> Repository::listBranches()
{
    return listRegularFiles(headsDir());
}

std::vector<std::string> Repository::listTags()
{
    return listRegularFiles(tagsDir());
}

std::string Repository::trimNewline(const std::string& text)
{
    std::string r=text;
    while(!r.empty()&&(r.back()=='\r'||r.back()=='\n'))
    r.pop_back();
    return r;
}

std::string Repository::resolverevision(const std::string& rev)
{
    if(Utils::isFile(tagPath(rev)))
    return trimNewline(Utils::readContentsAsString(tagPath(rev)));
    else if(rev.size()==Utils::UID_LENGTH&&Utils::isFile(ObjectStore::commitPath(rev)))
    return rev;
    else
    {
        std::string best;
    if (!rev.empty()) {
        DIR*dir=opendir(commitsDir().c_str());
        if(dir != NULL) {
            struct dirent* sub = NULL;
            while ((sub = readdir(dir))!= NULL) {
                std::string subName(sub->d_name);
                if (subName.size() != 2) continue;         
                std::string subDir = Utils::join(commitsDir(), subName);
                DIR* d2 = opendir(subDir.c_str());
                if (d2 ==NULL) continue;
                struct dirent* f = NULL;
                while ((f=readdir(d2))!= NULL) {
                    std::string fname(f->d_name);
                    if (fname.size()!= Utils::UID_LENGTH-2) continue;
                    std::string full = subName + fname;      
                    if (full.compare(0, rev.size(), rev)==0) {
                        if (best.empty()||full < best) 
                        {  
                            best=full;
                        }
                    }
                }
                closedir(d2);
            }
            closedir(dir);}}
            return best;
    }
}

std::vector<std::string> Repository::listRegularFiles(const std::string& dir)
{
    std::vector<std::string> names;
    DIR *d=opendir(dir.c_str());
    if(d==NULL)
    return names;
    else
    {
        struct dirent* e=NULL;
        while((e = readdir(d)) != NULL)
        {
            std::string name(e->d_name);
            if(name=="."||name=="..")
            continue;
           if(Utils::isFile(Utils::join(dir,name)))
           {
           names.push_back(name);
           }
        }
        closedir(d);
        std::sort(names.begin(), names.end()); 
        return names;
    }
}

std::string Repository::readTag(const std::string& name)
{
    return trimNewline(Utils::readContentsAsString(tagPath(name)));
}

void Repository::writeTag(const std::string& name, const std::string& commitId)
{
    Utils::writeContents(tagPath(name),commitId);
}

std::vector<std::string> Repository::listWorkingFiles()
{
    return listRegularFiles(".");
}
// TODO: 其余方法的实现由你自己补全