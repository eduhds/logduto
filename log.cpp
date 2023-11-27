#include <iostream>
#include <fstream>
#include <filesystem>
#include "log.hpp"
#include "file.hpp"

#define ROOT_DATA_DIR "./data"

using namespace std;

Log::Log(string pth, string rqd, string rsd, string ctp)
{
    path = pth;
    reqData = rqd;
    resData = rsd;
    contentType = ctp;
}

void Log::saveToFile()
{
    try
    {
        target_file tfile = resolve_file(path);
        tfile.extension = "." + contentType.substr(contentType.find("/") + 1, contentType.find(";") - contentType.find("/") - 1);

        string directories = ROOT_DATA_DIR;
        directories += tfile.path[0] == '/' ? "" : "/";
        directories += tfile.path.substr(0, tfile.path.find_last_of("/"));

        filesystem::create_directories(directories);

        ofstream logFile(directories + "/" + tfile.basename + tfile.extension);

        logFile << reqData;
        logFile << resData;

        logFile.close();
    }
    catch (const exception &e)
    {
        cerr << "Failed to save log file" << endl;
        cerr << e.what() << endl;
    }
}
