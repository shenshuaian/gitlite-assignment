#ifndef IGNORE_MATCHER_H
#define IGNORE_MATCHER_H

#include <string>
#include <vector>

// .gitliteignore 解释器。
// 支持的规则（本项目规范）：
//   每行一条规则、空行无效、行首 # 为注释、行首 ! 反选、行首 / 表示从根目录匹配
//   * 匹配任意长度字符序列、? 匹配任意单个字符、\ 转义紧随其后的字符
//   多条规则匹配同一文件时，以最后一条匹配的规则为准
class IgnoreMatcher {
public:
    // 读取工作目录根下的 .gitliteignore（不存在则得到空规则集）
    static IgnoreMatcher loadFromWorkingDir();

    // filename 是工作目录根下的完整文件名
    bool isIgnored(const std::string& filename) const;

    bool empty() const { return rules.empty(); }

private:
    struct Token {
        char kind;  // 'c' 字面量, '?' 单字符, '*' 任意序列
        char ch;    // kind == 'c' 时的字符
    };

    struct Rule {
        std::vector<Token> tokens;
        bool negate;
    };

    std::vector<Rule> rules;

    // 解析一行规则，返回是否有效（注释/空行/空模式返回 false）
    static bool parseRule(const std::string& line, Rule& rule);
    static bool matchTokens(const std::vector<Token>& tokens, size_t tokenIndex,
                            const std::string& name, size_t nameIndex);
    static bool matchRule(const Rule& rule, const std::string& name);
};

#endif // IGNORE_MATCHER_H
