#include <iostream>

using namespace std;

class Log
{
public:
    string filePath;
    string path;
    string reqData;
    string resData;

    Log(string pth, string rqd, string rsd);

    void saveToFile();
};
