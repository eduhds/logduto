#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <regex>
#include "targetfile.hpp"
#include "util.hpp"

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
    string logsDir;

    Logduto(string mtd, string pth, bool saveReq, bool saveRes);

    void setReqData(ReqData req);
    void setResData(ResData res);

    void saveToFile();
};

string removeLastNewLine(string str)
{
    return (!str.empty() && str[str.length() - 1] == '\n') ? str.substr(0, str.length() - 1) : str;
}

string removeLastSlash(string str)
{
    return (!str.empty() && str[str.length() - 1] == '/') ? str.substr(0, str.length() - 1) : str;
}

string extFromContentType(string contentType)
{
    if (contentType == "text/plain")
        return ".txt";

    int startValue = contentType.find("/") + 1;
    int endValue = contentType.find(";") - contentType.find("/") - 1;
    return "." + contentType.substr(startValue, endValue);
}

ReqData::ReqData(string h, string b, string c)
{
    headers = removeLastNewLine(h);
    body = removeLastNewLine(b);
    contentType = removeLastNewLine(c);
}

string ReqData::getBody()
{
    return body;
}

string ReqData::getHeaders()
{
    return headers;
}

string ReqData::getContentType()
{
    return contentType;
}

ResData::ResData(int s, string h, string b, string c)
{
    status = s;
    headers = removeLastNewLine(h);
    body = removeLastNewLine(b);
    contentType = removeLastNewLine(c);
}

int ResData::getStatus()
{
    return status;
}

string ResData::getHeaders()
{
    return headers;
}

string ResData::getBody()
{
    return body;
}

string ResData::getContentType()
{
    return contentType;
}

Logduto::Logduto(string mtd, string pth, bool saveReq, bool saveRes)
{
    method = removeLastNewLine(mtd);
    path = removeLastSlash(removeLastNewLine(pth));
    saveRequestData = saveReq;
    saveResponseData = saveRes;
}

void Logduto::setReqData(ReqData req)
{
    reqData = req;
}

void Logduto::setResData(ResData res)
{
    resData = res;
}

void Logduto::saveToFile()
{
    try
    {
        time_t now = time(0);
        string date = ctime(&now);
        date = removeLastNewLine(regex_replace(date, regex("  "), " "));

        string dateFormat = currentDateStr() + "_" + currentTimeStr();

        target_file tfile = resolve_file(path);

        filesystem::create_directories(logsDir);

        string logFileName = method + regex_replace(path, regex("/"), "_") + "_" + dateFormat;
        ofstream logFile(logsDir + "/" + logFileName + ".log");

        logFile << "[DATE]\n"
                << date << "\n\n";

        logFile << "[URL]\n"
                << method << " " << path << "\n\n";

        logFile << "[REQUEST HEADERS]\n"
                << reqData.getHeaders() << "\n\n";
        logFile << "[REQUEST BODY]\n"
                << reqData.getBody() << "\n\n";

        logFile << "[RESPONSE STATUS]\n"
                << resData.getStatus() << "\n\n";
        logFile << "[RESPONSE HEADERS]\n"
                << resData.getHeaders() << "\n\n";
        logFile << "[RESPONSE BODY]\n"
                << resData.getBody() << "\n";

        logFile.close();

        if (saveRequestData || saveResponseData)
        {
            string reqDir = logsDir + "/data/request/" + method + "/";
            string resDir = logsDir + "/data/response/" + method + "/";

            string reqResDirectories = tfile.path[0] == '/' ? "" : "/";
            reqResDirectories += tfile.path.substr(0, tfile.path.find_last_of("/"));

            if (saveRequestData && !reqData.getBody().empty())
            {
                reqDir += reqResDirectories;
                filesystem::create_directories(reqDir);
                ofstream reqFile(reqDir + "/" + tfile.basename + extFromContentType(reqData.getContentType()));
                reqFile << reqData.getBody();
                reqFile.close();
            }

            if (saveResponseData && !resData.getBody().empty())
            {
                resDir += reqResDirectories;
                filesystem::create_directories(resDir);
                ofstream resFile(resDir + "/" + tfile.basename + extFromContentType(resData.getContentType()));
                resFile << resData.getBody();
                resFile.close();
            }
        }
    }
    catch (const exception &e)
    {
        cerr << "Failed to save log file" << endl;
        cerr << e.what() << endl;
    }
}
