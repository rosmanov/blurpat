#pragma once
#ifndef MAIN_HXX
#define MAIN_HXX

#include <string>

const char* g_program_name;

std::string g_sample_file;
std::string g_input_file;
std::string g_output_file;

/// Template for `printf`-like function.
const char* g_usage_template =
"\nUsage: %1$s OPTIONS sample-image-file\n\n"
"OPTIONS:\n"
" -h, --help               Display this help.\n"
" -s, --sample             Path to sample image.\n"
" -i, --input              Path to input image.\n"
" -o, --output             Path to output image.\n";

const char *g_short_options = "hs:i:o:";
const struct option g_long_options[] = {
  {"help",   no_argument,       NULL, 'h'},
  {"sample", required_argument, NULL, 's'},
  {"input",  required_argument, NULL, 'i'},
  {"output", required_argument, NULL, 'o'},
  {0,               0,                 0,    0}
};

#endif // MAIN_HXX
// vim: et ts=2 sts=2 sw=2
