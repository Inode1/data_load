/*
 *
 */

#include <iostream>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "data_load.h"


using namespace std;
using namespace boost::filesystem;
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

bool DataLoad::GetFileInfo(const string &fileName, DataLoad::packageInfo &packageData, DataLoad::EPackageInfo info)
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
void DataLoad::SetPackageInfo(const DataLoad::packageInfo &packageData)
{
    m_packageInfo = packageData;
}


int main()
{
    DataLoad data("ru.archive.ubuntu.com/ubuntu/dists/");
   //data.Load();

    DataLoad::packageInfo d;
    DataLoad::GetFileInfo("data_load.cpp", d);
    cout << (std::get<1>(d)) << endl;





    return 0;

}
