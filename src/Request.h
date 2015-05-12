#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>

using namespace std;
namespace ramulator
{

class CommandInfo {
 public:
  int type;
  long clk;
 CommandInfo() {}
 CommandInfo(int type, int clk):
     type(type), clk(clk) {}
};

class Request
{
public:
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
    vector<CommandInfo> cmds;

    Request(long addr, Type type, function<void(Request&)> callback)
        : addr(addr), type(type), callback(callback) {}

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback)
        : addr_vec(addr_vec), type(type), callback(callback) {}

    Request(){}

    void add_command(int cmd, long clk) {
      cmds.emplace_back(cmd, clk);
    }
};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/

