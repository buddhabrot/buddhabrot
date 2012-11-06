#include <string>
#include <iostream>
#include <unistd.h>

struct BrotOptions
{
    int w,
    int h,
    int m,
    int d
};

const struct BrotOptions DEFAULT_OPTIONS = { 
    256, 256, 64, 64
};

void display_usage()
{
    cout << "brot [-m {mandelbrot_depth}] [-w {width}] [-h {height}]" << endl;
    cout << "mandelbrot_depth: calculation depth for anti-mandelbrot region" << endl;
    cout << "depth: calculation depth for buddhabrot paths" << endl;
    cout << "width: width of buddhabrot in pixels" << endl;
    cout << "height: height of buddhabrot in pixels" << endl;
}

const struct BrotOptions parse_options(int argc, char* argv[])
{
    struct BrotOptions options = DEFAULT_OPTIONS;
    int opt;

    while((opt = getopt(argc, argv, "d:w:h:")) != -1)
    {
        switch(opt)
        {
            case 'w':
                options.w = atoi(optarg);
                break;
            case 'h':
                options.h = atoi(optarg);
                break;
            case 'd':
                options.d = atoi(optarg);
                break;
            case 'm':
                options.m = atoi(optarg);
                break;
            case '?':
                display_usage();
                break;
        }
    }

    return options;
}

int main(int argc, char* argv[])
{
    Options options = parse_options(argc, argv); // may cause exit

}

