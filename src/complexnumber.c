#include "complexnumber.h"

#include <math.h>

#define NO_RETURN_RADIUS 2.0
#define SQUARE(nr) ((nr)*(nr))

void square_complex_number(complex_number *nr) {
    precision_t real_aux = nr->real;
    nr->real = SQUARE(nr->real) - SQUARE(nr->imag);
    nr->imag *= 2*real_aux;
}

precision_t absolute_of_complex_number(complex_number nr) {
    return sqrt(SQUARE(nr.real) + SQUARE(nr.imag));
}

int stability_complex_number(complex_number c, int max_iterations) {
    int iterations = 0;
    complex_number z = {0.0, 0.0};

    for (; iterations < max_iterations && absolute_of_complex_number(z) < NO_RETURN_RADIUS; iterations++) {
        square_complex_number(&z);

        z.real += c.real;
        z.imag += c.imag;
    }
    return iterations;
}
