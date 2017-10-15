#define SPIO_USE_EXCEPTIONS 0
#define SPIO_USE_FMT 0
#define SPIO_USE_FMT_OSTREAM 0
#include "spio.h"

int main() {
    io::writable_file f(io::make_file_wrapper(stdout, false));
    std::vector<char> data(8192, 'y');
    for(auto i = 1; i < data.size(); i += 2) {
        data[i] = '\n';
    }
    f.write(io::make_span(data), io::characters{8192});
}
