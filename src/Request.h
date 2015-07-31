#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>

using namespace std;

namespace ramulator
{

class Request
{
public:
    bool is_first_command;
    long addr;
    // long addr_row;
    vector<int> addr_vec;

    enum class Type
    {
        READ,
        WRITE,
        REFRESH,
        POWERDOWN,
        SELFREFRESH,
        EXTENSION,
        MAX
    } type;

    long arrive;
    long depart;
    function<void(Request&)> callback; // call back with more info

    Request(long addr, Type type)
        : is_first_command(true), addr(addr), type(type),
      callback([](Request& req){}) {}

    Request(long addr, Type type, function<void(Request&)> callback)
        : is_first_command(true), addr(addr), type(type), callback(callback) {}

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback)
        : is_first_command(true), addr_vec(addr_vec), type(type), callback(callback) {}

    Request()
        : is_first_command(true) {}
};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/

