#include <peep/peep.hpp>

int main() {
    double data[] = {0.0, 1.0, 2.0, 3.0};
    (void)peep::to_string(data, 2, 2, peep::Options().colorbar(false));
    return 0;
}
