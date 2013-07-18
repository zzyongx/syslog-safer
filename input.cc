/* vim:expandtab:shiftwidth=4:tabstop=4:smarttab:
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>

#include <logwriter.h>

/* g++ -g -Wall input.cc -I. -o input
 */

class InputFile {
public:
    InputFile(const char *file);
    ~InputFile();

    size_t read(char *buffer, size_t n);
    static const size_t nbuffer = 81920 + 30;

private:
    FILE *fp_;
};

InputFile::InputFile(const char *file)
{
    fp_ = fopen(file, "r");
    if (!fp_) throw errno;
}

InputFile::~InputFile()
{
    fclose(fp_);
}

size_t InputFile::read(char *buffer, size_t n)
{
    char *line = fgets(buffer, n, fp_);
    if (!line) {
        sleep(10);
        rewind(fp_);
        line = fgets(buffer, n, fp_);
    }
    return line ? strlen(line) : 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s srcfile destsock stream|dgram\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src  = argv[1];
    const char *dest = argv[2];
    bool isStream = (strcmp(argv[3], "stream") == 0);

    InputFile input(src);
    LogWriter<InputFile> logw(dest, &input, isStream);

    logw.run();
    return EXIT_SUCCESS;
}
