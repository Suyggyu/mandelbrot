#ifndef COMPLEX_NUMBER_H
#define COMPLEX_NUMBER_H

typedef double precision_t;

typedef struct {
  precision_t real, imag;
} complex_number;

void square_complex_number(complex_number *nr);
int stability_complex_number(complex_number c, int max_iterations);
precision_t absolute_of_complex_number(complex_number nr);

#endif
