#include <filesystem>

using namespace std;

bool cleanLogFiles(string directory)
{
    try
    {
        for (const auto &entry : filesystem::directory_iterator(directory))
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
