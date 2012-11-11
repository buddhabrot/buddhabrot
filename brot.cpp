#include <ostream>
#include <math.h>
#include <xreal.h>
#include <fstream>
#include "CImg.h"

#include "brot.h"

using namespace HPA;
using namespace cimg_library;
using namespace std;

/* Private prototypes */

bool is_mandelbrot_point(xreal x, xreal y, int depth);
int write_points(PointList* points, const string& path);
PointList *read_points(const string& path);


/* Public implementations */

PointList *anti_mandelbrot(const BrotOptions& options)
{
    int w = options.w;
    int h = options.h;
    unsigned int m = options.m;
    
    PointList *start = NULL;
    PointList *list = NULL;
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d_%d_%d.mb", w, h, m);
    
    if(ifstream(buffer))
    {
        cout << "Reading mandelbrot from cache file [" << buffer << "]" << endl;
        return read_points(string(buffer));
    }
    
    //
    /* The w/h parameters are interpreted as: [-2*w, +w] and [-h, +h] */
    for(int pixels_x = -2*w; pixels_x < w; pixels_x ++)
    {
        for(int pixels_y = -h; pixels_y < h; pixels_y++)
        {
            xreal x = (xreal) pixels_x / (xreal) w;
            xreal y = (xreal) pixels_y / (xreal) h;
            
            if(is_mandelbrot_point(x, y, m))
            {
                continue;
            }
            else
            {
                if(!list)
                {
                    start = list = new PointList();
                }
                else
                {
                    list->next = new PointList();
                    list = list->next;
                }
                list->x = x;
                list->y = y;
            }
        }
    }
    
    cout << "Writing mandelbrot to cache file [" << buffer << "]" << endl;
    write_points(start, buffer);
    
    return start;
}

void visualize_mandelbrot(PointList* point, const string& path, const BrotOptions& options)
{
    CImg<unsigned char> img(options.w * 3, options.h * 2,1,3);
    img.fill(0);
    
    unsigned int w = options.w;
    unsigned int h = options.h;
    
    PointList *list = point;
    
    unsigned char white[] = {255, 255, 255};
    while(list)
    {
        xreal x = list->x;
        xreal y = list->y;
        double px = x._2double() * w + 2*w;
        double py = y._2double() * h + h;
        
        img.draw_point((int) px, (int) py, white);
        
        list = list->next;
    }
    
    img.save_png(path.c_str());
}

int write_points(PointList* points, const string& path)
{
    if(points)
    {
        ofstream ofile(path.c_str());
        if(!ofile.fail())
        {
            while(points)
            { 
                points->x.print(ofile, 0, 1, 7);
                points->x.print(ofile, 0, 1, 7);
                
                points = points->next;
            }
            
            ofile.close();
            return 1;
        }
    }
    
    return 0;
}

PointList *read_points(const string& path)
{
    ifstream file(path.c_str());
    
    PointList *start = NULL;
    PointList *list = NULL;
    
    xreal v;
    
    if(!file.fail())
    {
        while(!file.eof())
        {
            if(!list)
            {
                start = list = new PointList();
            }
            else
            {
                list->next = new PointList();
                list = list->next;
            }
            
            v.getfrom(file);
            list->x = v;
            v.getfrom(file);
            list->y = v;
        }
        
        file.close();
        return start;
    }
    
    return NULL;
}

/* Private implementations */

bool is_mandelbrot_point(xreal x, xreal y, int depth)
{
	int n = 0;
	xreal nx, ny; /* next iteration */
	xreal ox, oy; /* previous iteration, or (0.0, 0.0) */
	xreal a, p, dst; /* helpers */
    
	a = (x-0.25)*(x-0.25) + y*y;
	p = sqrt(a);
	
	/* premature exit: the center "holes" of mandelbrot */
	if(x < p - 2*p*p + 0.25 || (x + 1) * (x + 1) + y * y < 1.0 / 16.0)
    {
		return 1;
	}
    
	ox = 0.0;
	oy = 0.0;
	while(n < depth)
    {
		nx = ox*ox - oy*oy + x;
		ny = 2*ox*oy + y;
		dst = nx*nx + ny*ny;
        
		if(dst > 4.0)
			return 0; /* point escaped */
        
		ox = nx;
		oy = ny;
		n++;
	}
    
	return 1;
}

