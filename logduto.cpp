#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <regex>
#include "logduto.hpp"
#include "targetfile.hpp"

#define ROOT_LOGS_DIR "./logs"

using namespace std;

string removeNewLine(string str)
{
    return regex_replace(str, regex("\n"), "");
}

ReqData::ReqData(string h, string b, string c)
{
    headers = removeNewLine(h);
    body = removeNewLine(b);
    contentType = removeNewLine(c);
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
    headers = removeNewLine(h);
    body = removeNewLine(b);
    contentType = removeNewLine(c);
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
    method = removeNewLine(mtd);
    path = removeNewLine(pth);
    saveRequestData = saveReq;
    saveResponseData = saveRes;
}

string Logduto::extFromContentType()
{
    string contentType = reqData.getContentType();
    if (contentType == "text/plain")
    {
        return ".txt";
    }

    int startValue = contentType.find("/") + 1;
    int endValue = contentType.find(";") - contentType.find("/") - 1;
    return "." + contentType.substr(startValue, endValue);
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
        date = removeNewLine(regex_replace(date, regex("  "), " "));

        string dateFormat = regex_replace(date, regex(" "), "_");

        target_file tfile = resolve_file(path);
        tfile.extension = extFromContentType();

        string directories = ROOT_LOGS_DIR;
        directories += tfile.path[0] == '/' ? "" : "/";
        directories += tfile.path.substr(0, tfile.path.find_last_of("/"));

        filesystem::create_directories(directories);

        string logFileName = method + regex_replace(path, regex("/"), "_") + "_" + dateFormat;
        ofstream logFile(directories + "/" + logFileName + ".log");

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
                << resData.getBody() << "\n\n";

        logFile.close();

        if (saveRequestData)
        {
            ofstream reqFile(directories + "/" + tfile.basename + tfile.extension);
            reqFile << reqData.getBody();
            reqFile.close();
        }

        if (saveResponseData)
        {
            ofstream resFile(directories + "/" + tfile.basename + tfile.extension);
            resFile << resData.getBody();
            resFile.close();
        }
    }
    catch (const exception &e)
    {
        cerr << "Failed to save log file" << endl;
        cerr << e.what() << endl;
    }
}
