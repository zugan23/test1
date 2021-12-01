#include "argparse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

args getArgs(int argc, char** argv) {
    args res;

    res.fileName = "1.mp3";
    res.show = 0;
    res.get = 0;
    res.set = 0;


    for (int i = 1; i < argc; i++) {
        if (!strcmp("--filename", argv[i])) {
            res.fileName = argv[++i];
        }
        else if (!strcmp("--show", argv[i])) {
            res.show = 1;
        }
        else if (!strcmp("--get", argv[i])) {
            res.get = 1;
            res.tagName = argv[++i];
        }
        else if (!strcmp("--set", argv[i])) {
            res.set = 1;
            res.tagName = argv[++i];
            res.tagVal = argv[++i];
        }
    }

    return res;
}