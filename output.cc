/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>

#include <logreader.h>

/* g++ -g -Wall output.cc -I. -o output
 */

class OutputFile {
public:
    OutputFile(const char *file);
    ~OutputFile();

    size_t write(const char *buffer, size_t n);
    static const size_t nbuffer = 81920 + 30;

private:
    FILE *fp_;
};

OutputFile::OutputFile(const char *file)
{
    fp_ = fopen(file, "w");
    if (!fp_) throw errno;
    setvbuf(fp_, (char *)NULL, _IOLBF, 0);
}

OutputFile::~OutputFile()
{
    fclose(fp_);
}

size_t OutputFile::write(const char *buffer, size_t n)
{
    return fwrite(buffer, 1, n, fp_);
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s srcsock destfile stream|dgram\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src  = argv[1];
    const char *dest = argv[2];
    bool isStream = (strcmp(argv[3], "stream") == 0);

    OutputFile output(dest);
    LogReader<OutputFile> logr(src, &output, isStream);

    logr.run();
    return EXIT_SUCCESS;
}
