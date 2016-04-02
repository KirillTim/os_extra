#ifndef OS_EXTRA_PARSERS_H
#define OS_EXTRA_PARSERS_H

#include <string>
#include <vector>

using namespace std;

typedef vector<string> program;

int parse_args(char *args, size_t len, program& prog) {
    if (len == 0)
        return -1;
    int first = 0;
    while (args[first] == ' ')
        first++;
    while (args[len - 1] == ' ')
        len--;
    bool double_quote = false, quote = false;
    int st = -1;
    for (int i = first; i < len; i++) {
        if (args[i] != ' ') {
            if (st == -1)
                st = i;
        }
        if (args[i] == '\'') {
            if (!double_quote)
                quote = !quote;
        }
        if (args[i] == '\"') {
            if (!quote)
                double_quote = !double_quote;
        }
        if (args[i] == '\\') {
            i++; //just skip one symbol
        }
        if (args[i] == ' ' && !(double_quote || quote)) {
            if (st != -1) {
                prog.push_back(string(args + st, (size_t) (i - st)));
                st = -1;
            }
        }
    }
    if (quote || double_quote) //braces not paired
        return 0;
    prog.push_back(string(args + st, (size_t) (len - st)));
    return 1;
}

int parse_command(char* args, size_t len, vector<program>& progs) {
    int pos = 0;
    for (pos; pos < len; pos++)
        if (args[pos] == '\n')
            break;
    if (pos == len)
        return 0;
    int st = 0;
    for (int i = 0; i < pos; i++) {
        if (args[i] == '|') {
            program cur;
            if (parse_args(args+st, (size_t)i-st, cur) <= 0)
                return -1; //can't parse args
            progs.push_back(cur);
        }
    }
    return 1;
}


#endif //OS_EXTRA_PARSERS_H
