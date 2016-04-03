#ifndef OS_EXTRA_HELPER_H_H
#define OS_EXTRA_HELPER_H_H

#include <sys/types.h>
#include <stdlib.h>
#include <vector>

typedef char **execargs_t;

int exec(execargs_t args) {
    /*cerr<<"exec()\n";
    cerr<<"prog: "<<*args<<"\n";
    int i = 0;
    cerr<<"args:";
    while(args[i] != 0) {
        cerr<<"len: "<<strlen(args[i])<<", "<<(args[i])<<"\n";
        i++;
    }*/
    return execvp(*args, args);
}

void execargs_free(execargs_t &args) {
    for (int i = 0; args[i] != 0; i++)
        free(args[i]);
    free(args);
    args = NULL;
}

execargs_t execargs_from_vector(std::vector<char *> args) {
    execargs_t rv = (execargs_t) malloc(sizeof(char *) * args.size());
    for (int i = 0; i < args.size(); i++)
        rv[i] = args[i];
    return rv;
}


#endif //OS_EXTRA_HELPER_H_H
