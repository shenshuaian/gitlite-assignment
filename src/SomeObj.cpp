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
    {Commit commit=ObjectStore::getCommit(id);
     std::cout<<"===\n";
     std::cout<<"commit "<<id<<'\n';
     if(commit.parents.size()==2)
     {
     std::cout <<"Merge: "<< commit.parents[0].substr(0,7)<< " " << commit.parents[1].substr(0,7) <<"\n";
    }
    std::time_t t = static_cast<std::time_t>(commit.timestamp);
    std::tm* tm = std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y %z", tm);
    std::cout << "Date: " << buf << "\n";
    std::cout << commit.message << "\n\n";
    if(commit.parents.empty())
    break;
    else
    id=commit.parents[0];
    }
    
}

void SomeObj::globalLog() {
    // TODO: 从所有本地分支、远程跟踪分支和标签出发，输出全部可达提交并按规范排序

}

void SomeObj::listTags() {
    // TODO: 按字典序打印所有标签及其目标提交 id
}

void SomeObj::tag(const std::string&) {
    // TODO: 为当前提交创建轻量标签
}

void SomeObj::tag(const std::string&, const std::string&) {
    // TODO: 为指定提交或标签所指提交创建轻量标签
}

// ---------------- Subtask 3：status / branch / rm-branch ----------------

void SomeObj::status() {
    // TODO: 显示分支、暂存区、未暂存修改与未跟踪文件
}

void SomeObj::branch(const std::string&) {
    // TODO: 新建指向当前提交的分支，不切换
}

void SomeObj::rmBranch(const std::string&) {
    // TODO: 删除分支指针，不删除其提交
}

// ---------------- Subtask 4：checkout / reset ----------------

void SomeObj::checkoutBranch(const std::string&) {
    // TODO: 切换到指定分支
}

void SomeObj::checkoutFile(const std::string&) {
    // TODO: 用当前提交中的版本覆盖工作目录中的文件
}

void SomeObj::checkoutFileInCommit(const std::string&, const std::string&) {
    // TODO: 用指定 revision 的版本覆盖工作文件，支持标签与 id 缩写
}

void SomeObj::reset(const std::string&) {
    // TODO: 解析 revision，检出指定提交并移动当前分支头
}

// ---------------- Subtask 5：merge ----------------

void SomeObj::merge(const std::string&) {
    // TODO: 将指定分支合并到当前分支
}

// ---------------- Bonus-1：add-remote / rm-remote / push / fetch / pull ----------------

void SomeObj::addRemote(const std::string&, const std::string&) {
    // TODO: 保存远程仓库地址
}

void SomeObj::rmRemote(const std::string&) {
    // TODO: 删除远程仓库信息
}

void SomeObj::push(const std::string&, const std::string&) {
    // TODO: 将当前分支的提交推送到远程分支
}

void SomeObj::fetch(const std::string&, const std::string&) {
    // TODO: 将远程分支的提交与 blob 复制到本地
}

void SomeObj::pull(const std::string&, const std::string&) {
    // TODO: fetch 后将取回的分支合并到当前分支
}

// ---------------- Bonus-2：diff / show ----------------

void SomeObj::diff() {
    // TODO: 比较当前提交与工作目录并输出差异，无差异时不输出
}

void SomeObj::diffWithCommit(const std::string&) {
    // TODO: 解析 revision，比较指定提交与工作目录，无差异时不输出
}

void SomeObj::diffBetween(const std::string&, const std::string&) {
    // TODO: 解析两个 revision 并比较对应提交，无差异时不输出
}

void SomeObj::show() {
    // TODO: 显示当前提交及其相对第一父提交的差异
}

void SomeObj::show(const std::string&) {
    // TODO: 显示指定提交或标签及其相对第一父提交的差异
}