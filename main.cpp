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

    string resource_url, host;
    int port;

    try
    {
        program.parse_args(argc, argv);

        resource_url = program.get<string>("--url");
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
    httplib::Client client(resource_url);

    auto get_handler = [&](const httplib::Request &req, httplib::Response &res)
    {
        string path = req.matches[0].str();

        auto resp = client.Get(path);

        string content_type = resp->has_header("Content-Type") ? resp->get_header_value("Content-Type") : "text/plain";

        Log log(path, req.body.data(), resp->body.data());

        log.saveToFile();

        res.set_content(resp->body, content_type);
    };

    server.Get(R"(/(.+))", get_handler);

    cout << "Forwarding from http://" << host << ":" << port << " to " << resource_url << endl;

    server.listen(host, port);

    return 0;
}
