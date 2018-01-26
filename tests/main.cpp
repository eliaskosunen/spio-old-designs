// Copyright 2017-2018 Elias Kosunen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "spio.h"

#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wweak-template-vtables"
#endif

template class io::basic_buffered_filehandle_base<io::filehandle>;
template class io::basic_buffered_filehandle<io::stdio_filehandle>;
template class io::basic_buffered_filehandle<io::native_filehandle>;
template class io::basic_instream<io::readable_buffer>;
template class io::basic_instream<io::readable_file>;
/* template class io::basic_file_instream<char, io::stdio_filehandle>; */
/* template class io::basic_file_instream<wchar_t, io::stdio_filehandle>; */
/* template class io::basic_file_instream<char, io::native_filehandle>; */
/* template class io::basic_file_instream<wchar_t, io::native_filehandle>; */
/* template class io::basic_buffer_instream<char>; */
/* template class io::basic_buffer_instream<wchar_t>; */
template class io::basic_outstream<io::writable_buffer>;
template class io::basic_outstream<io::writable_file>;
/* template class io::basic_file_outstream<char, io::stdio_filehandle>; */
/* template class io::basic_file_outstream<wchar_t, io::stdio_filehandle>; */
/* template class io::basic_file_outstream<char, io::native_filehandle>; */
/* template class io::basic_file_outstream<wchar_t, io::native_filehandle>; */
/* template class io::basic_buffer_outstream<char>; */
/* template class io::basic_buffer_outstream<wchar_t>; */
template class io::basic_lockable_stream<io::file_instream>;
template class io::basic_lockable_stream<io::buffer_instream>;
template class io::basic_lockable_stream<io::file_outstream>;
template class io::basic_lockable_stream<io::buffer_outstream>;
template class io::basic_readable_file<char, io::stdio_filehandle>;
template class io::basic_readable_file<wchar_t, io::stdio_filehandle>;
template class io::basic_readable_file<char, io::native_filehandle>;
template class io::basic_readable_file<wchar_t, io::native_filehandle>;
template class io::basic_readable_buffer<char>;
template class io::basic_readable_buffer<wchar_t>;
/* template class io::basic_readable_buffer<char, 64>; */
template class io::basic_writable_file<char, io::stdio_filehandle>;
template class io::basic_writable_file<wchar_t, io::stdio_filehandle>;
template class io::basic_writable_file<char, io::native_filehandle>;
template class io::basic_writable_file<wchar_t, io::native_filehandle>;
template class io::basic_writable_buffer<char>;
template class io::basic_writable_buffer<wchar_t>;
template class io::basic_writable_buffer<char,
                                         io::static_writable_buffer<char, 64>>;
template class io::basic_writable_buffer<char, io::span_writable_buffer<char>>;

#if SPIO_USE_STL
#include <string>
template struct io::custom_read<std::string>;
template struct io::custom_write<std::string>;
#endif

template struct io::reader_options<float>;
template struct io::reader_options<int>;
template struct io::reader_options<char>;
template struct io::writer_options<float>;
template struct io::writer_options<int>;
template struct io::writer_options<char>;
template struct io::type<int>;
template struct io::type<char>;
template struct io::type<char (&)[10]>;
template struct io::type<wchar_t>;
template struct io::type<double>;
/* template struct io::type<void*>; */
template struct io::type<std::string>;

using reader = io::buffer_instream;
using writer = io::buffer_outstream;
#if SPIO_USE_FMT
template struct io::custom_write<int>;

template bool io::custom_write<int>::write(writer&,
                                           const int&,
                                           io::writer_options<int>);
template bool io::custom_read<std::string>::read(
    reader&,
    std::string&,
    io::reader_options<std::string>);
template bool io::custom_write<std::string>::write(
    writer&,
    const std::string&,
    io::writer_options<std::string>);
/* template std::size_t io::filebuffer::write(io::const_byte_span, */
/*                                            void(io::const_byte_span)); */
#endif

#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
