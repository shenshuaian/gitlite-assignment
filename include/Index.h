#ifndef INDEX_H
#define INDEX_H

#include <map>
#include <set>
#include <string>

// 暂存区：记录"待添加"与"待删除"两类改动。
// 持久化格式（.gitlite/index，纯文本，便于调试）：
//   A <文件名> <blobId>
//   D <文件名>
class Index {
public:
    // 文件名 -> blob id
    std::map<std::string, std::string> stagedAdd;
    // 待删除的文件名
    std::set<std::string> stagedRemove;

    Index() {}

    static Index load();
    void save() const;

    bool empty() const;
    bool isStagedAdd(const std::string& filename) const;
    bool isStagedRemove(const std::string& filename) const;

    // 清空暂存区（提交后、init 时使用）
    void clear();
};

#endif // INDEX_H
