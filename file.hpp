#include <iostream>
#include <filesystem>

using namespace std;

struct target_file
{
    string path;
    string name;
    string basename;
    string extension;
};

target_file resolve_file(string path)
{
    target_file file;

    file.path = path;
    file.basename = filesystem::path(path).stem().string();
    file.extension = filesystem::path(path).extension().string();
    file.name = filesystem::path(file.basename).stem().string();

    return file;
}
