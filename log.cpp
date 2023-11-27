#include <iostream>
#include <fstream>
#include <filesystem>
#include "log.hpp"
#include "file.hpp"

using namespace std;

Log::Log(string pth, string rqd, string rsd)
{
    path = pth;
    reqData = rqd;
    resData = rsd;
}

void Log::saveToFile()
{
    try
    {
        target_file file = resolve_file(path);

        string directories = "./data";
        directories += file.path[0] == '/' ? "" : "/";
        directories += file.path.substr(0, file.path.find_last_of("/"));

        filesystem::create_directories(directories);

        ofstream logFile(directories + "/" + file.basename + ".txt");

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
