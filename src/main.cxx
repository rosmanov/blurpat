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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "exceptions.hxx"
#include "main.hxx"

/////////////////////////////////////////////////////////////////////

/// Outputs help message to stdout or stderr depending on `is_error`.
static void
Usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_kUsageTemplate, g_kProgramName);
}


static inline bool
FileExists(const char* filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}


/// Calculates MSSIM similarity coefficients for each channel
cv::Scalar
GetMSSIM(const cv::Mat& i1, const cv::Mat& i2)
{
  const double C1 = 6.5025, C2 = 58.5225;
  int d           = CV_32F;

  cv::Mat I1, I2;
  i1.convertTo(I1, d); // cannot calculate on one byte large values
  i2.convertTo(I2, d);

  cv::Mat I2_2  = I2.mul(I2); // I2^2
  cv::Mat I1_2  = I1.mul(I1); // I1^2
  cv::Mat I1_I2 = I1.mul(I2); // I1 * I2


  cv::Mat mu1, mu2;
  cv::GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
  cv::GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);

  cv::Mat mu1_2   = mu1.mul(mu1);
  cv::Mat mu2_2   = mu2.mul(mu2);
  cv::Mat mu1_mu2 = mu1.mul(mu2);

  cv::Mat sigma1_2, sigma2_2, sigma12;

  cv::GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
  sigma1_2 -= mu1_2;

  cv::GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
  sigma2_2 -= mu2_2;

  cv::GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
  sigma12 -= mu1_mu2;

  cv::Mat t1, t2, t3;

  t1 = 2 * mu1_mu2 + C1;
  t2 = 2 * sigma12 + C2;
  t3 = t1.mul(t2);  // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

  t1 = mu1_2 + mu2_2 + C1;
  t2 = sigma1_2 + sigma2_2 + C2;
  t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

  cv::Mat ssim_map;
  cv::divide(t3, t1, ssim_map); // ssim_map =  t3./t1;

  cv::Scalar mssim = cv::mean(ssim_map); // mssim = average of ssim map
  return mssim;
}


/// Calculates average channel similarity coefficient
static double
GetAvgMSSIM(const cv::Mat& i1, const cv::Mat& i2)
{
  auto mssim = GetMSSIM(i1, i2);
  return (mssim.val[0] + mssim.val[1] + mssim.val[2]) / 3;
}


/// Searches for matching pattern
/// \param match_loc Match location
/// \param img Input image
/// \param tpl The pattern to search for
static void
MatchTemplate(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl)
{
  cv::Mat result;
  int match_method = CV_TM_SQDIFF;

  int result_cols = img.cols - tpl.cols + 1;
  int result_rows = img.rows - tpl.rows + 1;
  result.create(result_cols, result_rows, CV_32FC1);

  cv::matchTemplate(img, tpl, result, match_method);
  cv::normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

  // Find the best match with minMaxLoc
  double min_val, max_val;
  cv::Point min_loc, max_loc;
  cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc, cv::Mat());

  // For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all
  // the other methods, the higher the better
  if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED ) {
    match_loc = min_loc;
  } else {
    match_loc = max_loc;
  }
}


static void
Run()
{
  cv::Point match_loc;
  cv::Mat in_img;
  cv::Mat in_img_inverted;
  cv::Rect roi_nearest;
  cv::Mat out_img;
  double max_mssim{0.};

  // Read input image forcing 3 channels
  in_img = cv::imread(g_input_file, 1);
  if (in_img.empty()) {
    throw ErrorException("failed to read input image " + g_input_file);
  }

  out_img = in_img.clone();

  // Convert input image to grayscale
  if (in_img.channels() > 1) {
    cv::cvtColor(in_img, in_img, CV_BGR2GRAY);
  }

  // Create inverted version of input image
  cv::bitwise_not(in_img, in_img_inverted);

  // Suppress noise
  cv::threshold(in_img, in_img, g_threshold, g_kThresholdColor, CV_THRESH_BINARY);
  cv::threshold(in_img_inverted, in_img_inverted, g_threshold, g_kThresholdColor, CV_THRESH_BINARY);

  // Process mask files
  for (auto& mask_file : g_mask_files) {
    cv::Mat tpl_img(cv::imread(mask_file, 1));
    if (tpl_img.empty()) {
      ERROR_LOG("skipping empty/invalid mask image %sn", mask_file.c_str());
    }
    // Convert mask to grayscale
    if (tpl_img.channels() > 1) {
      cv::cvtColor(tpl_img, tpl_img, CV_BGR2GRAY);
    }

    // Create inverted version of the mask
    cv::Mat tpl_img_inverted;
    cv::bitwise_not(tpl_img, tpl_img_inverted);

    for (auto& img : {in_img, in_img_inverted}) {
      cv::Rect img_rect(0, 0, img.cols, img.rows);

      cv::Rect tmp_roi(g_roi);
      if (tmp_roi.x < 0) tmp_roi.x = img_rect.width + tmp_roi.x;
      if (tmp_roi.y < 0) tmp_roi.y = img_rect.height + tmp_roi.y;

      cv::Rect in_img_roi(tmp_roi & img_rect);
      if (in_img_roi.area() == 0) {
        ERROR_LOG("ROI %d,%d %dx%d is out of bounds, skipping",
            tmp_roi.x, tmp_roi.y, tmp_roi.width, tmp_roi.height);
        continue;
      }
      VERBOSE_LOG("using ROI %d,%d %dx%d",
          in_img_roi.x, in_img_roi.y, in_img_roi.width, in_img_roi.height);

      for (auto& tpl : {tpl_img, tpl_img_inverted}) {
        // Find best matching location for current mask
        MatchTemplate(match_loc, img(in_img_roi), tpl);

        // Calculate similarity coefficient
        cv::Rect roi(match_loc.x + (img.cols - in_img_roi.width),
            match_loc.y + (img.rows - in_img_roi.height),
            tpl.cols, tpl.rows);

        auto mssim = GetAvgMSSIM(tpl, img(roi));

        VERBOSE_LOG("ROI: (%d, %d) %dx%d", roi.x, roi.y, roi.width, roi.height);
        VERBOSE_LOG("MSSIM for %s: %f", mask_file.c_str(), mssim);

        if (mssim > max_mssim) {
          max_mssim = mssim;
          roi_nearest = roi;
        }
      }
    }
  }

  if (max_mssim <= g_min_match_mssim) {
    throw ErrorException("Unable to find a good matching pattern");
  }

  roi_nearest.x      -= g_blur_margin[3];
  roi_nearest.y      -= g_blur_margin[0];
  roi_nearest.width  += g_blur_margin[1] + g_blur_margin[3];
  roi_nearest.height += g_blur_margin[2] + g_blur_margin[0];
  cv::Mat roi_img_nearest(out_img(roi_nearest));

  cv::GaussianBlur(roi_img_nearest, roi_img_nearest,
      cv::Size(g_kernel_size, g_kernel_size),
      g_gaussian_blur_deviation);
#if defined(DEBUG)
  cv::rectangle(roi_img_nearest, cv::Point(0,0),
      cv::Point(roi_img_nearest.cols, roi_img_nearest.rows),
      cv::Scalar(0,200,200), -1, 8);
#endif

  VERBOSE_LOG("writing to file %s using MSSIM %f", g_output_file.c_str(), max_mssim);
  if (!g_dry_run) {
    if (!cv::imwrite(g_output_file, out_img)) {
      throw ErrorException("failed to save to file " + g_output_file);
    }
  }
}

/////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  int next_option;

  g_kProgramName = argv[0];

  try {
    do {
      next_option = getopt_long(argc, argv, g_kShortOptions, g_kLongOptions, NULL);

      switch (next_option) {
        case 'h':
          Usage(false);
          ::exit(EXIT_SUCCESS);

        case 'i':
          if (!FileExists(optarg)) {
            throw InvalidCliArgException("File '%s' doesn't exist", optarg);
          }
          g_input_file = optarg;
          break;

        case 'k':
          if (optarg) g_kernel_size = GetOptArg<int>(optarg, "Invalid kernel size");
          break;

        case 'd':
          if (optarg) g_gaussian_blur_deviation = GetOptArg<int>(optarg, "Invalid Gaussian blur deviation");
          break;

        case 'o':
          g_output_file = optarg;
          break;

        case 't':
          g_threshold = optarg ? GetOptArg<int>(optarg, "Invalid threshold value") : 0;
          break;

        case 'r':
          {
            if (!optarg) break;

            const char* delim = ",";
            char* pch;
            if ((pch = strtok(optarg, delim)) == NULL) break;
            g_roi.x = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_roi.y = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_roi.width = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_roi.height = std::stoi(std::string(pch));
          }
          break;

        case 'm':
          {
            if (!optarg) break;

            const char* delim = ",";
            char* pch;
            if ((pch = strtok(optarg, delim)) == NULL) break;
            g_blur_margin[0] = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_blur_margin[1] = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_blur_margin[2] = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_blur_margin[3] = std::stoi(std::string(pch));
          }
          break;

        case 's':
          g_min_match_mssim = GetOptArg<double>(optarg, "Invalid min. MSSIM value");
          break;

        case 'T':
          g_dry_run = static_cast<bool>(GetOptArg<int>(optarg, ""));
          break;

        case 'v':
          g_verbose++;
          break;

        case -1:
          // done with options
          break;

        case '?':
          // unrecognized option
          Usage(true);
          ::exit(EXIT_FAILURE);

        default:
          ERROR_LOG("getopt returned character code 0%o", next_option);
          Usage(true);
          ::exit(EXIT_FAILURE);
      }
    } while (next_option != -1);
  } catch (InvalidCliArgException& e) {
    ERROR_LOG("%s", e.what());
    ::exit(EXIT_FAILURE);
  }

  if (optind >= argc) {
    ERROR_LOG0("Mask image(s) expected");
    Usage(true);
    ::exit(EXIT_FAILURE);
  }

  bool error{true};
  do {
    if (g_output_file.empty()) {
      ERROR_LOG0("output file expected");
      break;
    }
    if (g_input_file.empty()) {
      ERROR_LOG0("input file expected");
      break;
    }
    if (g_min_match_mssim < 0 || g_min_match_mssim > 1) {
      ERROR_LOG0("min. MSSIM value is out of range [0.0 .. 1.0]");
      break;
    }

    error = false;
  } while (0);
  if (error) {
    Usage(true);
    ::exit(EXIT_FAILURE);
  }

  if (g_roi.width <= 0) g_roi.width = 1e6;
  if (g_roi.height <= 0) g_roi.height = 1e6;

  VERBOSE_LOG("input file: %s", g_input_file.c_str());
  VERBOSE_LOG("output file: %s", g_output_file.c_str());
  VERBOSE_LOG("threshold: %f", g_threshold);
  VERBOSE_LOG("blur kernel size: %d", g_kernel_size);
  VERBOSE_LOG("blur deviation: %d", g_gaussian_blur_deviation);
  VERBOSE_LOG("roi: (%d,%d) %dx%d", g_roi.x, g_roi.y, g_roi.width, g_roi.height);
  VERBOSE_LOG("blur margin: %d %d %d %d", g_blur_margin[0], g_blur_margin[1], g_blur_margin[2], g_blur_margin[3]);
  VERBOSE_LOG("min. MSSIM: %f", g_min_match_mssim);
  VERBOSE_LOG("dry run: %d", static_cast<int>(g_dry_run));

  try {
    while (optind < argc) {
      const char* filename{argv[optind++]};
      if (!filename) continue;
      if (!FileExists(filename)) {
        throw InvalidCliArgException("File '%s' doesn't exist", optarg);
      }

      g_mask_files.push_back(std::string(filename));
    }
    if (g_mask_files.empty()) {
      ERROR_LOG0("No valid mask files provided");
      Usage(true);
      ::exit(EXIT_FAILURE);
    }

    Run();
  } catch (ErrorException& e) {
    ERROR_LOG("Fatal error: %s", e.what());
    ::exit(EXIT_FAILURE);
  } catch (std::exception& e) {
    ERROR_LOG("Uncaugth exception: %s", e.what());
    ::exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

// vim: et ts=2 sts=2 sw=2
