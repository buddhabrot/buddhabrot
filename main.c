/* Buddhabrot: fast C implementation
 * Usage: buddhabrot resolution cutoff
 *
 * Calculates NUM_DIMENSIONS stages of the buddhabrot with given resolution and cutoff.
 * Data is outputted as PNG
 *
 * @author: M. Mortier
 *
 * Optimized for 8 cores
 * Possible improvements/todo: 
 *	- comandelbrot section can be optimized by using data collision checks.
 *  - use data output to be able to work with any accuracy
 *
 * LICENSE: Use as you please.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>

#include "gd.h"

#define MAX_DISTANCE 4.0
#define NUM_DIMENSIONS 8

#define NUM_THREADS 1
#define true  1
#define false 0
	
typedef int bb_pixel;
typedef int bb_bool;
	
struct thread_data {
	int thread_id;
	int resolution;
	int cutoff;
	pthread_mutex_t *mp;
	clock_t start;
	bb_bool ** co_set;
	bb_pixel *** buddha_arr;
};
struct thread_data thread_data_array[NUM_THREADS];
struct thread_data thread_data_comandel_array[NUM_THREADS];


int generateBuddhabrot(int, int);
void * generateBuddhaSegment(void *);
void * coMandelbrotPopulator(void *);

bb_bool** coMandelbrotSet(int, int);
bb_pixel*** buddhaArray(int);
void * malloc_p(unsigned int);
int isMandelbrot(int, int, long double, long double, double,double);
void updateBuddhabrot(int cutoff, int n, int resolution,  double x,  double y, double cx, double cy, bb_pixel *** buddha_arr);

int 
main (int argc, char * const argv[]) {
	printf("** Started generating buddhabrot in multi-threaded segments.. this will take a shitload of time\n");

	int resolution, cutoff;
	if(argc != 3) {
		printf("Specify resolution and cutoff.\n");
	} else {
		resolution = atoi(argv[1]);
		cutoff = atoi(argv[2]);

		generateBuddhabrot(resolution, cutoff);
	}

	return 0;
}


int generateBuddhabrot(int resolution, int cutoff) {
	bb_bool** coMandelbrot = coMandelbrotSet(resolution, cutoff);
	bb_pixel*** buddha_arr = buddhaArray(resolution);

	int i,j;

	FILE *out;
	gdImagePtr im_out = 0;

	im_out = gdImageCreateTrueColor(2*resolution, 3*resolution);

	// create the threads
	pthread_t threads[NUM_THREADS];

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutex_t mp = PTHREAD_MUTEX_INITIALIZER; 
	pthread_mutexattr_t mattr; 
	int ret; 

	/* initialize a mutex to its default value */ 
	ret = pthread_mutex_init(&mp, NULL);

	int t;
	int rc;
	for(t = 0; t< NUM_THREADS; t++) {
		thread_data_array[t].thread_id = t;
		thread_data_array[t].co_set = coMandelbrot;
		thread_data_array[t].buddha_arr = buddha_arr;
		thread_data_array[t].resolution = resolution;
		thread_data_array[t].cutoff = cutoff;
		thread_data_array[t].start = clock();
		thread_data_array[t].mp = &mp;

		rc = pthread_create(&threads[t], &attr, generateBuddhaSegment,
				(void *) &thread_data_array[t] );
		if(rc) {
			printf("Error. Could not create a crucial worker thread.\n");
			exit(-1);
		}
	}

	pthread_attr_destroy(&attr);

	void * status;
	for(t = 0; t< NUM_THREADS; t++) {
		rc = pthread_join(threads[t], &status);
		if(rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		} 
		printf("Main: completed join with thread %d having a status of %ld\n", t, (long)status);
	}

	// populate pixel arrays
	int dim;
	for(dim = 0; dim < NUM_DIMENSIONS; dim ++) {
		bb_pixel ** buddha_arr_flat = buddha_arr[dim];
		int maxThreshold = 0;

		for(i = 0; i < 3 * resolution; i ++) {
			for(j = 0; j < 2 * resolution; j ++) {
				if(buddha_arr_flat[j][i] > maxThreshold)
					maxThreshold = buddha_arr_flat[j][i];
			}
		}


		for(i = 0; i < 3 * resolution; i ++) {
			for(j = 0; j < 2 * resolution; j ++) {
				int value = 255 * sqrt((double) buddha_arr_flat[j][i] / (double) maxThreshold);
				int color = gdTrueColor(value, value, value);
				gdImageSetPixel(im_out, j, i, color);
			}
		}

		// write the image
		char filename[1024];
		sprintf(filename, "buddha.%d.png", dim);
		out = fopen (filename, "wb");
		assert(out);
		gdImagePng (im_out, out);
		fclose (out);

		// write the data
		sprintf(filename, "buddha.%d.data", dim);
		out = fopen (filename, "wb");
		assert(out);
		for(i = 0; i < 2 * resolution; i ++) {
			fwrite(buddha_arr_flat[i], sizeof(bb_pixel), 3 * resolution, out);
		}
		fclose (out);
	}

	// free memory
	for(i = 0; i < 2 * resolution; i ++) 
		free(coMandelbrot[i]);
	free(coMandelbrot);

	for(dim = 0; dim < NUM_DIMENSIONS; dim ++) {
		for(i = 0; i < 2 * resolution; i ++) 
			free(buddha_arr[dim][i]);
		free(buddha_arr[dim]);
	}
	free(buddha_arr);

	printf("Finished writing all data. Thanks for your time.\n");
	pthread_exit(NULL);

	return 0;
}

void * generateBuddhaSegment(void * data) {
	struct thread_data *this_data;

	this_data = (struct thread_data*) data;
	pthread_mutex_t * mp = this_data->mp;

	int resolution = this_data->resolution;
	int cutoff = this_data->cutoff;
	bb_pixel *** buddha_arr = this_data->buddha_arr;
	bb_bool ** co_set = this_data->co_set;

	double cx, cy;
	int i, j;
	int start_row, end_row;
	long hits = 0;

	start_row = this_data->thread_id * (2 * resolution / NUM_THREADS);
	end_row = start_row + (2 * resolution / NUM_THREADS);

	printf("Starting thread number %d with row %d -> %d\n", this_data->thread_id, start_row, end_row);

	long max_hits = (end_row - start_row) * 3 * resolution;

	for(i = 0; i < 3 * resolution; i ++) 
		for(j = start_row; j < end_row; j ++) {
			if(!co_set[j][i]) {
				/* normalize */
				cy = (double) (j - resolution) / (double) resolution;
				cx = (double) (i - 2*resolution) / (double) resolution;
				updateBuddhabrot(cutoff, 0, resolution, 0.0, 0.0, cx, cy, buddha_arr);
				hits += 1;

				if(hits % 100000 == 0)
					printf("Thread number %d is at least at %ld percent of its work\n", this_data->thread_id, 100 * hits / max_hits);
			}
		}

	long ms = (clock() - this_data->start) / CLOCKS_PER_SEC;

	printf("Finished thread number %d in %ld s\n", this_data->thread_id, ms);

	return 0;
}

bb_bool** coMandelbrotSet(int resolution, int cutoff) {
	bb_bool** out;


	printf("Calculating the comandelbrot set with the necessary detail first..\n");

	// number of rows is 2 * resolution
	out = (bb_bool **) malloc_p (2 * resolution * sizeof (bb_bool *));

	int i;

	// number of cols is 3 * resolution
	for(i = 0; i < 2 * resolution; i ++) 
		out[i] = (bb_bool*) malloc_p(3 * resolution * sizeof (bb_bool));

	pthread_t threads[NUM_THREADS];

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutex_t mp = PTHREAD_MUTEX_INITIALIZER; 
	pthread_mutexattr_t mattr; 
	int ret; 

	/* initialize a mutex to its default value */ 
	ret = pthread_mutex_init(&mp, NULL);

	int t;
	int rc;
	for(t = 0; t< NUM_THREADS; t++) {
		thread_data_comandel_array[t].thread_id = t;
		thread_data_comandel_array[t].co_set = out;
		thread_data_comandel_array[t].buddha_arr = NULL;
		thread_data_comandel_array[t].resolution = resolution;
		thread_data_comandel_array[t].cutoff = cutoff;
		thread_data_comandel_array[t].mp = &mp;

		thread_data_comandel_array[t].start = clock();

		rc = pthread_create(&threads[t], &attr, coMandelbrotPopulator,
				(void *) &thread_data_comandel_array[t] );
		if(rc) {
			printf("Error. Could not create a crucial worker thread.\n");
			exit(-1);
		}
	}

	pthread_attr_destroy(&attr);

	void * status;
	for(t = 0; t< NUM_THREADS; t++) {
		rc = pthread_join(threads[t], &status);
		if(rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		} 
		printf("CoMandelbrot: completed join with thread %d having a status of %ld\n", t, (long)status);
	}


	printf("Ok, finished calculating comandelbrot.\n");

	return out;
}

void * coMandelbrotPopulator(void * data) {
	struct thread_data *this_data;

	this_data = (struct thread_data*) data;

	int resolution = this_data->resolution;
	int cutoff = this_data->cutoff;
	pthread_mutex_t * mp = this_data->mp;

	bb_bool ** co_set = this_data->co_set;

	int start_row, end_row;

	start_row = this_data->thread_id * (2 * resolution / NUM_THREADS);
	end_row = start_row + (2 * resolution / NUM_THREADS);

	printf("Starting comandelbrot thread number %d with row %d -> %d\n", this_data->thread_id, start_row, end_row);

	long i,j;
	double x,y;

	for(i = start_row; i < end_row; i ++) 
		for(j = 0; j < 3 * resolution; j ++) {
			y = (double) (i - resolution) / (double) resolution;
			x = (double) (j - 2*resolution) / (double) resolution;

			//pthread_mutex_lock(mp);
			co_set[i][j] = isMandelbrot(cutoff,0,0.0,0.0,x,y);
			//pthread_mutex_unlock(mp);
		}

	long ms = (clock() - this_data->start) / CLOCKS_PER_SEC;

	printf("Finished comandelbrot thread number %d in %ld s\n", this_data->thread_id, ms);
	return 0;

}

bb_pixel*** buddhaArray(int resolution) {
	bb_pixel*** out;
	int d, i, j;

	printf("Allocating memory for storage.. I will need about %ld Mb\n", 
		(long) (NUM_DIMENSIONS *  3 * 2 * resolution * resolution * sizeof(bb_pixel) / (1024 * 1024)));

	// number of dimensions is defined
	out = (bb_pixel ***) malloc_p (NUM_DIMENSIONS * sizeof (bb_pixel **));

	// number of rows is 2 * resolution
	for(i = 0; i < NUM_DIMENSIONS; i ++)  {
		out[i] = (bb_pixel**) malloc_p(2 * resolution * sizeof (bb_pixel *));

		// number of cols is 3 * resolution
		for(j = 0; j < 2 * resolution; j ++) 
			out[i][j] = (bb_pixel*) malloc_p(3 * resolution * sizeof (bb_pixel));
	}

	for(d = 0; d < NUM_DIMENSIONS; d ++)
		for(i = 0; i < 2 * resolution; i ++) 
			for(j = 0; j < 3 * resolution; j ++) 
				out[d][i][j] = 0;

	printf("Done allocating memory.\n");

	return out;
}

// Is cx + i * cy in the mandelbrot set?
int isMandelbrot(int cutoff, int n, long double x, long double y, double cx, double cy) {

	long double a = (cx - 0.25)*(cx-0.25) + cy*cy;
	long double p = sqrt(a);
	if(cx < p - 2*p*p + 0.25 || (cx + 1) * (cx + 1) + cy * cy < 1.0 / 16.0) {
		return true;
	}

	long double nx, ny, abs;
	while(n < cutoff) {
		nx = (long double) x*x - y*y + cx;
		ny = (long double) 2*x*y + cy;
		abs = (long double) nx*nx + ny*ny;

		if(abs >= MAX_DISTANCE)
			return false;

		if(abs < MAX_DISTANCE) {
			n += 1;
			x = nx;
			y = ny;
		} 
	}

	return true;
}

// Buddhabrot.. Updates buddhabrot array
void updateBuddhabrot(int cutoff, int n, int resolution, double x, double y, double cx, 
	double cy, bb_pixel *** buddha_arr) {
	
	double nx, ny;
	int dim;

	while(n < cutoff) {
		nx = x*x - y*y + cx;
		ny = 2*x*y + cy;

		int xcoo, ycoo;

		xcoo = (nx + 2.0) * resolution;
		ycoo = (ny + 1.0) * resolution;

		for(dim = 0; dim < NUM_DIMENSIONS; dim ++) {
			if(0 < xcoo && xcoo < 3 * resolution &&
				0 < ycoo && ycoo < 2 * resolution &&
				(dim == 0 || (n < cutoff >> (dim)))) {

				buddha_arr[dim][ycoo][xcoo] ++;

			}
		}

		n ++;
		x = nx;
		y = ny;
	}
}

// Just a protection built around malloc..
void* malloc_p(unsigned int size) {
	void* out = malloc(size);

	if(out == NULL) {
		printf("Oops.. out of memory!\n");
		exit(1);
	} else
		return out;
}


