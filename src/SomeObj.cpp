#include "../include/SomeObj.h"

#include "../include/Commit.h"
#include "../include/IgnoreMatcher.h"
#include "../include/Index.h"
#include "../include/ObjectStore.h"
#include "../include/Repository.h"
#include "../include/Utils.h"

#include <iostream>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <queue>
#include<utility>
namespace {
    void printCommitEntry(const std::string& id) {
    Commit commit=ObjectStore::getCommit(id);
     std::cout<<"===\n";
     std::cout<<"commit "<<id<<'\n';
     if(commit.parents.size()==2)
     {
     std::cout <<"Merge: "<< commit.parents[0].substr(0,7)<< " " << commit.parents[1].substr(0,7) <<"\n";
    }
    std::time_t t= static_cast<std::time_t>(commit.timestamp);
    std::tm* tm = std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y %z", tm);
    std::cout << "Date: " << buf << "\n";
    std::cout << commit.message << "\n\n";
    }

    std::set<std::string> collectAncestors(std::string start)
    {
        std::set<std::string> seen;
        std::vector<std::string> stack;
        stack.push_back(start);
        while(!stack.empty())
        {
            std::string id=stack.back();
            stack.pop_back();
            if(seen.count(id))
            continue;
            else
            {
                seen.insert(id);
                Commit c=ObjectStore::getCommit(id);
                for(auto &x:c.parents)
                {
                    if(!seen.count(x))
                    stack.push_back(x);
                }
            }
        }
        return seen;
    }

    std::string findsplitpoint(std::string curhead,std::string givenhead)
    {
        std::set<std::string> curAnc = collectAncestors(curhead);
        std::set<std::string> visited;
        std::queue<std::string> q;
        q.push(givenhead);
        while(!q.empty())
        {
            std::string id=q.front();
            q.pop();
            if(visited.count(id))
            continue;
            else
            {
                visited.insert(id);
                if(curAnc.count(id))
                return id;
                else
                {
                    Commit c=ObjectStore::getCommit(id);
                    for(auto &x:c.parents)
                    q.push(x);
                }
            }
        }
        return "";
    }
    bool samefileIn(Commit a,Commit b,std::string f)
    {
            if(a.tracks(f)!=b.tracks(f))
            return false;
            else
            {
                if(!a.tracks(f))
                return true;
                else
                return a.blobOf(f)==b.blobOf(f);
            }
    }
    std::string conflictContent(Commit&a,Commit&b,std::string&f)
    {
        std::string out="<<<<<<< HEAD\n";
        if(a.tracks(f)) {
            out+=ObjectStore::getBlobAsString(a.blobOf(f));
        }
        if(out[out.size()-1]!='\n') {
            out+="\n";
        }
        out+="=======\n";
        if(b.tracks(f)) {
            out+=ObjectStore::getBlobAsString(b.blobOf(f));
        }
        if (out[out.size()-1]!='\n') {
            out+="\n";
        }
        out+=">>>>>>>\n";
        return out;
    }
    std::vector<std::string> splitLines(const std::string&text)
    {
        std::vector<std::string> lines;
        size_t start=0;
        while(start<text.size())
        {
            size_t nl=text.find('\n',start);
            if(nl==std::string::npos)
            {
                lines.push_back(text.substr(start));
                break;
            }
            lines.push_back(text.substr(start,nl-start));
            start=nl+1;
        }
        return lines;
    }

    struct EditLine
    {
        char prefix;
        std::string text;
    };

    std::vector<EditLine> computeEdits(const std::vector<std::string>&a,const std::vector<std::string>&b)
    {
        size_t m=a.size();
        size_t n=b.size();
        std::vector<std::vector<int>> dp(m+1,std::vector<int>(n+1,0));
        for(size_t i=m;i-->0;)
        {
            for(size_t j=n;j-->0;)
            {
                if(a[i]==b[j])
                dp[i][j]=dp[i+1][j+1]+1;
                else if(dp[i+1][j]>=dp[i][j+1])
                dp[i][j]=dp[i+1][j];
                else
                dp[i][j]=dp[i][j+1];
            }
        }

        std::vector<EditLine> edits;
        size_t i=0;
        size_t j=0;
        while(i<m||j<n)
        {
            EditLine e;
            if(i<m&&j<n&&a[i]==b[j])
            {
                e.prefix=' ';
                e.text=a[i];
                i++;
                j++;
            }
            else if(j==n||(i<m&&dp[i+1][j]>=dp[i][j+1]))
            {
                e.prefix='-';
                e.text=a[i];
                i++;
            }
            else
            {
                e.prefix='+';
                e.text=b[j];
                j++;
            }
            edits.push_back(e);
        }
        return edits;
    }

    struct Hunk
    {
        size_t oldStart;
        size_t oldCount;
        size_t newStart;
        size_t newCount;
        std::vector<EditLine> lines;
        std::vector<bool> marker;
    };

    std::vector<Hunk> buildHunks(const std::vector<EditLine>&edits,bool oldTrailingNewline,bool newTrailingNewline)
    {
        std::vector<Hunk> hunks;
        size_t n=edits.size();
        if(n==0)
        return hunks;

        std::vector<size_t> oldNo(n,0);
        std::vector<size_t> newNo(n,0);
        size_t o=1;
        size_t w=1;
        for(size_t i=0;i<n;i++)
        {
            if(edits[i].prefix!='+')
            {
                oldNo[i]=o;
                o++;
            }
            if(edits[i].prefix!='-')
            {
                newNo[i]=w;
                w++;
            }
        }
        size_t totalOld=o-1;
        size_t totalNew=w-1;

        std::vector<std::pair<size_t,size_t>> blocks;
        size_t idx=0;
        while(idx<n)
        {
            if(edits[idx].prefix==' ')
            {
                idx++;
                continue;
            }
            size_t s=idx;
            size_t e=idx;
            while(e+1<n&&edits[e+1].prefix!=' ')
            e++;
            blocks.push_back(std::make_pair(s,e));
            idx=e+1;
        }
        if(blocks.empty())
        return hunks;

        size_t b=0;
        while(b<blocks.size())
        {
            size_t s=blocks[b].first;
            size_t e=blocks[b].second;
            size_t nb=b+1;
            while(nb<blocks.size())
            {
                size_t gap=blocks[nb].first-e-1;
                if(gap>6)
                break;
                e=blocks[nb].second;
                nb++;
            }

            size_t start=(s>=3)?s-3:0;
            size_t end=(e+3<n)?e+3:n-1;

            Hunk h;
            h.oldCount=0;
            h.newCount=0;
            size_t firstOld=0;
            size_t firstNew=0;
            for(size_t k=start;k<=end;k++)
            {
                if(edits[k].prefix!='+')
                {
                    h.oldCount++;
                    if(firstOld==0)
                    firstOld=oldNo[k];
                }
                if(edits[k].prefix!='-')
                {
                    h.newCount++;
                    if(firstNew==0)
                    firstNew=newNo[k];
                }
            }
            h.oldStart=(h.oldCount==0)?0:firstOld;
            h.newStart=(h.newCount==0)?0:firstNew;

            for(size_t k=start;k<=end;k++)
            {
                h.lines.push_back(edits[k]);
                bool m=false;
                if(edits[k].prefix!='+'&&oldNo[k]==totalOld&&!oldTrailingNewline)
                m=true;
                if(edits[k].prefix!='-'&&newNo[k]==totalNew&&!newTrailingNewline)
                m=true;
                h.marker.push_back(m);
            }

            hunks.push_back(h);
            b=nb;
        }
        return hunks;
    }

    void printHunk(const Hunk&h)
    {
        std::cout<<"@@ -"<<h.oldStart<<","<<h.oldCount<<" +"<<h.newStart<<","<<h.newCount<<" @@\n";
        for(size_t i=0;i<h.lines.size();i++)
        {
            std::cout<<h.lines[i].prefix<<h.lines[i].text<<"\n";
            if(h.marker[i])
            std::cout<<"\\ No newline at end of file\n";
        }
    }

    
    void printFileDiff(const std::string&name,bool oldExists,const std::string&oldText,bool newExists,const std::string&newText)
    {
        if(oldExists==newExists&&oldText==newText)
        return;
        std::vector<std::string> a;
        std::vector<std::string> b;
        if(oldExists)
        a=splitLines(oldText);
        if(newExists)
        b=splitLines(newText);
        bool oldNL=oldExists&&!oldText.empty()&&oldText[oldText.size()-1]=='\n';
        bool newNL=newExists&&!newText.empty()&&newText[newText.size()-1]=='\n';
        std::cout<<"diff --gitlite a/"<<name<<" b/"<<name<<"\n";
        std::cout<<"--- "<<(oldExists?"a/"+name:"/dev/null")<<"\n";
        std::cout<<"+++ "<<(newExists?"b/"+name:"/dev/null")<<"\n";
        std::vector<Hunk> hs=buildHunks(computeEdits(a,b),oldNL,newNL);
        for(size_t i=0;i<hs.size();i++)
        printHunk(hs[i]);
    }

    void printCommitDiff(const Commit&oldC,bool oldExists,const Commit&newC)
    {
        std::set<std::string> names;
        if(oldExists)
        {
            for(auto &kv:oldC.files)
            names.insert(kv.first);
        }
        for(auto &kv:newC.files)
        names.insert(kv.first);
        for(auto &f:names)
        {
            bool oldHas=oldExists&&oldC.tracks(f);
            bool newHas=newC.tracks(f);
            std::string oldText=oldHas?ObjectStore::getBlobAsString(oldC.blobOf(f)):"";
            std::string newText=newHas?ObjectStore::getBlobAsString(newC.blobOf(f)):"";
            printFileDiff(f,oldHas,oldText,newHas,newText);
        }
    }
}
SomeObj::SomeObj() {}

// ---------------- Subtask 1：init / add / commit / rm ----------------

void SomeObj::init() {
    //TODO: 创建 .gitlite 目录、initial commit 与 master 分支
    if(Repository::exists())
    {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
        return;
    }
    else
    {
        Utils::createDirectories(Repository::blobsDir());
        Utils::createDirectories(Repository::headsDir());
        Utils::createDirectories(Repository::commitsDir());
        Commit initial("initial commit",0);
        std::string InitalId=ObjectStore::saveCommit(initial);
        Repository::writeBranchHead("master",InitalId);
        Repository::writeHeadBranch("master");
        Index emptyindex;
        emptyindex.save();

    }

    
}

void SomeObj::add(const std::string&f) {
    // TODO: 将文件的当前内容加入暂存区
    if(!Utils::isFile(f))
    {
    Utils::exitWithMessage("File does not exist.");
    return;
    }
    else
    {
    std::vector<unsigned char> content=Utils::readContents(f);
    std::string blobId=ObjectStore::saveBlob(content);
    Index index=Index::load();
    Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
    if(cur.tracks(f)&&cur.blobOf(f)==blobId)
    {
        index.stagedAdd.erase(f);
        index.stagedRemove.erase(f);
        index.save();
        return;
    }
    if(!cur.tracks(f)&&!index.isStagedAdd(f)&&IgnoreMatcher::loadFromWorkingDir().isIgnored(f))
    {
        return;
    }
    index.stagedRemove.erase(f);
    index.stagedAdd[f]=blobId;
    index.save();
    }
}

void SomeObj::commit(const std::string&message) {
    // TODO: 将暂存区保存为新提交，并推进当前分支
    if(message.empty())
    {Utils::exitWithMessage("Please enter a commit message.");
    return;
    }
    else{
    Index index=Index::load();
    if(index.empty())
    {
    Utils::exitWithMessage("No changes added to the commit.");
    return;
    }
    else
    {
    std::string parentId=Repository::currentCommitId();
    Commit parent=ObjectStore::getCommit(parentId);
    Commit next=parent;
    next.parents.clear();
    next.parents.push_back(parentId);
    next.message=message;
    next.timestamp=static_cast<long long>(std::time(NULL));
    for(auto &kv:index.stagedAdd)
    {
        next.files[kv.first]=kv.second;
    }
    for(auto &name:index.stagedRemove)
    {
        next.files.erase(name);
    }
    std::string newId=ObjectStore::saveCommit(next);
    Repository::writeBranchHead(Repository::readHeadBranch(),newId);
    Index().save();
    return;
    }
}
}

void SomeObj::rm(const std::string&f) {
    // TODO: 将文件取消暂存，或标记为待删除
    Index index=Index::load();
    Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
    if(!cur.tracks(f)&&index.isStagedAdd(f))
    {
        index.stagedAdd.erase(f);
        index.save();
        return;
    }
    else if(cur.tracks(f))
    {
        index.stagedAdd.erase(f);
        index.stagedRemove.insert(f);
        index.save();
        if(Utils::isFile(f))
        {
        Utils::restrictedDelete(f);
        }
        return;
    }
    else
    Utils::exitWithMessage("No reason to remove the file.");
}

// ---------------- Subtask 2：log / global-log / tag ----------------

void SomeObj::log() {
    // TODO: 从当前提交沿第一个父提交打印日志
    std::string id=Repository::currentCommitId();
    while(!id.empty())
    {printCommitEntry(id);
    Commit commit=ObjectStore::getCommit(id);
    if(commit.parents.empty())
    break;
    else
    id=commit.parents[0];
    }
    
}

void SomeObj::globalLog() {
    // TODO: 从所有本地分支、远程跟踪分支和标签出发，输出全部可达提交并按规范排序
    std::vector<std::string> starts;
    std::vector<std::string> branches=Repository::listBranches();
    for(size_t i=0;i<branches.size();i++)
    {
        starts.push_back(Repository::readBranchHead(branches[i]));
    }
    std::vector<std::string> tags=Repository::listTags();
    for(size_t i=0;i<tags.size();i++)
    {
        starts.push_back(Repository::readTag(tags[i]));
    }
    std::set<std::string> seen;
    std::map<std::string, long long> idTime;
    std::vector<std::string> stack = starts;
    while(!stack.empty())
    {
        std::string id=stack.back();
        stack.pop_back();
        if(id.empty()||seen.count(id))
        continue;
        else
        {
            seen.insert(id);
            Commit commit=ObjectStore::getCommit(id);
            idTime[id]=commit.timestamp;
            for (size_t i=0; i<commit.parents.size();++i) {
            stack.push_back(commit.parents[i]);
        }
    }}
    std::vector<std::string> all;
    for(auto & u:idTime)
    {
        all.push_back(u.first);
    }
     std::sort(all.begin(), all.end(),
              [&idTime](const std::string& a, const std::string& b) {
                  if (idTime[a]!=idTime[b]) {
                      return idTime[a]>idTime[b];   
                  }
                  return a < b;                        
              });
    for(auto &x:all)
    printCommitEntry(x);
    return;

}

void SomeObj::listTags() {
    // TODO: 按字典序打印所有标签及其目标提交 id
    std::vector <std::string> tags=Repository::listTags();
    for(auto &x:tags)
    std::cout<<x<<" "<<Repository::readTag(x)<<"\n";
}

void SomeObj::tag(const std::string&n) {
    // TODO: 为当前提交创建轻量标签
     if(Utils::isFile(Repository::tagPath(n)))
     {
        Utils::exitWithMessage("A tag with that name already exists.");
     }
     else
     Repository::writeTag(n,Repository::currentCommitId());
}

void SomeObj::tag(const std::string&n, const std::string&rev) {
    // TODO: 为指定提交或标签所指提交创建轻量标签
    if(Utils::isFile(Repository::tagPath(n)))
    {
        Utils::exitWithMessage("A tag with that name already exists.");
    }
    else
    {
    std::string Id=Repository::resolverevision(rev);
    if(Id.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    else
    Repository::writeTag(n,Id);
    }
   
}

// ---------------- Subtask 3：status / branch / rm-branch ----------------

void SomeObj::status() {
    // TODO: 显示分支、暂存区、未暂存修改与未跟踪文件
    Index index=Index::load();
    std::string curBranch =Repository::readHeadBranch();
    std::cout << "=== Branches ===\n";
    std::vector<std::string> branches=Repository::listBranches();
    for(size_t i =0; i<branches.size(); ++i) {
        if (branches[i]==curBranch) {
            std::cout<<"*";
        }
        std::cout<<branches[i]<<"\n";
    }
    std::cout<< "\n";
    std::cout<< "=== Staged Files ===\n";
    for(auto x:index.stagedAdd)
    {
        std::cout<<x.first<<"\n";
    }
    std::cout<< "\n";
    std::cout<< "=== Removed Files ===\n";
    for(auto&x:index.stagedRemove)
    {
        std::cout<<x<<"\n";
    }
    std::cout<< "\n";
    std::cout<< "=== Modifications Not Staged For Commit ===\n";
    std::map<std::string,std::string>mods;
    Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
    for(auto x:cur.files)
    {
        std::string f=x.first;
        if(index.isStagedRemove(f))
        continue;
        else
        {
            std::string base =index.isStagedAdd(f)?index.stagedAdd[f]:x.second;
            if(!Utils::isFile(f))
            mods[f]="(deleted)";
            else
            {std::string now= Utils::sha1(Utils::readContents(f));
            if(now!=base)
            mods[f]="(modified)";
            }
        }}
    for(auto &x:index.stagedAdd)
    {
            std::string f=x.first;
            if(cur.tracks(f))
            continue;
            else
            {
                if(!Utils::isFile(f))
                mods[f]="(deleted)";
                else
                {std::string now=Utils::sha1(Utils::readContents(f));
                if(now!=x.second)
                mods[f]="(modified)";}
            }
    
    }
    for(auto &x:mods)
    {
        std::cout<<x.first<<" "<<x.second<<"\n";
    }
    std::cout<<"\n";

    std::cout<< "=== Untracked Files ===\n";
    std::vector <std::string> wfiles=Repository::listWorkingFiles();
    IgnoreMatcher ignore=IgnoreMatcher::loadFromWorkingDir();
    for(auto &x:wfiles)
    {
        bool tracked=cur.tracks(x);
        bool stageadd=index.isStagedAdd(x);
        bool stageremove=index.isStagedRemove(x);
        if(stageadd)
        continue;
        else if(tracked&&!stageremove)
        continue;
        else if(tracked&&stageremove)
        {
            std::cout<<x<<"\n";
            continue;
        }
        if(ignore.isIgnored(x))
        continue;

        std::cout<<x<<"\n";
    }
    std::cout<< "\n";
}




void SomeObj::branch(const std::string&f) {
    // TODO: 新建指向当前提交的分支，不切换
    if(Repository::branchExists(f))
    {
        Utils::exitWithMessage("A branch with that name already exists.");
        return;
    }
    else
    Repository::writeBranchHead(f,Repository::currentCommitId());

}

void SomeObj::rmBranch(const std::string&f) {
    // TODO: 删除分支指针，不删除其提交
    if(!Repository::branchExists(f))
    {
        Utils::exitWithMessage("A branch with that name does not exist.");
        return;
    }
    else if(f==Repository::readHeadBranch())
    {Utils::exitWithMessage("Cannot remove the current branch.");
    return;}
    else
    {
        std::remove(Repository::branchPath(f).c_str());
    }

}

// ---------------- Subtask 4：checkout / reset ----------------

void SomeObj::checkoutBranch(const std::string&b) {
    // TODO: 切换到指定分支
   if (!Repository::branchExists(b)) {
        Utils::exitWithMessage("No such branch exists.");
        return;
    }
    else 
    {
    std::string current=Repository::readHeadBranch();
    if(current==b)
    {
        Utils::exitWithMessage("No need to checkout the current branch.");
        return;
    }
    Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
    Index index=Index::load();
    Commit targent=ObjectStore::getCommit(Repository::readBranchHead(b));
    IgnoreMatcher ig = IgnoreMatcher::loadFromWorkingDir();
    for(auto &x:targent.files)
    {
        const std::string f=x.first;
        if(!Utils::isFile(f))
        continue;
        if(cur.tracks(f))
        continue;
        if(index.isStagedAdd(f))
        continue;
        if(ig.isIgnored(f))
        continue;
        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        return;
    }
    for(auto &x:cur.files)
    {
        const std::string f=x.first;
        if(targent.tracks(f))
        continue;
        if(Utils::isFile(f))
        {
        Utils::restrictedDelete(f);
        }
    }
    for(auto &x:targent.files)
    {
        std::vector<unsigned char> blob=ObjectStore::getBlob(x.second);
        Utils::writeContents(x.first,blob);
    }

    Repository::writeHeadBranch(b);
    Index().save();
    }

}

void SomeObj::checkoutFile(const std::string&f) {
    // TODO: 用当前提交中的版本覆盖工作目录中的文件
     Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
    if(!cur.tracks(f))
    {
        Utils::exitWithMessage("File does not exist in that commit.");
        return;
    }
    else
    {
        std::string blobId=cur.blobOf(f);
        std::vector<unsigned char>content=ObjectStore::getBlob(blobId);
        Utils::writeContents(f,content);
    }
}

void SomeObj::checkoutFileInCommit(const std::string&rev , const std::string&f) {
    // TODO: 用指定 revision 的版本覆盖工作文件，支持标签与 id 缩写
    std::string commitid=Repository::resolverevision(rev);
    if(commitid.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    else 
    {
        Commit commit=ObjectStore::getCommit(commitid);
        if(!commit.tracks(f))
        {
            Utils::exitWithMessage("File does not exist in that commit.");
            return;
        }
        else
        {
        std::string blobId=commit.blobOf(f);
        std::vector<unsigned char>content=ObjectStore::getBlob(blobId);
        Utils::writeContents(f,content);
    }

    }
}

void SomeObj::reset(const std::string&rev) {
    // TODO: 解析 revision，检出指定提交并移动当前分支头
    std::string commitid=Repository::resolverevision(rev);
    if(commitid.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    else
    {
        Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
        Commit targent=ObjectStore::getCommit(commitid);
        Index index=Index::load();
        IgnoreMatcher ig = IgnoreMatcher::loadFromWorkingDir();
    for(auto &x:targent.files)
    {
        const std::string f=x.first;
        if(!Utils::isFile(f))
        continue;
        if(cur.tracks(f))
        continue;
        if(index.isStagedAdd(f))
        continue;
        if(ig.isIgnored(f))
        continue;
        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        return;
    }
    for(auto &x:cur.files)
    {
        const std::string f=x.first;
        if(targent.tracks(f))
        continue;
        if(Utils::isFile(f))
        {
        Utils::restrictedDelete(f);
        }
    }
    for(auto &x:targent.files)
    {
        std::vector<unsigned char> blob=ObjectStore::getBlob(x.second);
        Utils::writeContents(x.first,blob);
    }

    Repository::writeBranchHead(Repository::readHeadBranch(), commitid);
    Index().save();
    }

    
}

// ---------------- Subtask 5：merge ----------------

void SomeObj::merge(const std::string&b) {
    // TODO: 将指定分支合并到当前分支
    Index index=Index::load();
    if(!index.empty())
    {
        Utils::exitWithMessage("You have uncommitted changes.");
        return;
    }
    else
    {
        if(!Repository::branchExists(b))
        {
            Utils::exitWithMessage("A branch with that name does not exist.");
            return;
        }
        std::string curb=Repository::readHeadBranch();
        if(b==curb)
        {
            Utils::exitWithMessage("Cannot merge a branch with itself.");
            return;
        }
        std::string curId=Repository::currentCommitId();
        std::string givenid=Repository::readBranchHead(b);
        std::string split=findsplitpoint(curId,givenid);
        if(givenid==split)
        {
            Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
            return;
        }
        if(split==curId)
        {
            Commit cur=ObjectStore::getCommit(curId);
            Commit given=ObjectStore::getCommit(givenid);
            IgnoreMatcher ig=IgnoreMatcher::loadFromWorkingDir();
            Index idx=Index::load();
            for(auto &x:given.files)
            {
                std::string f=x.first;
                if(!Utils::isFile(f))
                continue;
                if(cur.tracks(f))
                continue;
                if(idx.isStagedAdd(f))
                continue;
                if(ig.isIgnored(f))
                continue;

                Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
                return;
            }
            for(auto &x:cur.files)
            {
            std::string f=x.first;
            if (given.tracks(f)) {
                continue;
            }
            if (Utils::isFile(f)) {
                Utils::restrictedDelete(f);
            }
            }
            
            for(auto x:given.files)
            {
                std::vector<unsigned char> content=ObjectStore::getBlob(x.second);
                Utils::writeContents(x.first,content);
            }        
            Repository::writeBranchHead(Repository::readHeadBranch(), givenid);
            Index().save();
            std::cout << "Current branch fast-forwarded.\n";
            return;
    }
            Commit cur=ObjectStore::getCommit(curId);
            Commit given=ObjectStore::getCommit(givenid);
            Index index=Index::load();
            Commit base=ObjectStore::getCommit(split);
            IgnoreMatcher ig=IgnoreMatcher::loadFromWorkingDir();
            std::set<std::string> path;
            for(auto &x:cur.files)
            path.insert(x.first);
            for(auto &x:given.files)
            path.insert(x.first);
            for(auto &x:base.files)
            path.insert(x.first);
            bool hasconflict=false;
            std::map<std::string, std::string>newFiles=cur.files;   
            std::vector<std::string> writeGiven;    
            std::vector<std::string> delPaths;        
            std::vector<std::string> conflictPaths;   

            for(auto &f:path)
            {
                bool curchanged=!samefileIn(base,cur,f);
                bool givenchanged=!samefileIn(base,given,f);
                if(!curchanged&&!givenchanged)
                continue;
                if(!givenchanged&&curchanged)
                continue;
                if(!curchanged&&givenchanged)
                {
                    if(given.tracks(f))
                    {newFiles[f]=given.blobOf(f);
                    writeGiven.push_back(f);}
                    else
                    {
                        newFiles.erase(f);
                        delPaths.push_back(f);
                    }
                    continue;
                }
                if(samefileIn(cur,given,f))
                continue;
                else
                {
                    hasconflict=true;
                    conflictPaths.push_back(f);
                }
                
            }
                std::vector<std::string> touched=writeGiven;
                touched.insert(touched.end(),delPaths.begin(),delPaths.end());
                touched.insert(touched.end(),conflictPaths.begin(),conflictPaths.end());
                for(auto x:touched)
                {
                    if(!Utils::isFile(x))
                    continue;
                    if(ig.isIgnored(x))
                    continue;
                    if(index.isStagedAdd(x))
                    continue;
                    if(cur.tracks(x))
                    continue;
                    else
                    {
                        Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
                        return;
                    }

                }
                for(auto &x:delPaths)
                {
                    if(Utils::isFile(x))
                    Utils::restrictedDelete(x);
                }
                for(auto &x:writeGiven)
                {
                    Utils::writeContents(x,ObjectStore::getBlob(given.blobOf(x)));
                }
                for(auto &x:conflictPaths)
                {
                    std::string text=conflictContent(cur,given,x);
                    std::vector<unsigned char> bytes(text.begin(),text.end());
                    newFiles[x]=ObjectStore::saveBlob(bytes);
                    Utils::writeContents(x,bytes);
                }

            Commit merged=cur;
            merged.files=newFiles;
            merged.parents.clear();
            merged.parents={curId,givenid};
            merged.message ="Merged " +b+" into "+Repository::readHeadBranch() + ".";
            merged.timestamp = static_cast<long long>(std::time(NULL));
            std::string newid=ObjectStore::saveCommit(merged);
            Repository::writeBranchHead(Repository::readHeadBranch(),newid);
            Index().save();
            if(hasconflict)
            std::cout<<"Encountered a merge conflict.\n";
            return;
}
    

}

// ---------------- Bonus-1：add-remote / rm-remote / push / fetch / pull ----------------

void SomeObj::addRemote(const std::string&,const std::string&) {
}

void SomeObj::rmRemote(const std::string&) {
}

void SomeObj::push(const std::string&,const std::string&) {
}

void SomeObj::fetch(const std::string&,const std::string&) {
}

void SomeObj::pull(const std::string&,const std::string&) {
}

// ---------------- Bonus-2：diff / show ----------------

void SomeObj::diff() {
    Commit cur=ObjectStore::getCommit(Repository::currentCommitId());
    for(auto &kv:cur.files)
    {
        std::string oldText=ObjectStore::getBlobAsString(kv.second);
        bool newExists=Utils::isFile(kv.first);
        std::string newText=newExists?Utils::readContentsAsString(kv.first):"";
        printFileDiff(kv.first,true,oldText,newExists,newText);
    }
}

void SomeObj::diffWithCommit(const std::string&rev) {
    std::string id=Repository::resolverevision(rev);
    if(id.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    Commit target=ObjectStore::getCommit(id);
    for(auto &kv:target.files)
    {
        std::string oldText=ObjectStore::getBlobAsString(kv.second);
        bool newExists=Utils::isFile(kv.first);
        std::string newText=newExists?Utils::readContentsAsString(kv.first):"";
        printFileDiff(kv.first,true,oldText,newExists,newText);
    }
}

void SomeObj::diffBetween(const std::string&rev1,const std::string&rev2) {
    std::string id1=Repository::resolverevision(rev1);
    if(id1.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    std::string id2=Repository::resolverevision(rev2);
    if(id2.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    Commit c1=ObjectStore::getCommit(id1);
    Commit c2=ObjectStore::getCommit(id2);
    printCommitDiff(c1,true,c2);
}

void SomeObj::show() {
    std::string id=Repository::currentCommitId();
    Commit c=ObjectStore::getCommit(id);
    printCommitEntry(id);
    if(c.parents.empty())
    return;
    Commit parent=ObjectStore::getCommit(c.parents[0]);
    printCommitDiff(parent,true,c);
}

void SomeObj::show(const std::string&rev) {
    std::string id=Repository::resolverevision(rev);
    if(id.empty())
    {
        Utils::exitWithMessage("No commit with that id exists.");
        return;
    }
    Commit c=ObjectStore::getCommit(id);
    printCommitEntry(id);
    if(c.parents.empty())
    return;
    Commit parent=ObjectStore::getCommit(c.parents[0]);
    printCommitDiff(parent,true,c);
}
