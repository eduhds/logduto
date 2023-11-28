#include <iostream>
#include "libs/httplib.h"
#include "libs/argparse.hpp"
#include "log.hpp"

#define PROGRAM_NAME "program_name"
#define PROGRAM_VERSION "0.0.1"
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_PORT "8099"

using namespace std;

int main(int argc, char *argv[])
{
    argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);

    program.add_argument("-u", "--url")
        .required()
        .help("resource url");

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

    server.set_default_headers({{"Access-Control-Allow-Origin", "*"}});

    auto commonHandler = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();

        httplib::Params params;
        httplib::Result result;

        if (req.body.size() > 0)
        {
            params.emplace("json", req.body);
        }

        if (req.method == "POST")
            result = client.Post(path, req.headers, params);
        else if (req.method == "PUT")
            result = client.Put(path, req.headers, params);
        else if (req.method == "PATCH")
            result = client.Patch(path);
        else if (req.method == "DELETE")
            result = client.Delete(path, req.headers);
        else
            result = client.Get(path, req.headers);

        string contentType = result->has_header("Content-Type") ? result->get_header_value("Content-Type") : "text/plain";

        Log log(path, req.body.data(), result->body.data(), contentType);

        log.saveToFile();

        res.status = result->status;
        res.set_content(result->body, contentType);
    };

    auto optionsHandler = [](const httplib::Request &req, httplib::Response &res)
    {
        res.status = 200;
    };

    string urlPattern = R"(/(.+))";

    server.Get(urlPattern, commonHandler);
    server.Post(urlPattern, commonHandler);
    server.Put(urlPattern, commonHandler);
    server.Patch(urlPattern, commonHandler);
    server.Delete(urlPattern, commonHandler);
    server.Options(urlPattern, optionsHandler);

    cout << "Forwarding from http://" << host << ":" << port << " to " << resourceUrl << endl;

    server.listen(host, port);

    return 0;
}
