#include <iostream>
#include <string>
#include <ctime>

using namespace std;

string currentTimeStr()
{
  time_t t = time(0);
  tm *now = localtime(&t);
  string hr = to_string(now->tm_hour);
  hr = hr.length() == 1 ? "0" + hr : hr;
  string mn = to_string(now->tm_min);
  mn = mn.length() == 1 ? "0" + mn : mn;
  string sc = to_string(now->tm_sec);
  sc = sc.length() == 1 ? "0" + sc : sc;

  return hr + ":" + mn + ":" + sc;
}

string currentDateStr()
{
  time_t t = time(0);
  tm *now = localtime(&t);
  string year = to_string(now->tm_year + 1900);
  string month = to_string(now->tm_mon + 1);
  month = month.length() == 1 ? "0" + month : month;
  string day = to_string(now->tm_mday);
  day = day.length() == 1 ? "0" + day : day;

  return year + "-" + month + "-" + day;
}
