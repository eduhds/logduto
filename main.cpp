#include <iostream>
#include "libs/httplib.h"
#include "libs/argparse.hpp"
#include "log.hpp"

#define PROGRAM_NAME "program_name"
#define PROGRAM_VERSION "0.0.1"
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "8099"

using namespace std;

void handleResultSuccess(const httplib::Request &req, httplib::Response &res, httplib::Result &result);

void handleResultError(httplib::Response &res);

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

    // server.set_default_headers({{"Access-Control-Allow-Origin", "*"}});

    auto controller = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();
        string method = req.method;

        cout << "\n--- " << method << " ---" << endl;
        cout << "Path: " << path << endl;

        httplib::Result result;
        httplib::Params params;
        params.emplace("json", req.body);

        if (method == "POST")
        {
            result = client.Post(path, req.headers, params);
        }
        else if (req.method == "PUT")
        {
            result = client.Put(path, req.headers, params);
        }
        else if (req.method == "PATCH")
        {
            result = client.Patch(path);
        }
        else if (req.method == "DELETE")
        {
            result = client.Delete(path, req.headers);
        }
        else if (req.method == "OPTIONS")
        {
            result = client.Options(path, req.headers);
        }
        else
        {
            // Default GET
            result = client.Get(path, req.headers);
        }

        if (result)
        {
            handleResultSuccess(req, res, result);
            return;
        }

        handleResultError(res);
    };

    string urlPattern = R"(/(.+))";

    server.Get(urlPattern, controller);
    server.Post(urlPattern, controller);
    server.Put(urlPattern, controller);
    server.Patch(urlPattern, controller);
    server.Delete(urlPattern, controller);
    server.Options(urlPattern, controller);

    cout << "Forwarding from http://" << host << ":" << port << " to " << resourceUrl << endl;

    server.listen(host, port);

    return 0;
}

void handleResultSuccess(const httplib::Request &req, httplib::Response &res, httplib::Result &result)
{
    string contentType = result->has_header("Content-Type") ? result->get_header_value("Content-Type") : "text/plain";

    cout << "Status: " << result->status << endl;
    cout << "Content-Type: " << contentType << endl;

    Log log(req.matches[0].str(), req.body.data(), result->body.data(), contentType);
    log.saveToFile();

    res.status = result->status;
    res.set_content(result->body, contentType);
}

void handleResultError(httplib::Response &res)
{
    res.set_content("--- Error ---", "text/plain");
}
