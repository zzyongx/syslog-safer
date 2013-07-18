/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <ringbuffer.h>
#include <logreader.h>
#include <logwriter.h>

/* g++ -g -Wall syslog-safer.cc ringbuffer.cc -I. -lpthread -o syslog-safer
 */

struct config_t {
    const char *source;
    const char *dest;
    const char *pidfile;
    const char *notifyf;
    bool        verbose;
    bool        stream;
    bool        daemonize;
    size_t      bsize;
};

LogReader<RingBuffer> *logr;
LogWriter<RingBuffer> *logw;

int usage(const char *error = 0)
{
    if (error) printf("%s\n", error);
    printf("usage: syslog-safer -s source -d dest -b buffer -p pidfile [-D]\n\n"
           "   if syslogd(or something like) is blocked, the system who use syslog will be hanged up,\n"
           "   syslog-safer copy from source(usually /dev/log) to dest, never blocked by dest,\n"
           "   it read source as fast as possible, stor the content in buffer first, and then write to dest,\n"
           "   if buffer is full, drop the oldest data\n\n"
           "   -s source, default is /dev/log\n"
           "   -d dest, you must appoint, for example /dev/xlog\n"
           "   -t stream|dgram, default dgram\n"
           "   -p pidifle, default /var/run/syslog-safer.pid\n"
           "   -b buffer, default is 128M, you cant use(K/M/G) unit\n"
           "   -D default no daemonize\n"
           "   -n notify file, default no\n"
           "   -h show this help screen\n");
    return error == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

void getoption(int argc, char *argv[], config_t *config)
{
    config->source    = "/dev/log";
    config->dest      = 0;
    config->stream    = false;
    config->pidfile   = "/var/run/syslog-safer.pid";
    config->notifyf   = 0;
    config->bsize     = 128 * 1024 * 1024;
    config->verbose   = false;
    config->daemonize = false;

    opterr = 0;

    int c;
    while ((c = getopt(argc, argv, "s:d:t:p:n:b:Dvh")) != -1) {
        switch (c) {
            case 's': config->source  = optarg; break;
            case 'd': config->dest    = optarg; break;
            case 't': config->stream  = (strcmp(optarg, "stream") == 0); break;
            case 'p': config->pidfile = optarg; break;
            case 'n': config->notifyf = optarg; break;
            case 'b': {
                char *endptr;
                config->bsize = strtoul(optarg, &endptr, 10);
                if (endptr[0] == 'G' || endptr[0] == 'g') {
                    config->bsize *= 1024 * 1024 * 1024;
                } else if (endptr[0] == 'M' || endptr[0] == 'm') {
                    config->bsize *= 1024 * 1024;
                } else if (endptr[0] == 'K' || endptr[0] == 'k') {
                    config->bsize *= 1024;
                }
                break;
            }
            case 'D': config->daemonize = true; break;
            case 'v': config->verbose = true; break;
            case 'h': exit(usage()); break;
            default: exit(usage("unknow option"));
        }
    }

    if (config->dest == 0) exit(usage("you must appoint -d"));
    if (config->bsize < 8 * 1024 * 1024) exit(usage("-b at least 8M"));
}

void *logwRoutine(void *data)
{
    LogWriter<RingBuffer> *logw = (LogWriter<RingBuffer> *) data;
    logw->run();
    return 0;
}

bool startWriteThread(pthread_t *tid, LogWriter<RingBuffer> *logw)
{
    int eno = pthread_create(tid, 0, logwRoutine, logw);
    if (eno != 0) {
        fprintf(stderr, "pthread_create() error, %d:%s\n", eno, strerror(eno));
        return false;
    }
    return true;
}

bool stopWriteThread(pthread_t *tid, RingBuffer *rbuffer)
{
    logw->stop();
    rbuffer->interrupt();
    pthread_join(*tid, 0);
    return true;
}

void sigHandler(int signo)
{
    if (signo == SIGTERM) {
        logr->stop();
    }
}

bool writePidfile(const char *pidfile)
{
    FILE *fp = fopen(pidfile, "w");
    if (fp) {
        fprintf(fp, "%d", getpid());
        fclose(fp);
    }

    return fp != 0;
}

int main(int argc, char *argv[])
{
    config_t config;
    getoption(argc, argv, &config);

    if (config.daemonize) daemon(1, 1);
    writePidfile(config.pidfile);

    signal(SIGTERM, sigHandler);

    RingBuffer rbuffer(config.bsize, config.verbose, config.notifyf);
    LogReader<RingBuffer> logr(config.source, &rbuffer, config.stream);
    LogWriter<RingBuffer> logw(config.dest, &rbuffer, config.stream);

    pthread_t tid;
    if (!startWriteThread(&tid, &logw)) {
        fprintf(stderr, "can't start write thread, %d:%s\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    if (!logr.run()) {
        stopWriteThread(&tid, &rbuffer);
        return EXIT_FAILURE;
    }

    stopWriteThread(&tid, &rbuffer);
    return EXIT_SUCCESS;
}
