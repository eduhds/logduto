#include <iostream>

using namespace std;

class Logduto
{
private:
    string path;
    string reqData;
    string resData;
    string contentType;

    string extFromContentType();

public:
    Logduto(string pth, string rqd, string rsd, string ctp);

    void saveToFile();
};
