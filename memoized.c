/*
 * Memoized algorithm for buddhabrot creation
 *
 * @author: Maarten Mortier 2011
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "gd.h"

/* malloc helper */
void * malloc_p(unsigned int);

static int size; /* W = 3*size, H = 2*size */
static int depth;
static char out_dir[1024];

int width;
int height;

static int mandelbrot_escape_distance = 4.0; /* Distance at which a point can no longer be in the mandelbrot. */

/* Pixel arrays (conveniently mapped in two dimensions) */
char ** mandelbrot; /* 0 if not part of mandelbrot */
struct coo {
	double x;
	double y;
};
struct coo ** map;
int ** buddhabrot; /* Number of hits for each buddhabrot pixel */

void make_map() {
	/* Makes a normalized map (x = -2.0 .. 1.0, y = -1.0 .. 1.0) */
	int col, row;

	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			map[col][row].x = (double) (col - 2 * size) / (double) size;
			map[col][row].y = (double) (row - size) / (double) size;
		}
	}
}

char is_mandelbrot_point(double x, double y) {

	int n = 0;
	double nx, ny; /* next iteration */
	double ox, oy; /* previous iteration, or (0.0, 0.0) */
	double a, p, dst; /* helpers */

	a = (x-0.25)*(x-0.25) + y*y;
	p = sqrt(a);
	
	/* premature exit: center of mandelbrot */
	if(x < p - 2*p*p + 0.25 || (x + 1) * (x + 1) + y * y < 1.0 / 16.0) {
		return 1;
	}

	ox = 0.0;
	oy = 0.0;
	while(n < depth) {
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

/* Calculate the mandelbrot for given depth. */
void make_mandelbrot() {
	int row, col;
	double x, y;

	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			x = map[col][row].x;
			y = map[col][row].y;

			mandelbrot[col][row] = is_mandelbrot_point(x, y);
		}
	}
}

void make_buddhabrot() {

}

void write_mandelbrot() {
	int row, col;
	gdImagePtr image;
	FILE* file;
	char filename[1024] = {0};

	image = gdImageCreateTrueColor(width, height);

	int black = gdTrueColor(0, 0, 0);
	int white = gdTrueColor(255, 255, 255);
	int color;

	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			if(mandelbrot[col][row])
				color = black;
			else
				color = white;

			gdImageSetPixel(image, col, row, color);
		}
	}
	
	strcat(filename, out_dir);
	strcat(filename, "/mandelbrot.png");

	printf("%s\n", filename);
	file = fopen(filename, "wb");
	assert(file);
	gdImagePng(image, file);
	fclose(file);
}

void write_buddhabrot() {

}

void allocate_sets() {

	printf("%s\n", __FUNCTION__);
	int col;

	mandelbrot = (char**) malloc_p(width * sizeof(char*));
	buddhabrot = (int**) malloc_p(width * sizeof(int*));
	map = (struct coo**) malloc_p(width * sizeof(struct coo*));

	for(col=0; col<width; col++) {
		mandelbrot[col] = (char*) malloc_p(height);
		buddhabrot[col] = (int*) malloc_p(height * sizeof(int));
		map[col] = (struct coo*) malloc_p(height * sizeof(struct coo));
	}
}

int main(int argc, char* argv[]) {
	assert(argc >= 4);

	size = atoi(argv[1]);
	depth = atoi(argv[2]);
	strcpy(out_dir, argv[3]);

	width = 3 * size;
	height = 2 * size;

	allocate_sets();
	make_map();
	make_mandelbrot();

	write_mandelbrot();

	make_buddhabrot();
	
	write_buddhabrot();

	return 0;
}

void* malloc_p(unsigned int size) {
	void* out = malloc(size);

	if(out == NULL) {
		printf("Oops.. out of memory!\n");
		exit(1);
	} else
		return out;
}


