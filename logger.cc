/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <syslog.h>

static const size_t nbuffer = 10240;
static char buffer[nbuffer];

static bool getopt(int argc, char *argv[], int *facility, int *level);

int main(int argc, char *argv[])
{
    int facility, level;
    getopt(argc, argv, &facility, &level);

    openlog("", 0, facility);

    while (fgets(buffer, nbuffer, stdin)) {
        syslog(level, "%.*s", (int)nbuffer, buffer);
    }

    closelog();

    return EXIT_SUCCESS;
}

struct nv_t {
    const char *name;
    int value;
};

static nv_t facilitys[] = {
    { "auth",     LOG_AUTH },
    { "authpriv", LOG_AUTHPRIV },
    { "cron",     LOG_CRON },
    { "daemon",   LOG_DAEMON },
    { "ftp",      LOG_FTP },
    { "kern",     LOG_KERN },
    { "local0",   LOG_LOCAL0 },
    { "local1",   LOG_LOCAL1 },
    { "local2",   LOG_LOCAL2 },
    { "local3",   LOG_LOCAL3 },
    { "local4",   LOG_LOCAL4 },
    { "local5",   LOG_LOCAL5 },
    { "local6",   LOG_LOCAL6 },
    { "local7",   LOG_LOCAL7 },
    { "LPR",      LOG_LPR },
    { "mail",     LOG_MAIL },
    { "news",     LOG_NEWS },
    { "syslog",   LOG_SYSLOG },
    { "user",     LOG_USER },
    { "uucp",     LOG_UUCP },
};

static nv_t levels[] = {
    { "emerg",   LOG_EMERG },
    { "alert",   LOG_ALERT },
    { "crit",    LOG_CRIT },
    { "err",     LOG_ERR },
    { "warning", LOG_WARNING },
    { "notice",  LOG_NOTICE },
    { "info",    LOG_INFO },
    { "debug",   LOG_DEBUG },
};

static bool valueOfName(const nv_t *nvs, size_t nnv, const char *name, int *value)
{
    for (size_t i = 0; i < nnv; ++i) {
        if (strcmp(nvs[i].name, name) == 0) {
            *value = nvs[i].value;
            return true;
        }
    }
    return false;
}

bool getopt(int argc, char *argv[], int *facility, int *level)
{
    *facility = LOG_USER;
    *level    = LOG_NOTICE;

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            char *pos = strchr(argv[i+1], '.');
            if (pos) *pos = '\0';
            valueOfName(facilitys, sizeof(facilitys)/sizeof(nv_t), argv[i+1], facility);
            if (pos) valueOfName(levels, sizeof(levels)/sizeof(nv_t), pos+1, level);
            if (pos) *pos = '.';
            break;
        }
    }

    return true;
}
