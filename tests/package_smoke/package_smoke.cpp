#include "termimage.h"

int main() {
    double data[] = {0.0, 1.0, 2.0, 3.0};
    (void)termimage::to_string(data, 2, 2, termimage::Options().colorbar(false));
    return 0;
}
