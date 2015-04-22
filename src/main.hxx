#pragma once
#ifndef MAIN_HXX
#define MAIN_HXX

#include <cstdarg>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>

#include "exceptions.hxx"

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
//const int g_kBottomLineHeight{120};

/////////////////////////////////////////////////////////////////////
// CLI options

std::vector<std::string> g_mask_files;
std::string g_input_file;
std::string g_output_file;
double g_threshold{80.};
int g_kernelSize{3};
int g_gaussianBlurDeviation{10};
cv::Rect g_roi;
int g_blurMargin[4]{0,0,0,0};

/////////////////////////////////////////////////////////////////////
/// Template for `printf`-like function.
const char* g_usage_template =
"\nUsage: %1$s OPTIONS mask1 [mask2[, mask3[, ...]]]\n\n"
"OPTIONS:\n"
" -h, --help               Display this help.\n"
" -v, --verbose            Turn on verbose output. Can be used multiple times\n"
"                          to increase verbosity (e.g. -vv). Default: off.\n"
" -i, --input              Path to input image.\n"
" -o, --output             Path to output image.\n"
" -d, --blur-deviation     Gaussian blur deviation. Default: 10\n"
" -k, --blur-kernel-size   Gaussian blur kernel size. Default: 3\n"
" -t, --threshold          Noise suppression threshold (0..255).\n"
" -r, --roi                Region of interest(ROI) as x,y,width,height.\n"
"                          (width and height are equal to 1000000 by default)\n"
" -m, --blur-margin        Blur margin relative to the ROI as top,right,bottom,left integers.\n"
"                          Default: 0,0,0,0\n"
"\nEXAMPLE:\n"
"The following blurs a logo specified by logo19x24.jpg mask on in.jpg,\n"
"sets 500px wide line at the bottom of in.jpg as the region of interest,\n"
"writes the result to out.jpg:\n"
"%1$s -r 0,-500 -t60 -i in.jpg -o out.jpg -v logo.jpg\n";

const char *g_short_options = "hvi:o:d:k:t:r:m:";
const struct option g_long_options[] = {
  {"help",             no_argument,       NULL, 'h'},
  {"verbose",          no_argument,       NULL, 'v'},
  {"input",            required_argument, NULL, 'i'},
  {"output",           required_argument, NULL, 'o'},
  {"blur-deviation",   required_argument, NULL, 'd'},
  {"blur-kernel-size", required_argument, NULL, 'k'},
  {"threshold",        required_argument, NULL, 't'},
  {"roi",              required_argument, NULL, 'r'},
  {"blur-margin",      required_argument, NULL, 'm'},
  {0,                  0,                 0,    0}
};

/////////////////////////////////////////////////////////////////////

template <class T> T
get_opt_arg(const std::string& optarg, const char* format, ...)
{
  T result;
  std::istringstream is(optarg);

  if (is >> result) {
    return result;
  }

  if (!format) {
    throw ErrorException("Invalid error format in %s", __func__ );
  }

  std::string error;
  va_list args;
  va_start(args, format);
  char message[1024];
  const int message_len = vsnprintf(message, sizeof(message), format, args);
  error = std::string(message, message_len);
  va_end(args);
  throw InvalidCliArgException(error);
}

template int get_opt_arg(const std::string& optarg, const char* format, ...);
template double get_opt_arg(const std::string& optarg, const char* format, ...);

#endif // MAIN_HXX
// vim: et ts=2 sts=2 sw=2
