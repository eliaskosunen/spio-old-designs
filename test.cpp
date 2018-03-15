#include <spio/spio.h>

int main() {
    spio::basic_stream<spio::filehandle_device> s{stdout};
    s.print("Hello world!\n");
}
