#include <iostream>

using namespace std;

class Log
{
private:
    string path;
    string reqData;
    string resData;
    string contentType;

    string extFromContentType();

public:
    Log(string pth, string rqd, string rsd, string ctp);

    void saveToFile();
};
