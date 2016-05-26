#include <string>
#include <iostream>
#include <iomanip>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

using namespace boost::filesystem;
using std::cout;
using std::endl;

bool ChangeReleaseFile()
{
    std::fstream is("test_file");
    std::string line;
    // TO-DO:: bad performance.
    cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    boost::regex e ("^Package: firefox(.*)$");

    //std::string packetToChange = "firefox";

    while(getline(is, line))
    {
        boost::smatch match;

        if (boost::regex_match(line, match, e))
        {
            std::string matchResult = match[1];
            boost::regex findString("^Size: .*");
            while(getline(is, line))
            {
                if (boost::regex_match(line, findString))
                {
                    is.seekp(-(line.length() + 1), std::ios_base::cur);
                    // check then length insert the same

                    std::string size{"Size: 1024"};
                    int diff = line.length() - size.length();
                    is << size                                                           
                       << endl;
                    is << "MD5sum: " 
                       << "ffffffffffffffffffffffffffffffff"    
                       << endl;
                    is << "SHA1: "   
                       << "ffffffffffffffffffffffffffffffffffffffff"   
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
                        is.seekp(length, std::ios_base::beg);
                    }
                    is << "SHA256: " 
                       << "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" 
                       << endl;
                    if (diff > 0)
                    {
                        is << line << std::string(diff, '!') << endl;
                    }
                    else if (diff < 0)
                    {
                        is << line.substr(0, line.length() + diff) << endl;
                    }
                    break;
                }
            }
        }
    }
    return true;
}

int main()
{
    ChangeReleaseFile();
}