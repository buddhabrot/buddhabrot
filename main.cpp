#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/mman.h>

#include "brot.h"

using namespace std;

const BrotOptions DEFAULT_OPTIONS = { 
    256 /* w */, 256 /* h */, 64 /* m */, 64 /* d */
};

void display_usage()
{
    cout << "brot [-m {mandelbrot_depth}] [-w {width}] [-h {height}]" << endl;
    cout << "mandelbrot_depth: calculation depth for anti-mandelbrot region" << endl;
    cout << "depth: calculation depth for buddhabrot paths" << endl;
    cout << "width: width of buddhabrot in pixels" << endl;
    cout << "height: height of buddhabrot in pixels" << endl;
}

const BrotOptions parse_options(int argc, char* argv[])
{
    BrotOptions options = DEFAULT_OPTIONS;
    int opt;

    while((opt = getopt(argc, argv, "d:m:w:h:")) != -1)
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
                exit(1);
                break;
        }
    }

    return options;
}

int main(int argc, char* argv[])
{
    BrotOptions options = parse_options(argc, argv); // may cause exit
    
    cout << "Running buddhabrot.." << endl << "width:\t\t" << options.w << endl << "height:\t\t" << options.h << endl
        << "depth:\t\t" << options.d << endl << "m.depth:\t" << options.m << endl << endl;
    
    PointList *points;
    if(!(points = anti_mandelbrot(options)))
    {
        cout << "Could not create a mandelbrot file. Exiting." << endl;
        exit(1);
    }
    
    string debug = "out.png";
    visualize_mandelbrot(points, debug, options);
}

