/* Increment these if introducing backwards incompatible things.
   They are used to break cache files. */
#define MANDELBROT_ALGORITHM_VERSION 1
#define BUDDHABROT_ALGORITHM_VERSION 1

#include <xreal.h>

using namespace HPA;

typedef struct BrotOptions
{
    unsigned int w;
    unsigned int h;
    unsigned int m;
    unsigned int d;
} BrotOptions;

class PointList{
public:
    PointList(void) { x=0.0; y=0.0; next=NULL; };
    xreal x;
    xreal y;
    struct PointList* next;
};

PointList *anti_mandelbrot(const BrotOptions& options);
void visualize_mandelbrot(PointList* point, const string& path, const BrotOptions& options);
