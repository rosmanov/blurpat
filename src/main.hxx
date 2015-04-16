#pragma once
#ifndef MAIN_HXX
#define MAIN_HXX

#include <string>
#include <vector>

/////////////////////////////////////////////////////////////////////

#define error_log(fmt, ...) fprintf(stderr,  fmt  "\n", __VA_ARGS__)
#define error_log0(str) fprintf(stderr,  str  "\n")

#define verbose_log(fmt, ...)        \
  do {                               \
    if (g_verbose) {                 \
      printf(fmt "\n", __VA_ARGS__); \
    }                                \
  } while (0)
#define verbose_log2(fmt, ...)       \
  do {                               \
    if (g_verbose > 1) {             \
      printf(fmt "\n", __VA_ARGS__); \
    }                                \
  } while (0)

/////////////////////////////////////////////////////////////////////

int g_verbose{0};

const char* g_program_name;
/// Minimum MSSIM (similarity) coefficient for a pattern match to be
/// considered "good enough"
const double g_kMinMatchMssim{0.1};
/// Color used by cv::threshold()
const int g_kThresholdColor{255};
// XXX replace with "clip region" cv::Rect
const int g_kBottomLineHeight{120};
const int g_kKernelSize{3};
const int g_kGaussianBlurDeviation{10};

/////////////////////////////////////////////////////////////////////
// CLI options

std::vector<std::string> g_mask_files;
std::string g_input_file;
std::string g_output_file;
double g_threshold{80.};

/////////////////////////////////////////////////////////////////////
/// Template for `printf`-like function.
const char* g_usage_template =
"\nUsage: %1$s OPTIONS mask1 [mask2[, mask3[, ...]]]\n\n"
"OPTIONS:\n"
" -h, --help               Display this help.\n"
" -i, --input              Path to input image.\n"
" -o, --output             Path to output image.\n"
" -v, --verbose            Turn on verbose output. Can be used multiple times\n"
"                          to increase verbosity (e.g. -vv). Default: off.\n";

const char *g_short_options = "hs:i:o:v";
const struct option g_long_options[] = {
  {"help",   no_argument,       NULL, 'h'},
  {"input",  required_argument, NULL, 'i'},
  {"output", required_argument, NULL, 'o'},
  {"output", no_argument,       NULL, 'v'},
  {0,        0,                 0,    0}
};

#endif // MAIN_HXX
// vim: et ts=2 sts=2 sw=2
