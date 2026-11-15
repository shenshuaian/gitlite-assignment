#ifndef COMMIT_H
#define COMMIT_H

#include <map>
#include <string>
#include <vector>

// 抽象一次提交：消息、时间戳、父提交、文件快照。
// 序列化格式（固定顺序，保证同一快照得到同一 ID）：
//   tree
//   <文件名> <blobId>      （按文件名升序，std::map 保证）
//   parent <提交Id>        （0~2 行）
//   time <秒级时间戳>
//   message <提交信息>     （最后一个字段，可含空格）
class Commit {
public:
    std::string message;
    long long timestamp = 0;
    std::vector<std::string> parents;
    std::map<std::string, std::string> files;

    Commit() {}
    Commit(const std::string& msg, long long time);

    // 序列化 / 反序列化
    std::string serialize() const;
    static Commit deserialize(const std::string& text);

    // 提交 ID = 序列化文本的 SHA-1
    std::string id() const;

    // 该提交是否跟踪某个文件
    bool tracks(const std::string& filename) const;

    // 该提交中某文件的 blob id（不跟踪时返回空串）
    std::string blobOf(const std::string& filename) const;
};

#endif // COMMIT_H
