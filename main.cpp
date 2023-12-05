/**
 * Logduto
 * Salva requisições em arquivo
 */

#include <iostream>
#include "libs/argparse.hpp"
#include "logduto.hpp"
#include "title.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "libs/httplib.h"

#define PROGRAM_NAME "program_name"
#define PROGRAM_VERSION "0.0.1"
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "8099"

using namespace std;

void handleResultSuccess(const httplib::Request &req, httplib::Response &res, httplib::Result &result);

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

    string resourceUrl, host;
    int port;

    try
    {
        program.parse_args(argc, argv);

        resourceUrl = program.get<string>("--url");
        host = program.get<string>("--host");
        port = stoi(program.get<string>("--port"));
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

    auto controller = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();
        string method = req.method;

        cout << "\n--- " << method << " ---" << endl;
        cout << "Path: " << path << endl;

        if (method == "OPTIONS")
        {
            res.set_header("Access-Control-Allow-Methods", "*");
            res.set_header("Access-Control-Allow-Headers", "*");
            res.set_header("Access-Control-Allow-Origin", "*"); // req.get_header_value("Origin").c_str()
            res.set_header("Connection", "close");
            return;
        }

        httplib::Result result;
        httplib::Params params;
        httplib::Headers headers;

        // Set params
        if (!req.body.empty())
        {
            params.emplace("json", req.body);
        }

        // Set headers
        for (auto &header : req.headers)
        {
            if (isInvalidHeader(header.first))
                continue;
            headers.insert({header.first, header.second});
        }

        bool withHeaders = !headers.empty();
        bool withParams = !params.empty();
        bool withHeadersAndParams = withHeaders && withParams;

        if (method == "POST")
        {
            if (withHeadersAndParams)
                result = client.Post(path, headers, params);
            else if (withHeaders)
                result = client.Post(path, headers);
            else if (withParams)
                result = client.Post(path, params);
            else
                result = client.Post(path);
        }
        else if (req.method == "PUT")
        {
            if (withHeadersAndParams)
                result = client.Put(path, headers, params);
            else if (withParams)
                result = client.Put(path, params);
            else
                result = client.Put(path);
        }
        else if (req.method == "PATCH")
        {
            result = client.Patch(path);
        }
        else if (req.method == "DELETE")
        {
            if (withHeaders)
                result = client.Delete(path, headers);
            else
                result = headers.empty() ? client.Delete(path) : client.Delete(path, headers);
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
            handleResultSuccess(req, res, result);
            return;
        }

        handleResultError(res);
    };

    string urlPattern = "(.*)"; // R"((.+))";

    server.Get(urlPattern, controller);
    server.Post(urlPattern, controller);
    server.Put(urlPattern, controller);
    server.Patch(urlPattern, controller);
    server.Delete(urlPattern, controller);
    server.Options(urlPattern, controller);

    cout << title << endl;

    cout << "• Forwarding from http://" << host << ":" << port << " to " << resourceUrl << endl;

    server.listen(host, port);

    return 0;
}

void handleResultSuccess(const httplib::Request &req, httplib::Response &res, httplib::Result &result)
{
    string contentType = result->has_header("Content-Type") ? result->get_header_value("Content-Type") : "text/plain";

    cout << "Status: " << result->status << endl;
    cout << "Content-Type: " << contentType << endl;

    Logduto logduto(req.matches[0].str(), req.body.data(), result->body.data(), contentType);
    logduto.saveToFile();

    res.status = result->status;
    res.set_header("Access-Control-Allow-Origin", "*"); // req.get_header_value("Origin").c_str()
    res.set_content(result->body, contentType);
}

void handleResultError(httplib::Response &res)
{
    res.set_content("--- Error ---", "text/plain");
}

bool isInvalidHeader(const string &header)
{
    return header == "Host" ||
           header == "LOCAL_ADDR" ||
           header == "LOCAL_PORT" ||
           header == "REMOTE_ADDR" ||
           header == "REMOTE_PORT" ||
           header == "User-Agent";
}
