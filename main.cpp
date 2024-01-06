/**
 * Logduto
 * An unpretentious HTTP request logger.
 * Created by Eduardo H. da Silva on 30/08/2023
 */

#include <iostream>
#include <filesystem>
#include "libs/argparse.hpp"
#include "logduto.hpp"
#include "title.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "libs/httplib.h"

#define PROGRAM_NAME "Logduto"
#define PROGRAM_VERSION "0.0.1"
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "8099"
#define CLIENT_TIMEOUT 10

using namespace std;

void handleResultSuccess(Logduto &logduto, const httplib::Request &req, httplib::Response &res, httplib::Result &result);

void handleResultError(httplib::Response &res);

bool isInvalidHeader(const string &header);

int main(int argc, char *argv[])
{
    argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);

    program.add_argument("-u", "--url")
        .help("resource url")
        .required();

    program.add_argument("-H", "--host")
        .help("specify host")
        .default_value(DEFAULT_HOST);

    program.add_argument("-p", "--port")
        .help("specify port")
        .default_value(DEFAULT_PORT);

    program.add_argument("-d", "--data")
        .help("save request and response files")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-l", "--logs")
        .help("specify logs directory")
        .default_value("");

    string resourceUrl, host, logsDir;
    bool saveData = false;
    int port;

    try
    {
        program.parse_args(argc, argv);

        resourceUrl = program.get<string>("--url");
        host = program.get<string>("--host");
        port = stoi(program.get<string>("--port"));
        saveData = program.get<bool>("--data");
        logsDir = program.get<string>("--logs");

        if (!logsDir.empty())
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
    client.set_connection_timeout(CLIENT_TIMEOUT, 0);
    client.set_read_timeout(CLIENT_TIMEOUT, 0);
    client.set_write_timeout(CLIENT_TIMEOUT, 0);

    auto controller = [&](const httplib::Request &req, httplib::Response &res)
    {
        try
        {
            string path = req.matches[0].str();
            string method = req.method;
            string contentType = req.has_header("Content-Type") ? req.get_header_value("Content-Type") : "text/plain";

            cout << "\n\x1B[34m--- " << method << " ---\033[0m" << endl;

            if (method == "OPTIONS")
            {
                cout << "Path: " << path << endl;
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

            if (logsDir != "")
            {
                logduto.logsDir = logsDir;
            }

            withHeadersAndParams = withHeaders && withParams;
            withHeadersAndBody = withHeaders && withBody;

            cout << "Path: " << path << endl;

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
                handleResultSuccess(logduto, req, res, result);
                return;
            }

            auto err = result.error();
            throw runtime_error("Cannot obtain result from " + path + ".\n" + httplib::to_string(err));
        }
        catch (const exception &e)
        {
            handleResultError(res);
            cerr << e.what() << '\n';
        }
    };

    string urlPattern = "(.*)";

    server.Get(urlPattern, controller);
    server.Post(urlPattern, controller);
    server.Put(urlPattern, controller);
    server.Patch(urlPattern, controller);
    server.Delete(urlPattern, controller);
    server.Options(urlPattern, controller);

    cout << "\x1B[34m" << title << "\033[0m" << endl;

    cout << "• Forwarding from \x1B[32mhttp://" << host << ":" << port << "\033[0m to \x1B[31m" << resourceUrl << "\033[0m" << endl;

    server.listen(host, port);

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

    cout << "Status: " << result->status << endl;

    logduto.setReqData(ReqData(reqHeaders, req.body.data(), reqCtnType));
    logduto.setResData(ResData(result->status, resHeaders, result->body.data(), resCtnType));

    logduto.saveToFile();

    res.status = result->status;
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_content(result->body, resCtnType);
}

void handleResultError(httplib::Response &res)
{
    cout << "\n\x1B[31m--- Error ---\033[0m" << endl;
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
