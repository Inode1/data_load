/*
 * data_load.h
 *
 */

#ifndef DATA_LOAD_H_
#define DATA_LOAD_H_

#include <string>
#include <tuple>
#include <queue>
#include <map>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>

class DataLoad {
public:
    enum EPackageInfo
    {
        eMD5Sum,
        eSHA1Sum,
        eSHA256Sum,
        eSize,
        eAllField
    };

    enum EArchiveOperataion
    {
        eCompress,
        eDecompress
    };

    typedef std::tuple<std::string, std::string,
                       std::string, std::string> packageInfo;
    typedef std::pair<std::string, packageInfo>  fullPackageData;
    typedef std::map<std::string, packageInfo> packageMap;
    // first param name Package you wand intercept, second param path 
    // file on what you want intercept
    DataLoad(const std::map<std::string, fullPackageData> &interceptData);
    // Get ubuntu repo directory with
    // Release and Package.* File
    bool Load();
    // Package size and hash (in md5sun, sha1, sha256)
    static bool GetFileInfo(const std::string &fileName, 
                            packageInfo &packageData,
                            EPackageInfo info = eAllField);

    void GiveWorkerJob();
    ~DataLoad()                          = default;
    DataLoad(const DataLoad&)            = delete;
    DataLoad& operator=(const DataLoad&) = delete;
private:
    static const char*                  m_util[];
    static const std::string            m_ubuntuArchive;
    static const std::string            m_releaseName;
    static const std::string            m_packages;
    // First name Package that need to intercept, 
    // second Path to file, on want intercept 
    std::map<std::string,
             fullPackageData>           m_interceptData;
    // map with archive type, compress and decompress command
    std::map<std::string,
             std::pair<std::string, 
                       std::string>>    m_archivesType;
    boost::thread_group                 m_threads;
    std::queue<std::string>             m_workQueue;
    boost::mutex                        m_mutex;

    void RestoreArchive(const std::string& filePath, const std::string &rootPath,
                        const std::vector<std::string> &extensions,
                        packageMap &result);
    // change Package.gz and Package.bz2
    bool ChangePackageFile(const boost::filesystem::path &packagePath,
                           const std::string &rootPath, packageMap &result);
    // change main Release file
    bool ChangeReleaseFile(const boost::filesystem::path& rootPath,
                           packageMap &result);

    void Worker();
};

#endif /* DATA_LOAD_H_ */
