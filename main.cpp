/**
 * Logduto
 * An unpretentious HTTP request logger.
 * Created by Eduardo H. da Silva on 30/08/2023
 */

#include <iostream>
#include <filesystem>
#include <thread>
#include "libs/argparse.hpp"
#include "libs/termbox2.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "libs/httplib.h"
#include "logduto.hpp"
#include "title.hpp"

#define PROGRAM_NAME "Logduto"
#define PROGRAM_VERSION "0.0.1"
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "8099"
#define DEFAULT_TIMEOUT "10"
#define DEFAULT_LOGS_DIR "./logs"

using namespace std;

void handleResultSuccess(Logduto &logduto, const httplib::Request &req, httplib::Response &res, httplib::Result &result);

void handleResultError(httplib::Response &res);

bool isInvalidHeader(const string &header);

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

    string resourceUrl, host, logsDir;
    bool saveData = false;
    int port, timeout;

    try
    {
        program.parse_args(argc, argv);

        resourceUrl = program.get<string>("url");
        host = program.get<string>("--host");
        port = stoi(program.get<string>("--port"));
        saveData = program.get<bool>("--data");
        logsDir = program.get<string>("--logs");
        timeout = stoi(program.get<string>("--timeout"));

        if (logsDir != DEFAULT_LOGS_DIR)
        {
            if (!filesystem::is_directory(logsDir))
                throw runtime_error("Specified logs directory is not a directory\n");
        }
    }
    catch (const exception &err)
    {
        cerr << err.what() << endl;
        cerr << program;
        exit(1);
    }

    httplib::Server server;
    httplib::Client client(resourceUrl);

    client.enable_server_certificate_verification(false);
    client.set_connection_timeout(timeout, 0);
    client.set_read_timeout(timeout, 0);
    client.set_write_timeout(timeout, 0);

    struct tb_event ev;
    int y = 0;

    tb_init();
    string emptyStr(tb_width(), ' ');

    const int maxReports = 5;
    string reports[maxReports][2];

    int pos = 0;

    auto updateRequestsUI = [&](string method, string path)
    {
        if (pos == maxReports - 1)
        {
            if (!reports[pos][0].empty())
            {
                for (int i = 0; i < pos; i++)
                {
                    reports[i][0] = reports[i + 1][0];
                    reports[i][1] = reports[i + 1][1];
                }
            }
        }

        reports[pos][0] = method;
        reports[pos][1] = path;

        for (int i = 0; i <= pos; i++)
        {
            // Clear previous result
            tb_printf(0, y + i, 0, 0, emptyStr.c_str());
            tb_printf(0, y + (i + 1), 0, 0, emptyStr.c_str());

            tb_printf(0, y + i, 0, TB_YELLOW, " %s ", reports[i][0].c_str());
            tb_printf(reports[i][0].size() + 3, y + i, 0, 0, reports[i][1].c_str());
        }

        tb_present();

        if (pos < maxReports - 1)
        {
            pos++;
        }
    };

    auto updateStatusUI = [&](bool error, string method, string path, int status, string message)
    {
        tb_printf(0, y + 7, 0, 0, emptyStr.c_str());
        tb_printf(0, y + 8, 0, 0, emptyStr.c_str());

        if (error)
        {
            tb_printf(0, y + 7, TB_RED, 0, "%s", message.c_str());
        }
        else
        {
            tb_printf(0, y + 7, TB_GREEN, 0, "[%s] %s", method.c_str(), path.c_str());
            tb_printf(0, y + 8, TB_GREEN, 0, "%d - %s", status, message.c_str());
        }

        tb_present();
    };

    auto controller = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();
        string method = req.method;
        string contentType = req.has_header("Content-Type") ? req.get_header_value("Content-Type") : "text/plain";

        try
        {

            if (method == "OPTIONS")
            {
                updateRequestsUI(method, path);

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

            withHeadersAndParams = withHeaders && withParams;
            withHeadersAndBody = withHeaders && withBody;

            updateRequestsUI(method, path);

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
                updateStatusUI(false, method, path, result->status, result->reason);
                handleResultSuccess(logduto, req, res, result);
                return;
            }

            auto err = result.error();
            throw runtime_error("Cannot obtain result from " + path + ": " + httplib::to_string(err));
        }
        catch (const exception &e)
        {
            string err = e.what();
            updateStatusUI(true, method, path, 0, err);
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

    for (const string s : title_array)
    {
        tb_printf(0, y++, TB_BLUE, 0, s.c_str());
    }

    string from = "http://" + host + ":" + to_string(port);
    string to = resourceUrl;
    string statusBar(73, ' ');

    tb_printf(0, y++, 0, 0, "");
    tb_printf(0, y, 0, 0, "Forwarding from ");
    tb_printf(16, y, TB_GREEN, 0, from.c_str());
    tb_printf(from.size() + 16, y, 0, 0, " to ");
    tb_printf(from.size() + 20, y++, TB_RED, 0, to.c_str());
    tb_printf(0, y++, 0, 0, "Press Esc or Ctrl-C to quit");
    tb_printf(0, y++, 0, 0, "");
    tb_printf(0, y + 6, 0, TB_BLUE, statusBar.c_str());

    tb_present();

    while (true)
    {
        tb_poll_event(&ev);

        if (ev.key == 3 || ev.key == 27)
        {
            tb_printf(0, y, 0, 0, "Stoping...");
            tb_present();
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
