/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/regex.hpp>

#include "data_load.h"

using namespace std;
using namespace boost::filesystem;
using namespace boost::iostreams;

const char* DataLoad::util[] = { "md5sum ",
                                 "sha1sum ",
                                 "sha256sum "
                               };
const string DataLoad::m_ubuntuArchive = "ru.archive.ubuntu.com/ubuntu/dists/";

DataLoad::DataLoad(const string &packageName):
        m_packageName(packageName)
{
}
bool DataLoad::Load()
{
    string command = string("wget -nv -R index.html -r --no-parent -A 'Packages.*','Release' > /dev/null 2>&1 ") + m_ubuntuArchive;
    cout << "Wait while all data will be loaded." << endl;
    if (system(command.c_str()))
    {
        cout << "Something happen. Data isn`t loaded." << endl;
        return false;
    }

    cout << "All data load." << endl;
    return true;
}

bool DataLoad::GetFileInfo(const string &fileName, packageInfo &packageData, EPackageInfo info)
{
    if (info == eAllField)
    {
        for (int i= 0; i < eAllField; ++i)
        {
           if (!GetFileInfo(fileName, packageData, static_cast<DataLoad::EPackageInfo>(i)))
               return false;
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

    string command = string{DataLoad::util[info]} + fileName;
    FILE *fp;
    fp =popen(command.c_str(),"r");
    boost::iostreams::stream_buffer<boost::iostreams::file_descriptor_source> fpstream(fileno(fp), boost::iostreams::never_close_handle);
    std::istream in (&fpstream);

    string line;
    if (in)
    {
        getline(in, line);
    }
    /* close */
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

DataLoad::packageInfo DataLoad::GetPackageInfo()
{
    return m_packageInfo;
}
void DataLoad::SetPackageInfo(const packageInfo &packageData)
{
    m_packageInfo = packageData;
}
bool DataLoad::ChangePackageFile(const boost::filesystem::path &packagePath, packageMap &result)
{
    string command;
    cout << packagePath << " and " << packagePath.extension() << endl;
    if (packagePath.extension().string() == ".bz2")
    {
        command = string("bunzip2 -f > /dev/null 2>&1 ") + packagePath.string();
    }
    else if (packagePath.extension().string() == ".gz")
    {
        command = string("gunzip -f > /dev/null 2>&1 ") + packagePath.string();
    }
    else
    {
        return false;
    }

    cout << "Unzip:" << packagePath << " Start" << endl;
    if (system(command.c_str()))
    {
        cout << "Modified:" << packagePath << " Failed" << endl;
        return false;
    }

    cout << "Unzip:" << packagePath << " Finished" << endl;

    path filePath(packagePath.parent_path() / "Packages");
    cout << filePath << endl;
    if (!exists(filePath))
    {
        cout << filePath << " not exist" << endl;
        return false;
    }
    mapped_file mapPackege(filePath.string());
    stream<mapped_file> is(mapPackege, std::ios_base::binary);
    string line;

    while(getline(is, line))
    {
        if (line.find(string{"Package: "} + m_packageName) != std::string::npos)
        {
            cout << "Find" << endl;
            boost::regex findString("^Size: .*");
            while(getline(is, line))
            {
                if (boost::regex_match(line, findString))
                {
                    is.seekp(-(line.length() + 1), ios_base::cur);
                    // check then length insert the same
                    string size = string{"Size: "} + get<eSize>(m_packageInfo);
                    int diff = line.length() - size.length();
                    is << size                                         << endl;
                    is << "MD5sum: " << get<eMD5Sum>(m_packageInfo)    << endl;
                    is << "SHA1: "   << get<eSHA1Sum>(m_packageInfo)   << endl;
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
                        cout << line << endl;
                        is.seekp(length, ios_base::beg);

                    }
                    is << "SHA256: " << get<eSHA256Sum>(m_packageInfo) << endl;
                    cout << diff << endl;
                    if (diff > 0)
                    {
                        is << line << string(diff, '!') << endl;
                    }
                    else if (diff < 0)
                    {
                        is << line.substr(0, line.length() + diff) << endl;
                    }

/*                    if (diff > 0)
                    {
                        length = is.tellg();
                        getline(is, line);
                        getline(is, line);
                        cout << line << endl;
                        is.seekp(length, ios_base::beg);
                        is << line << string(diff, '!') << endl;

                     //       is << line << string(diff, '!');
                    }*/

                    cout << "Package:" << m_packageName << " modified in " <<  packagePath.parent_path()
                         << " directory" << endl;
                    break;
                }
            }
            break;
        }
    }

    is.~stream<mapped_file>();
    mapPackege.~mapped_file();
    command = string("bzip2 -f -k > /dev/null 2>&1 ") + filePath.string();
    system(command.c_str());
    command = string("gzip -f > /dev/null 2>&1 ") +  filePath.string();
    system(command.c_str());
    return true;
}
bool DataLoad::ChangeReleaseFile(const packageMap &result)
{
    return true;
}
int main()
{
    DataLoad data("account-plugin-aim");
   //data.Load();

    DataLoad::packageInfo d;
    DataLoad::GetFileInfo("account-plugin-aim", d);
    data.SetPackageInfo(d);
    DataLoad::packageMap result;
    data.ChangePackageFile(path("Packages.bz2"), result);
    //cout << (get<1>(d)) << endl;





    return 0;

}
