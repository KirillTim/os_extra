#include <iostream>
#include "parsers.h"

using namespace std;

typedef vector<string> program;

void print_diff(const vector<string>& expected, const vector<string>& actual) {
    cerr<<"expected:\n";
    for (auto& i : expected)
        cerr<<i<<"\n";
    cerr<<"actual:\n";
    for (auto& i : actual)
        cerr<<i<<"\n";
}

void test_parse_args_ok(const string line, const vector<string>& ans) {
    vector<string> prog;
    int res = parse_args((char*)line.c_str(), line.size(), prog);
    if (res != 1) {
        cerr << "res != -1\n";
        return;
    }
    if (ans.size() != prog.size()) {
        print_diff(ans, prog);
        return;
    }
    for (int i = 0; i < ans.size(); i++) {
        if (ans[i] != prog[i]) {
            print_diff(ans, prog);
            return;
        }
    }
    cerr<<"OK\n";
}

void test_parse_args_fail(const string line) {
    vector<string> prog;
    if (parse_args((char*)line.c_str(), line.size(), prog) != 0)
        cerr<<"for line: "<<line<<" 0 expected\n";
    else
        cerr<<"OK\n";

}

void test_all() {
    test_parse_args_ok("cat /proc/cpuinfo", {"cat", "/proc/cpuinfo"});
    test_parse_args_ok("grep 'model name'", {"grep", "'model name'"});
    test_parse_args_ok("sed    -re    's/.*: (.*)/\\1/'", {"sed", "-re", "'s/.*: (.*)/\\1/'"});
    test_parse_args_ok(" ./custom \"spaces\\\" word\"   'and \" more' ",
                       {"./custom", "\"spaces\\\" word\"", "'and \" more'"});
    test_parse_args_fail("sed 'blah");
    test_parse_args_fail("grep bl\"ah");
}