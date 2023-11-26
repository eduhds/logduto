#include <iostream>
#include <fstream>
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
    target_file file = resolve_file(path);

    ofstream logFile(file.basename + ".txt");

    logFile << reqData;
    logFile << resData;

    logFile.close();
}
