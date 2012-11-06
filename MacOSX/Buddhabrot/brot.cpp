#include <ostream>

bool is_mandelbrot_point(double x, double y, double depth)
{
	int n = 0;
	double nx, ny; /* next iteration */
	double ox, oy; /* previous iteration, or (0.0, 0.0) */
	double a, p, dst; /* helpers */
    
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
        
		if(dst > mandelbrot_escape_distance)
			return 0; /* point escaped */
        
		ox = nx;
		oy = ny;
		n++;
	}
    
	return 1;
}