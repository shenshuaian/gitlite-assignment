#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>

// 抽象仓库的持久化状态：路径、HEAD 与分支引用。
// 注意：main.cpp 的 checkCWD 依赖 getGitliteDir()，请保留该方法。
class Repository {
public:
    // 元数据目录名，即 ".gitlite"
    static std::string getGitliteDir();

    // ---- 路径 ----
    static std::string refsDir();
    static std::string headsDir();
    static std::string tagsDir();
    static std::string remotesDir();
    static std::string objectsDir();
    static std::string blobsDir();
    static std::string commitsDir();
    static std::string headPath();
    static std::string indexPath();

    // 分支/标签/远程分支的引用文件路径
    // branch 允许包含 '/'（如 "origin/master"），写入时会自动建出中间目录
    static std::string branchPath(const std::string& branch);
    static std::string tagPath(const std::string& tag);
    static std::string remoteBranchPath(const std::string& remote,
                                        const std::string& branch);

    // ---- 仓库状态 ----
    static bool exists();

    // ---- HEAD ----
    // HEAD 文件内容形如 "ref: refs/heads/master"
    static std::string readHeadBranch();
    static void writeHeadBranch(const std::string& branch);

    // ---- 分支引用 ----
    static bool branchExists(const std::string& branch);
    static std::string readBranchHead(const std::string& branch);
    static void writeBranchHead(const std::string& branch, const std::string& commitId);
    static std::vector<std::string> listBranches();

    // 当前分支的头提交 ID
    static std::string currentCommitId();

    static std::string resolverevision(const std::string& rev);

private:
    // 列出目录下的普通文件名（用 stat 判断，避免依赖 dirent 的 d_type）
    static std::vector<std::string> listRegularFiles(const std::string& dir);
    static std::string trimNewline(const std::string& text);
   
};

#endif // REPOSITORY_H
