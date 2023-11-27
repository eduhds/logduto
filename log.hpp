#include <iostream>

using namespace std;

class Log
{
public:
    string path;
    string reqData;
    string resData;
    string contentType;

    Log(string pth, string rqd, string rsd, string ctp);

    void saveToFile();
};
