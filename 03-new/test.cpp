#include <spio.h>

int main()
{
    std::vector<char> vec;
    spio::vector_outstream vec_out(vec);
    spio::putln(vec_out, "Hello world!");
    vec.push_back('\0');

    spio::stdio_outstream out(stdout);
    spio::putstr(out, vec.data());
}
