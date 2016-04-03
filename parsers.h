#ifndef OS_EXTRA_PARSERS_H
#define OS_EXTRA_PARSERS_H

#include <string.h>

#include <vector>

#include "helper.h"

using namespace std;

int parse_args(char *args, size_t len, execargs_t &prog) {
    vector<char *> temp;
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
                size_t sz = (size_t) (i - st);
                if (args[i - 1] == '\'' || args[i - 1] == '\"')
                    sz--;
                if (args[st] == '\'' || args[i - 1] == '\"') {
                    st++;
                    sz--;
                }
                char *str = (char *) malloc((sz + 1));
                memcpy(str, args + st, sz);
                str[sz] = 0;
                temp.push_back(str);
                st = -1;
            }
        }
    }
    if (quote || double_quote) //braces not paired
        return 0;

    size_t sz = (size_t) (len - st);
    if (args[len - 1] == '\'' || args[len - 1] == '\"')
        sz--;
    if (args[st] == '\'' || args[len - 1] == '\"') {
        st++;
        sz--;
    }
    char *str = (char *) malloc((sz + 1));
    memcpy(str, args + st, sz);
    str[sz] = 0;
    temp.push_back(str);

    prog = execargs_from_vector(temp);

    return 1;
}

//TODO: fix parsing and pass all test
int parse_command(char *args, size_t len, vector<execargs_t> &progs) {
    int pos = 0;
    for (pos; pos < len; pos++)
        if (args[pos] == '\n')
            break;
    if (pos == len)
        return 0;
    int st = 0;
    bool double_quote = false, quote = false;
    for (int i = 0; i < pos; i++) {
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
        if (args[i] == '|' && !(double_quote || quote)
            || (args[i] != '|' && i == pos - 1)) {
            execargs_t cur;
            if (i == pos - 1)
                i++;
            if (parse_args(args + st, (size_t) i - st, cur) <= 0)
                return -1; //can't parse args
            progs.push_back(cur);
            st = i + 1;
        }
    }
    return 1;
}


#endif //OS_EXTRA_PARSERS_H
