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
    typedef std::tuple<std::string, std::string,
                       std::string, std::string> packageInfo;

    typedef std::map<std::string, packageInfo> packageMap;
    // first param name Package you wand intercept, second param path file on what you want intercept
    DataLoad(const std::string &packageName,const std::string &pathPackageIntercept);
    // Get ubuntu repo directory with
    // Release and Package.* File
    bool Load();
    // Package size and hash (in md5sun, sha1, sha256)
	static bool GetFileInfo(const std::string &fileName, packageInfo &packageData,EPackageInfo info = eAllField);


	packageInfo GetPackageInfo();

	void GiveWorkerJob();
    ~DataLoad()                          = default;
    DataLoad(const DataLoad&)            = delete;
    DataLoad& operator=(const DataLoad&) = delete;
private:
    static const char*                  m_util[];
    static const std::string            m_ubuntuArchive;
    static const std::string            m_releaseName;
    static const std::string            m_packages;
    packageInfo                         m_packageInfo;
    std::string                         m_packageName;  // Name Package that need to intercept
    std::string                         m_pathPackageIntercept; // Path to file, on want intercept
    boost::thread_group                 m_threads;
    std::queue<std::string>             m_workQueue;
    boost::mutex                        m_mutex;

    // change Package.gz and Package.bz2
    bool ChangePackageFile(const boost::filesystem::path &packagePath, const std::string &rootPath, packageMap &result);
    // change main Release file
    bool ChangeReleaseFile(const boost::filesystem::path& rootPath, packageMap &result);

    void Worker();
    // set package data: size and hash
    void SetPackageInfo(const packageInfo &packageData);
};

#endif /* DATA_LOAD_H_ */
