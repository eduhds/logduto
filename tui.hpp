#include "libs/termbox2.h"

using namespace std;

const int fixedLines = 11;

const char X_ICON[4] = "✗";
const char UP_ICON[4] = "↑";
const char DOWN_ICON[4] = "↓";

int maxRecordLines(int height)
{
    int freeLines = height - fixedLines;
    return freeLines > 0 ? freeLines : 0;
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
