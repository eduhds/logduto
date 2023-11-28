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

    auto getHandler = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();

        auto resp = client.Get(path, req.headers);

        string contentType = resp->has_header("Content-Type") ? resp->get_header_value("Content-Type") : "text/plain";

        Log log(path, req.body.data(), resp->body.data(), contentType);

        log.saveToFile();

        res.set_content(resp->body, contentType);
    };

    auto postHandler = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();

        httplib::Params params;
        params.emplace("json", req.body);

        auto resp = client.Post(path, req.headers, params);

        string contentType = resp->has_header("Content-Type") ? resp->get_header_value("Content-Type") : "text/plain";

        Log log(path, req.body.data(), resp->body.data(), contentType);

        log.saveToFile();

        res.set_content(resp->body, contentType);
    };

    string urlPattern = R"(/(.+))";

    server.set_default_headers({{"Access-Control-Allow-Origin", "*"}});

    server.Get(urlPattern, getHandler);
    server.Post(urlPattern, postHandler);

    cout << "Forwarding from http://" << host << ":" << port << " to " << resourceUrl << endl;

    server.listen(host, port);

    return 0;
}
