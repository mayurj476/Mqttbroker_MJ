#pragma once
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;

#undef FunctionName
#define FunctionName Reporter(__FUNCTION__, __FILE__, __LINE__)

enum class LEVEL
{
    INFO,
    DEBUG,
    VERBOSE,
    WARNING
};

class Logger
{

private:
    static const char *getlable(const LEVEL level)
    {
        const char *lable = "";
        switch (level)
        {
        case LEVEL::INFO:
            lable = "INFO";
            break;
        case LEVEL::DEBUG:
            lable = "DEBUG";
            break;
        case LEVEL::VERBOSE:
            lable = "VERBOSE";
            break;
        case LEVEL::WARNING:
            lable = "WARNING";
            break;

        default:
            lable = "VERBOSE"; // set default log level as verbose
            break;
        }
        return lable;
    }

    Logger();
    ~Logger();

    Logger(const Logger &);
    Logger &operator=(const Logger &);

public:
    template <typename... T>
    static void log(LEVEL level, const char *message, T... arg)
    {
        const char *lable = getlable(level);
        FILE *fp = fopen("Logs.txt", "a");
        if (!fp)
        {
            perror("File open failed");
            fclose(fp);
            return;
        }
        cout << __TIMESTAMP__ << " ";
        printf("[%s] ", lable);
        fprintf(fp, "[%s] ", lable);

        fprintf(fp, " %s  ", __TIMESTAMP__);
        printf(message, arg...);
        printf("\n");
        fprintf(fp, message, arg...);
        fprintf(fp, "\n");

        fclose(fp);
    }
};
