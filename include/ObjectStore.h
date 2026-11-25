#ifndef OBJECT_STORE_H
#define OBJECT_STORE_H

#include <string>
#include <vector>

#include "../include/Commit.h"
#include "../include/Repository.h"

class ObjectStore {
public:
    // ---- blob ----
    static std::string saveBlob(const std::vector<unsigned char>& content);
    static bool hasBlob(const std::string& blobId);
    static std::string blobPath(const std::string& blobId);
    static std::vector<unsigned char> getBlob(const std::string& blobId);
    static std::string getBlobAsString(const std::string& blobId);

    // ---- commit ----
    static std::string saveCommit(const Commit& commit);
    static bool hasCommit(const std::string& commitId);
    static std::string commitPath(const std::string& commitId);
    static Commit getCommit(const std::string& commitId);

private:
    static std::string objectPath(const std::string& dir,
                                  const std::string& objectId);
};

#endif // OBJECT_STORE_H
