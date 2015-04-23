/* \file
 *
 * \copyright Copyright © 2015  Ruslan Osmanov <rrosmanov@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
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

#define ERROR_LOG(fmt, ...) fprintf(stderr,  fmt  "\n", __VA_ARGS__)
#define ERROR_LOG0(str) fprintf(stderr,  str  "\n")

#define VERBOSE_LOG(fmt, ...)        \
  do {                               \
    if (g_verbose) {                 \
      printf(fmt "\n", __VA_ARGS__); \
    }                                \
  } while (0)
#define VERBOSE_LOG2(fmt, ...)       \
  do {                               \
    if (g_verbose > 1) {             \
      printf(fmt "\n", __VA_ARGS__); \
    }                                \
  } while (0)

/////////////////////////////////////////////////////////////////////

int g_verbose{0};

const char* g_kProgramName;
/// Color used by cv::threshold()
const int g_kThresholdColor{255};

/////////////////////////////////////////////////////////////////////
// CLI options

std::vector<std::string> g_mask_files;
std::string g_input_file;
std::string g_output_file;
double g_threshold{80.};
int g_kernel_size{3};
int g_gaussian_blur_deviation{10};
cv::Rect g_roi;
int g_blur_margin[4]{0,0,0,0};
/// Minimum MSSIM (similarity) coefficient for a pattern match to be
/// considered "good enough"
double g_min_match_mssim{0.1};
bool g_dry_run{false};

/////////////////////////////////////////////////////////////////////
/// Template for `printf`-like function.
const char* g_kUsageTemplate{
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
" -s, --min-mssim          Minimum MSSIM value to consider a match successful.\n"
"                          Possible values: 0..1 incl. Default: 0.1\n"
" -T, --dry-run            Don't write to FS\n"
"\nEXAMPLE:\n"
"The following blurs a logo specified by logo19x24.jpg mask on in.jpg,\n"
"sets 500px wide line at the bottom of in.jpg as the region of interest,\n"
"writes the result to out.jpg:\n"
"%1$s -r 0,-500 -t60 -i in.jpg -o out.jpg -v logo.jpg\n"};

const char *g_kShortOptions = "hvi:o:d:k:t:r:m:s:T";
const struct option g_kLongOptions[] = {
  {"help",             no_argument,       NULL, 'h'},
  {"verbose",          no_argument,       NULL, 'v'},
  {"input",            required_argument, NULL, 'i'},
  {"output",           required_argument, NULL, 'o'},
  {"blur-deviation",   required_argument, NULL, 'd'},
  {"blur-kernel-size", required_argument, NULL, 'k'},
  {"threshold",        required_argument, NULL, 't'},
  {"roi",              required_argument, NULL, 'r'},
  {"blur-margin",      required_argument, NULL, 'm'},
  {"min-mssim",        required_argument, NULL, 's'},
  {"dry-run",          no_argument,       NULL, 'T'},
  {0,                  0,                 0,    0}
};

/////////////////////////////////////////////////////////////////////

template<class T> T
GetOptArg(const std::string& optarg, const char* format, ...)
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

template int GetOptArg(const std::string& optarg, const char* format, ...);
template double GetOptArg(const std::string& optarg, const char* format, ...);

#endif // MAIN_HXX
// vim: et ts=2 sts=2 sw=2
