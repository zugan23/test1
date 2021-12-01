#ifndef ARGPARSE
#define ARGPARSE

typedef struct tagSettings {
	char* fileName;
    char show;
    char get;
    char set;
    char* tagName;
    char* tagVal;
} args;

args getArgs(int argc, char **argv);

#endif