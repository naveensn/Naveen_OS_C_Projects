
#include "bitmap.h"
#include <pthread.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#define MAX_NUM_THREADS 100    //Max number of threads we can create.
                               //if you need more threads increase this.

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void *compute_image( void *threadarg );

// information we are sending to each thread for image calculation.
struct image_thread_data
{
    struct bitmap *ibm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    int height;
    int width;
    int iterations;
    int part_height;
    int remain_height;
};

pthread_mutex_t mutex;

void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>     The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>   X coordinate of image center point. (default=0)\n");
	printf("-y <coord>   Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>   Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels>  Width of the image in pixels. (default=500)\n");
	printf("-H <pixels>  Height of the image in pixels. (default=500)\n");
	printf("-o <file>    Set output file. (default=mandel.bmp)\n");
	printf("-n <threads> Number of threads created to compute the image (default = 1)\n");
	printf("-h           Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int    num_of_threads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:n:o:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
            case 'n':
				num_of_threads = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	if (num_of_threads < 1 || num_of_threads > MAX_NUM_THREADS)
    {
        printf("Number of threads can only be between 1 to %d\n", MAX_NUM_THREADS);
        num_of_threads = 1;
    }
	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d num_of_threads=%d outfile=%s\n",xcenter,ycenter,scale,max,num_of_threads,outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// sending below data to a all threads to create its portion of the image
	struct image_thread_data part_image;
	part_image.ibm = bm;
    part_image.xmin = xcenter-scale;
    part_image.xmax = xcenter+scale;
    part_image.ymin = ycenter-scale;
    part_image.ymax = ycenter+scale;
    part_image.width = image_width;
    part_image.height = image_height;
    part_image.iterations = max;
    part_image.part_height = image_height/num_of_threads;  // height of portion of the image each thread creates.
    part_image.remain_height = image_height%num_of_threads; // reminder height after dividing image between each threads.

	pthread_t thread_id[MAX_NUM_THREADS];

    pthread_mutex_init( & mutex, NULL );

	int i;
    // Creates user specified number of threads to generate the image.
	for (i=0;i<num_of_threads;i++)
    {
        if(pthread_create(&thread_id[i], NULL, compute_image, (void *) &part_image))
        {
            perror("Error creating thread: ");
            exit( EXIT_FAILURE );
        }
    }
    // waiting in the loop for all threads to complete.
    for (i=0;i<num_of_threads;i++)
    {
        if(pthread_join(thread_id[i], NULL))
        {
            perror("Problem with pthread_join: ");
        }
    }

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile))
    {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

// Compute the Mandelbrot image
void *compute_image( void *threadarg )
{
    // this is the critical region in the program.
    // giving thread number for each thread starting from 1
    pthread_mutex_lock( &mutex );
        int static num_time_in_this_func;
        num_time_in_this_func++;
        int thread_num = num_time_in_this_func;
    pthread_mutex_unlock( &mutex );

    struct image_thread_data *image_data;
    image_data = (struct image_thread_data *) threadarg;

	// distributing remaining image after equal division to first few threads.
	int i,j,extra_start_j,extra_end_j;
    if (image_data->remain_height < thread_num)
    {
        extra_start_j = image_data->remain_height;
        extra_end_j = image_data->remain_height;
    }
    else
    {
        extra_start_j = thread_num - 1;
        extra_end_j = thread_num;
    }

	// For every pixel in the image portion of this thread...
	for(j = (image_data->part_height * (thread_num - 1) + extra_start_j);
	    j < image_data->part_height * thread_num + extra_end_j ;j++)
    {
		for(i=0;i<image_data->width;i++)
        {
			// Determine the point in x,y space for that pixel.
			double x = image_data->xmin + i*(image_data->xmax-image_data->xmin)/image_data->width;
			double y = image_data->ymin + j*(image_data->ymax-image_data->ymin)/image_data->height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,image_data->iterations);

			// Set the pixel in the bitmap.
			bitmap_set(image_data->ibm,i,j,iters);
		}
	}
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}
