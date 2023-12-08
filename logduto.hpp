#include <iostream>

#define ROOT_LOGS_DIR "./logs"

using namespace std;

string removeLastNewLine(string str);

string removeLastSlash(string str);

string extFromContentType(string contentType);

class ReqData
{
private:
    string headers;
    string body;
    string contentType;

public:
    ReqData() = default;
    ReqData(string h, string b, string c);

    string getHeaders();
    string getBody();
    string getContentType();
};

class ResData
{
private:
    int status;
    string headers;
    string body;
    string contentType;

public:
    ResData() = default;
    ResData(int s, string h, string b, string c);

    int getStatus();
    string getHeaders();
    string getBody();
    string getContentType();
};

class Logduto
{
private:
    string method;
    string path;
    ReqData reqData;
    ResData resData;
    bool saveRequestData = false;
    bool saveResponseData = false;

public:
    string logsDir = ROOT_LOGS_DIR;

    Logduto(string mtd, string pth, bool saveReq, bool saveRes);

    void setReqData(ReqData req);
    void setResData(ResData res);

    void saveToFile();
};
