/*
 * Basic algorithm for buddhabrot creation
 *
 * @author: Maarten Mortier 2011
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
 
#include "gd.h" 

/* malloc helper */
void * malloc_p(unsigned int);

int size; /* W = 3*size, H = 2*size */
int depth;
int mandelbrot_depth; 
char out_dir[1024];

int width;
int height;

int mandelbrot_escape_distance = 4.0; /* Distance at which a point can no longer be in the mandelbrot. */

/* Pixel arrays (conveniently mapped in two dimensions) */
char ** mandelbrot; /* 0 if not part of mandelbrot */
struct coo {
	double x;
	double y;
};
struct coo ** map; /* Maps real coordinates to screen coordinates. */
int ** buddhabrot; /* Number of hits for each buddhabrot pixel */

/* State-keeping */
int amplitude = 0; /* Amplitude of the buddha: max. number of hits in one pixel. */
long long calculations; /* Number of calculations performed for buddhabrot. */
long long max_calculations; 

/* Monitor thread */
pthread_t monitor;
int finished;

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
	while(n < mandelbrot_depth) {
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

void process_buddhabrot(double x, double y) {
	int n = 0;
	double nx, ny;
	double ox, oy;
	int col, row;

	ox = x;
	oy = y;
	while(n < depth) {
		nx = ox*ox - oy*oy + x;
		ny = 2*ox*oy + y;

		col = (nx + 2.0) * size;
		row = (ny + 1.0) * size;

		if(col >= 0.0 && row >= 0.0 && col < width && row < height) {
			buddhabrot[col][row] ++;
		}

		ox = nx;
		oy = ny;
		n++;
		calculations++;
	}
}

void make_buddhabrot() {
	int row, col;
	double x, y;

	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			if(mandelbrot[col][row])
				continue;

			x = map[col][row].x;
			y = map[col][row].y;

			process_buddhabrot(x, y);
		}
	}

	/* Calculate amplitude. */
	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			if(amplitude < buddhabrot[col][row])
				amplitude = buddhabrot[col][row];
		}
	}

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
	int row, col;
	gdImagePtr image;
	FILE* file;
	char filename[1024] = {0};

	printf("%s\n", __FUNCTION__);

	image = gdImageCreateTrueColor(height, width);

	int color;
	int intensity = 0;

	if(amplitude > 0) {
		for(col=0; col<width; col++) {
			for(row=0; row<height; row++) {
				char idx = (char) buddhabrot[col][row];
				
				intensity = (255 * idx) / amplitude;
				color = gdTrueColor(intensity, intensity, intensity);
				gdImageSetPixel(image, row, col, color);
			}
		}
	}

	strcat(filename, out_dir);
	strcat(filename, "/buddhabrot.png");

	printf("%s\n", filename);
	file = fopen(filename, "wb");
	assert(file);
	gdImagePng(image, file);
	fclose(file);
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

void * monitor_loop(void* data) {

	int completeness;

	assert(max_calculations > 0);
	while(!finished) {
		sleep(1);
		completeness = 100 * calculations / max_calculations;
		printf("%.2d%% complete..\n", completeness);
	}

	return NULL;
}

void start_monitor_thread() {
	
	int error;
	pthread_attr_t attr;

	printf("%s\n", __FUNCTION__);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	error = pthread_create(&monitor, &attr, monitor_loop, (void*) NULL);
	assert(!error);

	pthread_attr_destroy(&attr);

}

void make_time_estimate() {
	int col, row;
	int anti_mandelbrot_size = 0;

	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			if(!mandelbrot[col][row])
				anti_mandelbrot_size++;
		}
	}

	max_calculations = anti_mandelbrot_size * depth;
}

int main(int argc, char* argv[]) {
	assert(argc >= 4);

	size = atoi(argv[1]);
	depth = atoi(argv[2]);
	strcpy(out_dir, argv[3]);

	int mandelbrot_detail = 1;
	if(argc >= 5) {
		mandelbrot_detail = atoi(argv[4]);
	}

	/* The mandelbrot can be calculated a lot deeper than the buddhabrot
	   because it is cheaper and this produces finer results. */
	mandelbrot_depth = depth * mandelbrot_detail;

	width = 3 * size;
	height = 2 * size;


	finished = 0;

	allocate_sets();
	make_map();

	/* Create the mandelbrot. */
	make_mandelbrot();
	write_mandelbrot();

	/* Based on size of mandelbrot: */
	make_time_estimate();
	start_monitor_thread();

	/* Create the buddhabrot */
	make_buddhabrot();
	write_buddhabrot();

	printf("Finished.\n");
	finished = 1;
	return 0;
}

void* malloc_p(unsigned int size) {
	void* out = calloc(size, 1);

	if(out == NULL) {
		printf("Oops.. out of memory!\n");
		exit(1);
	} else
		return out;
}


