#include "../include/ObjectStore.h"
#include "../include/Repository.h"
#include "../include/Utils.h"

std::string ObjectStore::objectPath(const std::string& dir,const std::string& objectId)
{
    std::string prefix=objectId.substr(0,2);
    std::string suffix=objectId.substr(2);
    return Utils::join(Utils::join(dir,prefix),suffix);
}

std::string ObjectStore::commitPath(const std::string& commitId)
{
    return objectPath(Repository::commitsDir(),commitId);
}

bool ObjectStore::hasCommit(const std::string& commitId)
{
    return Utils::isFile(commitPath(commitId));
}

std::string ObjectStore::saveCommit(const Commit& commit)
{
    std::string text=commit.serialize();
    std::string Id=Utils::sha1(text);
    if(!hasCommit(Id))
    {
        Utils::writeContents(commitPath(Id),text);
    }
    return Id;
}

Commit ObjectStore::getCommit(const std::string& commitId)
{
    return Commit::deserialize(Utils::readContentsAsString(commitPath(commitId)));
}

std::string ObjectStore::blobPath(const std::string& blobId)
{
    return objectPath(Repository::blobsDir(),blobId);
}


std::string ObjectStore::saveBlob(const std::vector<unsigned char>& content)
{
    std::string blobId=Utils::sha1(content);
    if(!hasBlob(blobId))
    {
        Utils::writeContents(blobPath(blobId),content);
    }
    return blobId;
}

bool ObjectStore::hasBlob(const std::string& blobId)
{
    return Utils::isFile(blobPath(blobId));
}

std::vector<unsigned char> ObjectStore::getBlob(const std::string& blobId)
{
    return Utils::readContents(blobPath(blobId));
}


std::string ObjectStore::getBlobAsString(const std::string& blobId)
{
    return Utils::readContentsAsString(blobPath(blobId));
}
