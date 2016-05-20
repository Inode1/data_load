#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include <set>

#include <benchmark/benchmark.h>

static void BM_FindInMappedFile(benchmark::State& state) {
    struct stat sb;
    char file[] = "firefox";
    int fd;
    int offset;
    int count_symbols = 0;
    int line_number = 0;
    // point to map file
    char *p;


    while (state.KeepRunning())
    {
        fd = open("test_file", O_RDONLY);
        assert(fd != -1 && "fd = open(test_file, O_RDONLY)");
        offset = fstat(fd, &sb);
        assert(offset != -1 && "fstat(fd, &sb) fail");
        p = (char *)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
        assert(p != NULL && "mmap fail");
        close(fd);
        for (offset = 0; offset != sb.st_size; ++offset)
        {
            if (p[offset] == file[count_symbols])
            {
                if (count_symbols == (sizeof(file) - 2))
                {
                    ++line_number;
                }
                ++count_symbols;
            }
            else
            {
                count_symbols = 0;
            }
        }
        offset = munmap(p, sb.st_size);
        assert(offset != -1 && "munmap not");
    }
    printf("total count %d\n", line_number);
}
// Register the function as a benchmark
BENCHMARK(BM_FindInMappedFile);

// Define another benchmark
static void BM_FindINFopenFile(benchmark::State& state) {
    FILE *fp;
    char buffer[1024];
    char file[] = "firefox";
    int line_number = 0;
    while (state.KeepRunning())
    {
        fp = fopen("test_file", "r");
        assert(fp != NULL && "File is not open.");
        while (fgets(buffer, sizeof buffer, fp))
        {
            if (strstr(buffer, file))
            {
                ++line_number;    
            }    
        }
        fclose(fp);
    }
    printf("total count %d\n", line_number);
}
BENCHMARK(BM_FindINFopenFile);

// Define another benchmark
static void BM_FindInBoost(benchmark::State& state) {
    mapped_file mapRelease("test_file");
    string line;
    while (state.KeepRunning())
    {
        stream<mapped_file> is(mapRelease, std::ios_base::binary);
        while (getline(is, line))
        {
            
        }
        fclose(fp);
    }
    printf("total count %d\n", line_number);
}
BENCHMARK(BM_FindINFopenFile);

BENCHMARK_MAIN();