/**
 * Logduto
 * An unpretentious HTTP request logger.
 * Created by Eduardo H. da Silva on 30/08/2023
 */

#include <iostream>
#include <filesystem>
#include <thread>
#include <ctime>
#include "libs/argparse.hpp"
#include "libs/termbox2.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "libs/httplib.h"
#include "logduto.hpp"
#include "title.hpp"

#define PROGRAM_NAME "Logduto"
#define PROGRAM_VERSION "0.0.4"
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "8099"
#define DEFAULT_TIMEOUT "10"
#define DEFAULT_LOGS_DIR "./logs"

using namespace std;

string resourceUrl, host, logsDir;
bool saveData = false, cleanLogs = false;
int port, timeout;
const int maxReports = 100;
const int fixedLines = 13;
int countFiles = 0;
float sizeFiles = 0;
bool logsCleaned = false;

void handleResultSuccess(Logduto &logduto, const httplib::Request &req, httplib::Response &res, httplib::Result &result);

void handleResultError(httplib::Response &res);

bool isInvalidHeader(const string &header);

int printUI(int w, int h);

int calcFreeLines(int h);

int calcMaxResultLines(int freeLines);

void countLogFiles();

bool cleanLogFiles();

int methodColor(string method);

int main(int argc, char *argv[])
{
    argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);

    program.add_argument("url")
        .help("URL to redirect all requests to")
        .required();

    program.add_argument("-H", "--host")
        .help("specify host for the server")
        .default_value(DEFAULT_HOST);

    program.add_argument("-p", "--port")
        .help("specify port for the server")
        .default_value(DEFAULT_PORT);

    program.add_argument("-l", "--logs")
        .help("specify the directory where to save logs, requests and responses files")
        .default_value(DEFAULT_LOGS_DIR);

    program.add_argument("-t", "--timeout")
        .help("specify timeout for the client")
        .default_value(DEFAULT_TIMEOUT);

    program.add_argument("-d", "--data")
        .help("saves requests and responses to files")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-c", "--clean")
        .help("cleans log files")
        .default_value(false)
        .implicit_value(true);

    try
    {
        program.parse_args(argc, argv);

        resourceUrl = program.get<string>("url");
        host = program.get<string>("--host");
        port = stoi(program.get<string>("--port"));
        saveData = program.get<bool>("--data");
        logsDir = program.get<string>("--logs");
        timeout = stoi(program.get<string>("--timeout"));
        cleanLogs = program.get<bool>("--clean");

        if (logsDir != DEFAULT_LOGS_DIR)
        {
            if (!filesystem::is_directory(logsDir))
                throw runtime_error("Specified logs directory is not a directory\n");
        }

        filesystem::create_directories(logsDir);
    }
    catch (const exception &err)
    {
        cerr << err.what() << endl;
        cerr << program;
        exit(1);
    }

    if (cleanLogs)
    {
        logsCleaned = cleanLogFiles();
    }

    httplib::Server server;
    httplib::Client client(resourceUrl);

    client.enable_server_certificate_verification(false);
    client.set_connection_timeout(timeout, 0);
    client.set_read_timeout(timeout, 0);
    client.set_write_timeout(timeout, 0);

    tb_init();

    struct tb_event ev;
    int y = 0, w = tb_width(), h = tb_height();
    int freeLines = calcFreeLines(h);
    int maxResultLines = calcMaxResultLines(freeLines);
    string reports[maxReports][3];
    int pos = 0;

    auto printRequestsUI = [&](string method, string path, string timemin)
    {
        if (maxResultLines < 1)
            return;

        if (pos == maxReports - 1)
        {
            if (!reports[pos][0].empty())
            {
                for (int i = 0; i < pos; i++)
                {
                    reports[i][0] = reports[i + 1][0];
                    reports[i][1] = reports[i + 1][1];
                    reports[i][2] = reports[i + 1][2];
                }
            }
        }

        reports[pos][0] = method;
        reports[pos][1] = path;
        reports[pos][2] = timemin;

        string emptyStr(w, ' ');
        int start = pos > (maxResultLines - 1) ? pos - (maxResultLines - 1) : 0;
        int line = 0;

        for (int i = start; i <= pos; i++)
        {
            // Clear previous result
            tb_printf(0, y + line, 0, 0, emptyStr.c_str());
            tb_printf(0, y + (line + 1), 0, 0, emptyStr.c_str());

            tb_printf(0, y + line, 0, 0, reports[i][2].c_str());
            tb_printf(9, y + line, 0, methodColor(reports[i][0]), " %s ", reports[i][0].c_str());
            tb_printf(reports[i][0].size() + 12, y + line, 0, 0, reports[i][1].c_str());
            line++;
        }

        tb_present();

        if (pos < maxReports - 1)
        {
            pos++;
        }
    };

    auto printResultsUI = [&](bool error, string method, string path, int status, string message)
    {
        string emptyStr(w, ' ');
        int firstResultLine = h - 3, secondResultLine = h - 2;

        tb_printf(0, firstResultLine, 0, 0, emptyStr.c_str());
        tb_printf(0, secondResultLine, 0, 0, emptyStr.c_str());

        if (error)
        {
            tb_printf(0, firstResultLine, TB_RED, 0, "> %s %s", method.c_str(), path.c_str());
            tb_printf(0, secondResultLine, TB_RED, 0, "> %s", message.c_str());
        }
        else
        {
            tb_printf(0, firstResultLine, TB_GREEN, 0, "> %s %s", method.c_str(), path.c_str());
            tb_printf(0, secondResultLine, TB_GREEN, 0, "> %d - %s", status, message.c_str());
        }

        tb_present();
    };

    auto controller = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();
        string method = req.method;
        string contentType = req.has_header("Content-Type") ? req.get_header_value("Content-Type") : "text/plain";

        string timemin = currentTimeStr();

        try
        {
            if (method == "OPTIONS")
            {
                printRequestsUI(method, path, timemin);
                Logduto::saveCalls(logsDir, "[↑] " + method + " " + path);

                res.set_header("Access-Control-Allow-Methods", "*");
                res.set_header("Access-Control-Allow-Headers", "*");
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Connection", "close");
                return;
            }

            httplib::Result result;
            httplib::Params params;
            httplib::Headers headers;
            string body = "";
            string queryParams = "";
            bool withHeaders = false;
            bool withParams = false;
            bool withBody = false;
            bool withHeadersAndParams = false;
            bool withHeadersAndBody = false;

            // Handle headers
            if (!req.headers.empty())
            {
                withHeaders = true;
                for (auto &header : req.headers)
                {
                    if (isInvalidHeader(header.first))
                        continue;
                    headers.insert({header.first, header.second});
                }
            }

            // Handle params
            if (!req.params.empty())
            {
                withParams = true;
                queryParams = "?";
                for (auto &param : req.params)
                {
                    queryParams += param.first + "=" + param.second + "&";
                    params.insert({param.first, param.second});
                }
                queryParams.pop_back();
                path += queryParams;
            }

            // Handle body
            if (!req.body.empty())
            {
                withBody = true;
                body = req.body;
            }

            Logduto logduto(method, path, saveData, saveData);
            logduto.logsDir = logsDir;

            Logduto::saveCalls(logduto.logsDir, "[↑] " + method + " " + path);

            withHeadersAndParams = withHeaders && withParams;
            withHeadersAndBody = withHeaders && withBody;

            printRequestsUI(method, path, timemin);

            if (method == "POST")
            {
                if (withHeadersAndParams)
                    result = client.Post(path, headers, params);
                else if (withHeadersAndBody)
                    result = client.Post(path, headers, body, contentType);
                else if (withHeaders)
                    result = client.Post(path, headers);
                else if (withParams)
                    result = client.Post(path, params);
                else if (withBody)
                    result = client.Post(path, body, contentType);
                else
                    result = client.Post(path);
            }
            else if (req.method == "PUT")
            {
                if (withHeadersAndParams)
                    result = client.Put(path, headers, params);
                else if (withHeadersAndBody)
                    result = client.Put(path, headers, body, contentType);
                else if (withParams)
                    result = client.Put(path, params);
                else if (withBody)
                    result = client.Put(path, body, contentType);
                else
                    result = client.Put(path);
            }
            else if (req.method == "PATCH")
            {
                if (withHeadersAndBody)
                    result = client.Patch(path, headers, body, contentType);
                else if (withBody)
                    result = client.Patch(path, body, contentType);
                else
                    result = client.Patch(path);
            }
            else if (req.method == "DELETE")
            {
                if (withHeadersAndBody)
                    result = client.Delete(path, headers, body, contentType);
                else if (withHeaders)
                    result = client.Delete(path, headers);
                else if (withBody)
                    result = client.Delete(path, body, contentType);
                else
                    result = client.Delete(path);
            }
            else //  Default GET
            {
                if (withHeaders)
                    result = client.Get(path, headers);
                else
                    result = client.Get(path);
            }

            if (result)
            {
                Logduto::saveCalls(logduto.logsDir, "[↓] " + method + " " + path + " " + to_string(result->status) + " - " + result->reason);
                printResultsUI(false, method, path, result->status, result->reason);
                handleResultSuccess(logduto, req, res, result);
                return;
            }

            auto err = result.error();
            throw runtime_error("Cannot obtain result from " + path + ": " + httplib::to_string(err));
        }
        catch (const exception &e)
        {
            string err = e.what();
            Logduto::saveCalls(logsDir, "[✗] " + method + " " + path + " " + err);
            printResultsUI(true, method, path, 0, err);
            handleResultError(res);
        }
    };

    string urlPattern = "(.*)";

    server.Get(urlPattern, controller);
    server.Post(urlPattern, controller);
    server.Put(urlPattern, controller);
    server.Patch(urlPattern, controller);
    server.Delete(urlPattern, controller);
    server.Options(urlPattern, controller);

    bool serverRunning = false;
    string serverError = "";

    auto startServer = [&]()
    {
        try
        {
            serverRunning = true;
            server.listen(host, port);
        }
        catch (const exception &e)
        {
            serverError = e.what();
            serverRunning = false;
        }
    };

    thread t(startServer);
    t.detach();

    y = printUI(w, h);

    while (true)
    {
        tb_poll_event(&ev);

        if (ev.type == TB_EVENT_RESIZE)
        {
            w = ev.w;
            h = ev.h;
            pos = 0;
            freeLines = calcFreeLines(h);
            maxResultLines = calcMaxResultLines(freeLines);

            for (int i = 0; i < maxReports; i++)
            {
                reports[i][0] = "";
                reports[i][1] = "";
                reports[i][2] = "";
            }

            tb_clear();
            y = printUI(w, h);
        }

        if (ev.key == 3 || ev.key == 27)
        {
            tb_shutdown();
            return 0;
        }
    }

    return 0;
}

void handleResultSuccess(Logduto &logduto, const httplib::Request &req, httplib::Response &res, httplib::Result &result)
{
    string reqCtnType = req.has_header("Content-Type") ? req.get_header_value("Content-Type") : "text/plain";
    string resCtnType = result->has_header("Content-Type") ? result->get_header_value("Content-Type") : "text/plain";

    string reqHeaders = "";
    for (auto &header : req.headers)
    {
        if (isInvalidHeader(header.first))
            continue;
        reqHeaders += header.first + ": " + header.second + "\n";
    }

    string resHeaders = "";
    for (auto &header : result->headers)
    {
        resHeaders += header.first + ": " + header.second + "\n";
    }

    logduto.setReqData(ReqData(reqHeaders, req.body.data(), reqCtnType));
    logduto.setResData(ResData(result->status, resHeaders, result->body.data(), resCtnType));

    logduto.saveToFile();

    res.status = result->status;
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(result->body, resCtnType);
}

void handleResultError(httplib::Response &res)
{
    res.status = 599;
    res.set_content("--- Error ---", "text/plain");
}

bool isInvalidHeader(const string &header)
{
    return header == "Host" ||
           header == "LOCAL_ADDR" ||
           header == "LOCAL_PORT" ||
           header == "REMOTE_ADDR" ||
           header == "REMOTE_PORT";
}

int printUI(int w, int h)
{
    int y = 0;
    string from = "http://" + host + ":" + to_string(port);
    string to = resourceUrl;
    string emptyStr(w, ' ');

    countLogFiles();
    float filesSize = sizeFiles > 1000 ? sizeFiles / 1000 : sizeFiles;
    string unity = "KB";

    if (filesSize > 1000)
    {
        filesSize /= 1000;
        unity = "MB";
    }

    if (filesSize > 1000)
    {
        filesSize /= 1000;
        unity = "GB";
    }

    // Print Title
    for (const string s : title_array)
    {
        tb_printf(0, y++, TB_BLUE, 0, s.c_str());
    }

    // Print Info
    tb_printf(0, y++, 0, 0, "");
    tb_printf(0, y, 0, 0, "Forwarding from ");
    tb_printf(16, y, TB_GREEN, 0, from.c_str());
    tb_printf(from.size() + 16, y, 0, 0, " to ");
    tb_printf(from.size() + 20, y++, TB_RED, 0, to.c_str());
    tb_printf(0, y++, 0, 0, "Press Esc or Ctrl-C to quit");
    tb_printf(0, y++, 0, 0, "");

    // Print Status bar
    tb_printf(0, h - 1, 0, TB_BLUE, emptyStr.c_str());
    if (logsCleaned)
    {
        logsCleaned = false;
        tb_printf(1, h - 1, TB_WHITE, TB_BLUE, "All logs cleaned");
    }
    else
    {
        tb_printf(1, h - 1, TB_WHITE, TB_BLUE, "%d logs (%.2f %s)", countFiles, filesSize, unity.c_str());
    }
    tb_printf(w - 7, h - 1, TB_WHITE, TB_BLUE, "v%s", PROGRAM_VERSION);

    tb_present();

    return y;
}

int calcFreeLines(int h)
{
    int freeLines = h - fixedLines;
    return freeLines > 0 ? freeLines : 0;
}

int calcMaxResultLines(int freeLines)
{
    return freeLines < maxReports ? freeLines : maxReports;
}

void countLogFiles()
{
    countFiles = 0;
    sizeFiles = 0;

    for (const auto &entry : filesystem::directory_iterator(logsDir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".log")
        {
            int fileSize = filesystem::file_size(entry.path());
            sizeFiles += fileSize;
            countFiles++;
        }
    }
}

bool cleanLogFiles()
{
    try
    {
        for (const auto &entry : filesystem::directory_iterator(logsDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".log")
                filesystem::remove(entry.path());
        }
        return true;
    }
    catch (const exception &e)
    {
        return false;
    }
}

int methodColor(string method)
{
    if (method == "GET")
        return TB_GREEN;
    if (method == "POST")
        return TB_YELLOW;
    if (method == "PUT")
        return TB_BLUE;
    if (method == "DELETE")
        return TB_RED;
    return TB_WHITE;
}