/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <utility>
#include <iomanip>
#include <fstream>

#include <boost/algorithm/string/erase.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/regex.hpp>

#include "data_load.h"

using namespace std;
using namespace boost::filesystem;

const char* DataLoad::m_util[] = { "md5sum ",
                                   "sha1sum ",
                                   "sha256sum "
                                 };
const string DataLoad::m_releaseName = "Release";
const string DataLoad::m_packages    = "Packages";
//ru.archive.ubuntu.com/ubuntu/dists
const string DataLoad::m_ubuntuArchive = "ru.archive.ubuntu.com/ubuntu/dists/";

DataLoad::DataLoad(const std::map<std::string, fullPackageData> &interceptData):
                   m_interceptData(interceptData)
{
    // archives type
    m_archivesType.emplace(std::make_pair(std::string{".bz2"}, 
                            std::make_pair(std::string{"bzip2 -f -k > /dev/null 2>&1 "}, 
                                           std::string{"bunzip2 -f > /dev/null 2>&1 "})));


    m_archivesType.emplace(std::make_pair(std::string{".gz"}, 
                            std::make_pair(std::string{"gzip -f > /dev/null 2>&1 "}, 
                                           std::string{"gunzip -f > /dev/null 2>&1 "})));
    
    m_archivesType.emplace(std::make_pair(std::string{".xz"}, 
                            std::make_pair(std::string{"xz -z -f > /dev/null 2>&1 "}, 
                                           std::string{"xz -d -f > /dev/null 2>&1 "})));
}

bool DataLoad::Load()
{
    string command = string("wget -nv -R index.html -r --no-parent \
                             -A 'Packages.*','Release' > /dev/null 2>&1 ") + 
                     m_ubuntuArchive;
    cout << "Wait while all data will be loaded." << endl;
    if (system(command.c_str()))
    {
        cout << "Something happen. Data isn`t loaded." << endl;
        return false;
    }

    cout << "All data load." << endl;
    return true;
}

bool DataLoad::GetFileInfo(const string &fileName, packageInfo &packageData, 
                           EPackageInfo info)
{
    if (info == eAllField)
    {
        for (int i = 0; i < eAllField; ++i)
        {
            if (!GetFileInfo(fileName, packageData, 
                            static_cast<DataLoad::EPackageInfo>(i)))
            {
                return false;
            }
        }
        return true;
    }
    if (info == eSize)
    {
        path filePath(fileName);

        if (!exists(filePath) && is_directory(filePath))
        {
            cout << "Bad file" << endl;
            return false;
        }
        else
        {
            get<eSize>(packageData) = to_string(file_size(filePath));
            return true;
        }
    }

    string command = string{DataLoad::m_util[info]} + fileName;
    FILE *fp;
    fp = popen(command.c_str(), "r");

    char buffer[100];
    string line;
    while (fgets(buffer, sizeof buffer, fp) != NULL)
    {
        line += buffer;
    }

    pclose(fp);

    if (line.empty())
    {
        return false;
    }
    std::size_t found = line.find(" ");
    if (found == std::string::npos)
    {
        return false;
    }
    line = line.substr(0, found);
    if (info == eMD5Sum)
    {
        get<eMD5Sum>(packageData) = line;
    }
    if (info == eSHA1Sum)
    {
        get<eSHA1Sum>(packageData) = line;
    }
    if (info == eSHA256Sum)
    {
        get<eSHA256Sum>(packageData) = line;
    }
    return true;
}

void DataLoad::RestoreArchive(const string& filePath, const string &rootPath,
                              const std::vector<std::string> &extensions,
                              packageMap &result)
{
    packageInfo info;
    for (const auto& extension: extensions)
    {
        system((get<eCompress>(m_archivesType[extension]) + 
                               filePath).c_str());        
        std::string fullPath{filePath + extension};
        GetFileInfo(fullPath, info);
        result.insert(pair<string, packageInfo>(
                      fullPath.substr(rootPath.length() + 1), info));
    }
} 

bool DataLoad::ChangePackageFile(const boost::filesystem::path &packagePath, 
                                 const string &rootPath, packageMap &result)
{
    // iterate in this directory and find all Packages Archives extension
    std::vector<std::string> extensions;
    std::cout << packagePath.string() << std::endl; 
    for (const auto& directoryFile: directory_iterator(packagePath))
    {
        if (is_regular_file(directoryFile.path()) && directoryFile.path().has_extension())
        {
            std::string extension(directoryFile.path().extension().string());
            if (m_archivesType.find(extension) != m_archivesType.end())
            {
                std::cout << "Ext" << directoryFile.path().string() << std::endl;
                extensions.push_back(std::move(extension));
            }
        }
    }

    // no package files
    if (extensions.empty())
    {
        return true;
    }

    path filePath(packagePath / m_packages);
    std::cout << (get<eDecompress>(m_archivesType[extensions[0]]) + 
                filePath.string() + 
                extensions[0]) << std::endl;
    if (system((get<eDecompress>(m_archivesType[extensions[0]]) + 
                filePath.string() + 
                extensions[0]).c_str()))
    {
        cout << "Modified:" << (filePath.string() + extensions[0]) << " Failed" << endl;
        return false;
    }

    cout << "Unzip:" << (filePath.string() + extensions[0]) << " Finished" << endl;

    if (!exists(filePath))
    {
        cout << absolute(filePath) << " not exist" << endl;
        return false;
    }
    if (boost::filesystem::is_empty(filePath))
    {
        //restore
        RestoreArchive(filePath.string(), rootPath, extensions, result);
        return false;
    }

    std::fstream is(filePath.string());
    string line;
    bool find = false;
    boost::regex e ("^Package: (.*)");
    while(getline(is, line))
    {
        boost::smatch match;

        if (boost::regex_match(line, match, e) && m_interceptData.find(match[1]) 
            != m_interceptData.end())
        {
            string matchResult = match[1];
            boost::regex findString("^Size: .*");
            while(getline(is, line))
            {
                if (boost::regex_match(line, findString))
                {
                    is.seekp(-(line.length() + 1), ios_base::cur);
                    // check then length insert the same

                    string size = string{"Size: "} + 
                               get<eSize>(m_interceptData[matchResult].second);
                    int diff = line.length() - size.length();
                    is << size                                                           
                       << endl;
                    is << "MD5sum: " 
                       << get<eMD5Sum>(m_interceptData[matchResult].second)    
                       << endl;
                    is << "SHA1: "   
                       << get<eSHA1Sum>(m_interceptData[matchResult].second)   
                       << endl;
                    int length;
                    if (diff)
                    {
                        length = is.tellg();
                        getline(is, line);
                        getline(is, line);
                        if (diff > 0)
                        {
                            getline(is, line);
                        }
                        is.seekp(length, ios_base::beg);
                    }
                    is << "SHA256: " 
                       << get<eSHA256Sum>(m_interceptData[matchResult].second) 
                       << endl;
                    if (diff > 0)
                    {
                        is << line << string(diff, '!') << endl;
                    }
                    else if (diff < 0)
                    {
                        is << line.substr(0, line.length() + diff) << endl;
                    }
                    cout << "Package:" << match[1] << " modified in "
                         <<  absolute(packagePath.parent_path())
                         << " directory" << endl;
                    find = true;
                    break;
                }
            }
        }
    }
    if (!find)
    {
        cout << "Packages not find in " 
        <<  absolute(packagePath.parent_path())
        << " directory" << endl;
    }
    RestoreArchive(filePath.string(), rootPath, extensions, result);

    return true;
}

bool DataLoad::ChangeReleaseFile(const boost::filesystem::path& rootPath,
                                 packageMap &result)
{
    path releasePath(rootPath / m_releaseName);
    if (!exists(releasePath) || boost::filesystem::is_empty(releasePath))
    {
        cout << absolute(releasePath) << " not exists." << endl;
        return false;
    }

    std::fstream is(releasePath.string());
    string line;
    // TO-DO:: bad performance.
    cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    cout << absolute(releasePath) << endl;
    string modifiedString;
    int n = 0;
    while(getline(is, line))
    {
        modifiedString = line.substr(line.find_last_of(' ') + 1);
        if (result.find(modifiedString) != result.end())
        {
            is.seekg(-(line.length() + 1), ios_base::cur);
            if ((n / result.size()) == 0)
            {
                is << " " << get<eMD5Sum>(result[modifiedString]) 
                   << setw(17) << get<eSize>(result[modifiedString]) 
                   << " " << modifiedString << endl;
            }
            if ((n / result.size()) == 1)
            {
                is << " " << get<eSHA1Sum>(result[modifiedString]) 
                   << setw(17) << get<eSize>(result[modifiedString]) 
                   << " " << modifiedString << endl;
            }
            if ((n / result.size()) == 2)
            {
                is << " " << get<eSHA256Sum>(result[modifiedString]) 
                   << setw(17) << get<eSize>(result[modifiedString]) 
                   << " " << modifiedString << endl;
            }
            ++n;
        }
    }

    return true;
}

void DataLoad::Worker()
{
    while(1)
    {
        string version;
        {
            boost::lock_guard<boost::mutex> lock(m_mutex);
            if (m_workQueue.empty())
            {
                break;
            }

            version = m_workQueue.front();
            m_workQueue.pop();
    
        }
        path versionPath(version);
        cout << "Thread: " << boost::this_thread::get_id() << " start" << endl;
        if (!exists(versionPath))
        {
            cout << versionPath << " not exists." << endl;
            break;
        }
        boost::regex findPackages("Packages..*");
        packageMap packageHashs;
        for(boost::filesystem::recursive_directory_iterator file(versionPath);
                file != boost::filesystem::recursive_directory_iterator();
                ++file )
        {
            if (boost::regex_match(file->path().filename().string(), 
                                   findPackages))
            {
                cout << file->path().string()
                        .substr(versionPath.string().length() + 1) << endl;
                cout << file->path().root_path() << endl;
                cout << file->path().relative_path() << endl;
                ChangePackageFile(file->path().parent_path(), 
                                  versionPath.string(), packageHashs);
                file.pop();
            }
        }
        if (packageHashs.empty())
        {
            cout << "Nothing change" << endl;
            return;
        }
        for(const auto& a: packageHashs)
        {
            cout << "File Changes: " << a.first << endl;
        }

        ChangeReleaseFile(versionPath, packageHashs);
    }
}

void DataLoad::GiveWorkerJob()
{
    if (m_interceptData.empty())
    {
        cout << "Data for intercept empty. Nothing change" << endl;
        return;
    }
    for (auto &record: m_interceptData)
    {
        packageInfo info;
        if(!GetFileInfo(record.second.first, info))
        {
            cout << "Error: Get file size and hash " 
                 << record.second.first << endl;
            return;
        }
        record.second.second = move(info);
    }

    path archivePath(m_ubuntuArchive);
    if (!exists(archivePath) || boost::filesystem::is_empty(archivePath))
    {
        cout << absolute(archivePath) << " not exists." << endl;
        return;
    }

    for(const auto& dir: boost::make_iterator_range(
                         directory_iterator(archivePath), {}))
    {
        m_workQueue.push(dir.path().string());
    }
    for (unsigned int i = 0; i < (boost::thread::hardware_concurrency() / 2u);
         ++i)
    {
        m_threads.add_thread(new boost::thread(bind(&DataLoad::Worker, this)));
    }
    m_threads.join_all();
}

int main()
{
    // first param name Package you wand intercept, 
    // second param path file on what you want intercept
    std::string packet = "mc_4.7.0-1ubuntu2_i386.deb";
    DataLoad::fullPackageData test_data{packet, DataLoad::packageInfo{}};
    std::map<std::string, DataLoad::fullPackageData> interceptData;
    interceptData.emplace(std::make_pair(std::string("firefox"), test_data));
    DataLoad intercept(interceptData);
    //intercept.Load();
    intercept.GiveWorkerJob();
    return 0;
}
