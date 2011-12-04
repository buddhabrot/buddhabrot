/*
 * Basic algorithm for buddhabrot creation.
 *
 * LICENSE: Use as you see fit, but give credit where you think it's due.
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
#include <gd.h> 

#include "cpu_info.h"

#define MULTITHREADED 1
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

/* Monitor thread */
pthread_t monitor;
int finished;
int total_points;

/* Multi-threading */
int num_threads;
struct thread_data {
	int id;
	int startpoint; /* Index in coordinate space where thread starts its work. */
	int points_processed;
	int points_to_process;
	int finished;
};
int points_per_thread;
struct thread_data* thread_data_set;
pthread_t *threads;
pthread_mutex_t ** mutexes;


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

		nx = (ox+oy)*(ox-oy) + x;
		ny = 2*ox*oy + y;

		if(nx >= -2 && ny >= -1 && nx < 1 && ny < 1 ) {
			col = (nx + 2) * size;
			row = (ny + 1) * size;

			pthread_mutex_lock(&mutexes[col][row]);
			buddhabrot[col][row] ++;
			pthread_mutex_unlock(&mutexes[col][row]);
		}

		ox = nx;
		oy = ny;
		n++;
	}
}

void post_processing() {

	int row, col;

	/* Calculate amplitude. */
	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			if(amplitude < buddhabrot[col][row])
				amplitude = buddhabrot[col][row];
		}
	}

}

void* make_buddhabrot(void * data) {
	int row, col;
	double x, y;
	int startpoint;
	int id;

	struct thread_data* thread_data = (struct thread_data*) data;
	id = thread_data->id;
	startpoint = thread_data->startpoint;

	printf("Starting thread %d at index %d.\n", id, startpoint);

	thread_data->points_processed = 0;

	int i = 0;
	for(col=0; col<width; col++) {
		for(row=0; row<height; row++, i++) {
			if(i < startpoint)
				continue;

			if(mandelbrot[col][row])
				continue;

			x = map[col][row].x;
			y = map[col][row].y;

			process_buddhabrot(x, y);
			thread_data->points_processed ++;

			if(thread_data->points_processed == thread_data->points_to_process) {
				thread_data->finished = 1;
				break;
			}
			
		}

		if(thread_data->finished)
			break;
	}

	/* Thread is finished. */
	thread_data->finished = 1;

	return NULL;
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
	double intensity = 0.0;

	if(amplitude > 0) {
		for(col=0; col<width; col++) {
			for(row=0; row<height; row++) {
				char idx = (char) buddhabrot[col][row];
				
				intensity = sqrt((double) idx / (double) amplitude);
				intensity *= 255; /* squared, and capped to 0, 255 */
				color = gdTrueColor((int) intensity, (int) intensity, (int) intensity);
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
	int thread_no;
	int seconds;

	if(depth > 10000)
		seconds = 600;
	else
		seconds = 2;

	while(!finished) {	
		finished = 1;

		completeness = 0;
		for(thread_no=0; thread_no<num_threads; thread_no++) {
			finished &= thread_data_set[thread_no].finished;
			completeness += 100 * thread_data_set[thread_no].points_processed;
		}
		completeness /= total_points;
		printf("%.2d%% complete..\n", completeness);

		sleep(seconds);
	}

	return NULL;
}

void wait_monitor_thread() {
	
	int error;
	void* status;
	pthread_attr_t attr;

	printf("%s\n", __FUNCTION__);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	error = pthread_create(&monitor, &attr, monitor_loop, (void*) NULL);
	assert(!error);

	pthread_attr_destroy(&attr);
	error = pthread_join(monitor, &status);
	assert(!error);

}

/* Schedules the calculation for the different threads. */
void make_schedule() {

	int col, row;
	int anti_mandelbrot_size = 0;

	/* Determine anti-mandelbrot (points not in mandelbrot) set size. */
	for(col=0; col<width; col++) {
		for(row=0; row<height; row++) {
			if(!mandelbrot[col][row])
				anti_mandelbrot_size++;
		}
	}

	num_threads = get_num_cores(); /* Thread per core. */
	if(num_threads > 1 && MULTITHREADED) {
		points_per_thread = anti_mandelbrot_size / num_threads;
		thread_data_set = (struct thread_data*) 
			malloc_p(sizeof(struct thread_data) * num_threads);
		threads = (pthread_t*) malloc(sizeof(pthread_t) * num_threads);

		/* Give a slice of the mandelbrot point area to each thread. */
		int points = 0;
		int thread_no = 0;
		int i = 0;
		for(col=0; col<width; col++) {
			for(row=0; row<height; row++, i++) {
				if(mandelbrot[col][row])
					continue;

				if(points%points_per_thread == 0) {
					if(thread_no < num_threads) {
						thread_data_set[thread_no].id = thread_no;
						thread_data_set[thread_no].startpoint = i;
						thread_data_set[thread_no].points_to_process = points_per_thread;
						thread_no++;
					}
				}
				points ++;
			}
		}

		/* Pad the last thread so it finishes up. */
		thread_data_set[num_threads - 1].points_to_process += anti_mandelbrot_size % num_threads;
	}

	/* Allocate and initialize the mutexes. */
	mutexes = (pthread_mutex_t**) malloc(sizeof(pthread_mutex_t*) * width);
	for(col=0; col<width; col++)
		mutexes[col] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * height);
	for(col=0; col<width; col++)
		for(row=0; row<height; row++)
			pthread_mutex_init(&mutexes[col][row], NULL);

	total_points = anti_mandelbrot_size;

}

void create_buddhabrot_thread(int thread_no) {
	int error;
	pthread_attr_t attr;

	assert(thread_no < num_threads);
	printf("%s\n", __FUNCTION__);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	thread_data_set[thread_no].finished = 0;
	thread_data_set[thread_no].points_processed = 0;

	error = pthread_create(&threads[thread_no], &attr, make_buddhabrot, 
		(void*) &thread_data_set[thread_no]);
	assert(!error);

	pthread_attr_destroy(&attr);	
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
	make_schedule();

	/* Create the buddhabrot (multi-threaded) */
	if(num_threads == 1 || MULTITHREADED == 0) {
		make_buddhabrot(NULL);
	} else {
		int thread_no;
		for(thread_no = 0; thread_no < num_threads; thread_no++) {
			create_buddhabrot_thread(thread_no);
		}
	}

	wait_monitor_thread(); /* blocks current thread */
	post_processing();
	write_buddhabrot();

	printf("Finished.\n");
 
	free(threads);
	free(thread_data_set);
	/* todo: free the point sets. But heap is destroyed anyway. */

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


