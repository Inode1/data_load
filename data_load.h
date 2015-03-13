/*
 * data_load.h
 *
 */

#ifndef DATA_LOAD_H_
#define DATA_LOAD_H_

#include <string>
#include <tuple>
#include <map>

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
    typedef std::map<boost::filesystem::path, packageInfo> packageMap;
    DataLoad(const std::string &packageName);
    // Get ubuntu repo directory with
    // Release and Package.* File
    bool Load();
    // Package size and hash (in md5sun, sha1, sha256)
	static bool GetFileInfo(const std::string &fileName, packageInfo &packageData,EPackageInfo info = eAllField);
	void SetPackageInfo(const packageInfo &packageData);
	packageInfo GetPackageInfo();
	// change Package.gz and Package.bz2
    bool ChangePackageFile(const boost::filesystem::path &packagePath, packageMap &result);
    // change main Release file
    bool ChangeReleaseFile(const packageMap &result);

	//void MakeReplacement();
    ~DataLoad()   = default;
private:
    static const char* util[];
    static const std::string       m_ubuntuArchive;
    packageInfo                    m_packageInfo;
    std::string                    m_packageName;
};

#endif /* DATA_LOAD_H_ */
