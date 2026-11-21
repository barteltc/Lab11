///
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//
///
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "jpegrw.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

// local routines
static int iteration_to_color(int i, int max);
static int iterations_at_point(double x, double y, int max);
static void compute_image(imgRawImage *img, double xmin, double xmax,
						  double ymin, double ymax, int max);
static void show_help();

int main(int argc, char *argv[])
{
	int c;
	int processes = 1;
	// These are the default configuration values used
	// These are the default configuration values used
	// if no command line arguments are given.
	const char *outfile = "mandel.jpg";
	double xcenter = 0;
	double ycenter = 0;
	double xscale = 4;
	double yscale = 0; // calc later
	int image_width = 3840;
	int image_height = 2160;
	int max = 10000;

	// For each command line argument given,
	// override the appropriate configuration value.

	while ((c = getopt(argc, argv, "x:y:s:W:H:m:o:p:h")) != -1)
	{
		switch (c)
		{
		case 'x':
			xcenter = atof(optarg);
			break;
		case 'y':
			ycenter = atof(optarg);
			break;
		case 's':
			xscale = atof(optarg);
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
		case 'o':
			outfile = optarg;
			break;
		case 'h':
			show_help();
			exit(1);
			break;
		case 'p':
			processes = atoi(optarg);
			if (processes < 1)
			{
				fprintf(stderr, "Process count must be positive\n");
				return 1;
			}
			break;
		}
	}
	// Display configuration.
	yscale = xscale / image_width * image_height;
	printf("mandel: x=%lf y=%lf xscale=%lf yscale=%lf max=%d outfile=%s\n",
		   xcenter, ycenter, xscale, yscale, max, outfile);

	double zoom = 1.0;
	int active = 0;

	mkdir("frames", 0755);

	for (int i = 0; i < 100; i++)
	{
		pid_t pid = fork();

		if (pid == 0)
		{
			// Child process generates one frame.
			char filename[64];
			snprintf(filename, sizeof(filename),
					 "frames/mandel%d.jpg", i);

			double width = xscale * zoom;
			double height = width * image_height / (double)image_width;

			double xmin = xcenter - width / 2.0;
			double xmax = xcenter + width / 2.0;
			double ymin = ycenter - height / 2.0;
			double ymax = ycenter + height / 2.0;

			imgRawImage *img = initRawImage(image_width, image_height);
			setImageCOLOR(img, 0);

			compute_image(img, xmin, xmax, ymin, ymax, max);
			storeJpegImageFile(img, filename);

			printf("Frame %d/100 generated: %s\n",
				   i + 1, filename);
			fflush(stdout);

			freeRawImage(img);
			_exit(0);
		}
		else if (pid > 0)
		{
			// Parent controls how many children run.
			active++;

			if (active >= processes)
			{
				if (wait(NULL) > 0)
					active--;
				else
				{
					perror("wait");
					break;
				}
			}
		}
		else
		{
			perror("fork");
			break;
		}

		zoom *= 0.92;
	}

	// Wait for remaining children.
	while (active > 0)
	{
		if (wait(NULL) > 0)
			active--;
		else
		{
			perror("wait");
			break;
		}
	}

	return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point(double x, double y, int max)
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while ((x * x + y * y <= 4) && iter < max)
	{

		double xt = x * x - y * y + x0;
		double yt = iter * x * y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void compute_image(imgRawImage *img,
				   double xmin, double xmax,
				   double ymin, double ymax, int max)
{
	int width = img->width;
	int height = img->height;

	double dx = (xmax - xmin) / width;
	double dy = (ymax - ymin) / height;

	double cx = (xmin + xmax) / 2.0;
	double cy = (ymin + ymax) / 2.0;

	for (int j = 0; j < height; j++)
	{
		double y = ymax - (j + 0.5) * dy;

		for (int i = 0; i < width; i++)
		{
			double x = xmin + (i + 0.5) * dx;
			// Rotate the displayed fractal -90 degrees (clockwise).
			double rotated_x = cx - (y - cy);
			double rotated_y = cy + (x - cx);

			int iters = iterations_at_point(rotated_x, rotated_y, max);
			setPixelCOLOR(img, i, j,
						  iteration_to_color(iters, max));
		}
	}
}
/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color(int iters, int max)
{
	if (iters >= max)
		return 0x000000;

	// Preserve the original 10000-iteration color palette
	double color = 0xFFFFFFu * 200.0 *
				   ((double)iters / 10000.0);

	unsigned long long value = (unsigned long long)color;

	return (int)(value & 0xFFFFFFu);
}

// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}
