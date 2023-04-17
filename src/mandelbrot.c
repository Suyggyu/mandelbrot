#ifndef BENCHMARK
    #include <ncurses.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "complexnumber.h"
#include "stb_image_write.h"

// make gcc shut up about usleep implicit declaration
#define _BSD_SOURCE
#include <unistd.h>

typedef struct {
    int *stability_buf;
    int row_width, resolution_x, resolution_y, max_iterations, antialiasing;
    complex_number center;
    precision_t scale_x, scale_y;
} draw_args;

void draw(int *stability_buf, int row_width, complex_number center, precision_t scale_x, precision_t scale_y, int resolution_x, int resolution_y, int max_iterations, int antialiasing) {
    static const precision_t    aa_dx[] = {-0.25, 0.25, -0.25, 0.25},
                                aa_dy[] = {0.25, 0.25, -0.25, -0.25};
    int stability, stability_avg = 0;
    complex_number z;
    const precision_t   pixel_width = 4.0/resolution_x/scale_x,
                        pixel_height = 2.0/resolution_y/scale_y;

    for (int y = 0; y < resolution_y; y++) {
        for (int x = 0; x < resolution_x; x++) {
            if (antialiasing) {
                stability_avg = 0;
                for(int i = 0; i < 4; i++)
                {
                    z.real = center.real + ((precision_t)x-resolution_x/2.0+aa_dx[i]) * pixel_width;
                    z.imag = center.imag + ((precision_t)y-resolution_y/2.0+aa_dy[i]) * pixel_height;
                    stability = stability_complex_number(z, max_iterations);
                    stability_avg += stability;
                }
                stability = stability_avg/4;
            } else {
                z.real = center.real + (x-resolution_x/2.0) * pixel_width;
                z.imag = center.imag + (y-resolution_y/2.0) * pixel_height;
                stability = stability_complex_number(z, max_iterations);
            }
            stability_buf[row_width*y+x] = stability;
        }
    }
}

void *draw_pthread(void *args)
{
    draw_args arguments = *(draw_args *)args;
    draw(arguments.stability_buf, arguments.row_width, arguments.center, arguments.scale_x, arguments.scale_y, arguments.resolution_x, arguments.resolution_y, arguments.max_iterations, arguments.antialiasing);
    return NULL;
}

void draw_parallel(int thread_count, int *stability_buf, complex_number center, precision_t scale, int resolution_x, int resolution_y, int max_iterations, int antialiasing) {
    const precision_t viewport_width = 4.0/scale;
    center.real += (1.0/(thread_count*2.0) - 1.0/2.0)*viewport_width;

    pthread_t threads[thread_count];
    draw_args arguments[thread_count];

    for (int i = 0; i < thread_count; i++) {
        arguments[i].antialiasing = antialiasing;
        arguments[i].max_iterations = max_iterations;
        arguments[i].stability_buf = stability_buf+i*resolution_x/thread_count;
        arguments[i].center = center;
        arguments[i].scale_x = scale*thread_count;
        arguments[i].scale_y = scale;
        arguments[i].resolution_x = resolution_x/thread_count;
        arguments[i].resolution_y = resolution_y;
        arguments[i].row_width = resolution_x;
        pthread_create(&threads[i], NULL, draw_pthread, &arguments[i]);
        // draw(stability_buf+i*resolution_x/thread_count, resolution_x, center, scale*thread_count, scale, resolution_x/thread_count, resolution_y, max_iterations, antialiasing);
        center.real += viewport_width/thread_count;
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

char *create_file_name(int n)
{
    char *fn = malloc(sizeof(*fn)*64);
    char *n_cstring = malloc(sizeof(*n_cstring)*10);
    int masca_cifre = 1, idx = 0;

    while (n/masca_cifre/10) {
        masca_cifre *= 10;
    }

    while (masca_cifre > 0) {
        n_cstring[idx++] = '0' + n/masca_cifre%10;
        masca_cifre /= 10;
    }
    n_cstring[idx] = 0;

    strcpy(fn, "screenshot");
    strcat(fn, n_cstring);
    strcat(fn, ".png");

    free(n_cstring);
    return fn;
}

void screenshot(complex_number center, precision_t scale, int resolution_x, int resolution_y, int max_iterations, int antialiasing) {
    static int screenshot_cnt = 0;
    char *fn = create_file_name(screenshot_cnt), *img_pixels;
    int *img_stability = malloc(sizeof(*img_stability)*resolution_x*resolution_y);

        // get data
        draw_parallel(NTHREADS, img_stability, center, scale, resolution_x, resolution_y, max_iterations, antialiasing);

        img_pixels = malloc(sizeof(*img_pixels)*resolution_x*resolution_y*3);

        int current_element_idx, current_pixel_idx;
        for (int y = 0; y < resolution_y; y++) {
            for (int x = 0; x < resolution_x; x++) {
                current_element_idx = resolution_x*y+x;
                current_pixel_idx = 3*(resolution_x*y+x);
                for (int i = 0; i < 3; i++) {
                    img_pixels[current_pixel_idx+i] = (1+max_iterations-img_stability[current_element_idx]*255)/max_iterations;
                }
            }
        }
        
        stbi_write_png(fn, resolution_x, resolution_y, 3, img_pixels, 3*resolution_x);

    screenshot_cnt++;

    free(fn);
    free(img_stability);
    free(img_pixels);
}

int main(int argc, const char **argv) {
    complex_number center = {-0.5, 0.0};
    char c, antialiasing = 0, *img;
    stbi_write_png_compression_level = 0;
    int img_size_w = 16*3, img_size_h = 9*3, max_iterations = 200;
    int *img_stability = malloc(sizeof(*img_stability)*img_size_h*img_size_w);
    precision_t char_scale = 1.0;
    
#ifdef BENCHMARK
    screenshot(center, char_scale, 3840, 2160, 200, 1);
#else

    char const *ascii_luminosity = "$@B8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
    unsigned int const ascii_luminosity_len = strlen(ascii_luminosity);
    img = malloc(sizeof(*img)*img_size_h*(img_size_w+1));

    // init ncurses console
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, 1);

    do {
        // handle input
        c = getch();
        switch (c) {
            case 'a':
                center.real -= 4.0/img_size_w/char_scale*4;
                break;
            case 'd':
                center.real += 4.0/img_size_w/char_scale*4;
                break;
            case 'w':
                center.imag -= 2.0/img_size_h/char_scale*4;
                break;
            case 's':
                center.imag += 2.0/img_size_h/char_scale*4;
                break;
            case 'r':
                antialiasing = 1 - antialiasing;
                break;
            case 'e':
                screenshot(center, char_scale, 1920, 1080, max_iterations, antialiasing);
                break;
            case '=':
                char_scale /= 0.9;
                char_scale /= 0.9;
                break;
            case '-':
                char_scale *= 0.9;
                char_scale *= 0.9;
                break;
            case '[':
                if (max_iterations > 5) max_iterations *= 0.8;
                break;
            case ']':
                max_iterations /= 0.8;
                break;
        }

        // get data
        draw_parallel(NTHREADS, img_stability, center, char_scale, img_size_w, img_size_h, max_iterations, antialiasing);
 
        // build image
        for (int y = 0; y < img_size_h; y++) {
            for (int x = 0; x < img_size_w; x++) {
                img[(img_size_w+1)*y+x] = ascii_luminosity[ascii_luminosity_len-1-((ascii_luminosity_len-1)*img_stability[y*img_size_w+x])/max_iterations];
            }
            img[(img_size_w+1)*y+img_size_w] = '\n';
        }
        img[(img_size_w+1)*img_size_h-1] = 0;
        
        clear();
        printw("%s\n", img);
        refresh();

        usleep(50000);
    } while (c != 'q');

    // restore console
    nocbreak();
    echo();
    endwin();
    nodelay(stdscr, 0);

    free(img_stability);
    free(img);
#endif
    return 0;
}