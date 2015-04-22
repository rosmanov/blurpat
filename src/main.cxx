#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "exceptions.hxx"
#include "main.hxx"


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template, g_program_name);
}

static inline bool
file_exists(const char* filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}

static inline bool
file_exists(const std::string& filename)
{
  struct stat st;
  return (stat(filename.c_str(), &st) == 0);
}

cv::Scalar
get_MSSIM(const cv::Mat& i1, const cv::Mat& i2)
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


static double
get_avg_MSSIM(const cv::Mat& i1, const cv::Mat& i2)
{
  auto mssim = get_MSSIM(i1, i2);
  return (mssim.val[0] + mssim.val[1] + mssim.val[2]) / 3;
}


static void
match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl)
{
  cv::Mat result;
  int match_method = CV_TM_SQDIFF;

  int result_cols = img.cols - tpl.cols + 1;
  int result_rows = img.rows - tpl.rows + 1;
  result.create(result_cols, result_rows, CV_32FC1);

  cv::matchTemplate(img, tpl, result, match_method);
  cv::normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

  // Locate the best match with minMaxLoc
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
run()
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
      error_log("skipping empty/invalid mask image %sn", mask_file.c_str());
    }
    // Convert mask to grayscale
    if (tpl_img.channels() > 1) {
      cv::cvtColor(tpl_img, tpl_img, CV_BGR2GRAY);
    }

    // Create inverted version of the mask
    cv::Mat tpl_img_inverted;
    cv::bitwise_not(tpl_img, tpl_img_inverted);

    for (auto& img : {in_img, in_img_inverted}) {
      //cv::Rect in_img_roi(0, img.rows - g_kBottomLineHeight, img.cols, g_kBottomLineHeight);
      cv::Rect img_rect(0, 0, img.cols, img.rows);

      cv::Rect tmp_roi(g_roi);
      if (tmp_roi.x < 0) tmp_roi.x = img_rect.width + tmp_roi.x;
      if (tmp_roi.y < 0) tmp_roi.y = img_rect.height + tmp_roi.y;

      cv::Rect in_img_roi(tmp_roi & img_rect);
      if (in_img_roi.area() == 0) {
        error_log("ROI %d,%d %dx%d is out of bounds, skipping",
            tmp_roi.x, tmp_roi.y, tmp_roi.width, tmp_roi.height);
        continue;
      }
      verbose_log("using ROI %d,%d %dx%d",
          in_img_roi.x, in_img_roi.y, in_img_roi.width, in_img_roi.height);

      for (auto& tpl : {tpl_img, tpl_img_inverted}) {
        // Find best matching location for current mask
        match_template(match_loc, img(in_img_roi), tpl);

        // Calculate similarity coefficient
        cv::Rect roi(match_loc.x + (img.cols - in_img_roi.width),
            match_loc.y + (img.rows - in_img_roi.height),
            tpl.cols, tpl.rows);

        auto mssim = get_avg_MSSIM(tpl, img(roi));

        verbose_log("ROI: (%d, %d) %dx%d", roi.x, roi.y, roi.width, roi.height);
        verbose_log("MSSIM for %s: %f", mask_file.c_str(), mssim);

        if (mssim > max_mssim) {
          max_mssim = mssim;
          roi_nearest = roi;
        }
      }
    }
  }

  if (max_mssim <= g_kMinMatchMssim) {
    throw ErrorException("Unable to find a good matching pattern");
  }

  roi_nearest.x      -= g_blurMargin[3];
  roi_nearest.y      -= g_blurMargin[0];
  roi_nearest.width  += g_blurMargin[1] + g_blurMargin[3];
  roi_nearest.height += g_blurMargin[2] + g_blurMargin[0];
  cv::Mat roi_img_nearest(out_img(roi_nearest));

  cv::GaussianBlur(roi_img_nearest, roi_img_nearest,
      cv::Size(g_kernelSize, g_kernelSize),
      g_gaussianBlurDeviation);
#if defined(DEBUG)
  cv::rectangle(roi_img_nearest, cv::Point(0,0),
      cv::Point(roi_img_nearest.cols, roi_img_nearest.rows),
      cv::Scalar(0,200,200), -1, 8);
#endif

  verbose_log("writing to file %s using MSSIM %f", g_output_file.c_str(), max_mssim);
  if (!cv::imwrite(g_output_file, out_img)) {
    throw ErrorException("failed to save to file " + g_output_file);
  }
}


int
main(int argc, char **argv)
{
  int next_option;

  g_program_name = argv[0];

  try {
    do {
      next_option = getopt_long(argc, argv, g_short_options, g_long_options, NULL);

      switch (next_option) {
        case 'h':
          usage(false);
          exit(0);

        case 'i':
          if (!file_exists(optarg)) {
            throw InvalidCliArgException("File '%s' doesn't exist", optarg);
          }
          g_input_file = optarg;
          break;

        case 'k':
          if (optarg) g_kernelSize = get_opt_arg<int>(optarg, "Invalid kernel size");
          break;

        case 'd':
          if (optarg) g_gaussianBlurDeviation = get_opt_arg<int>(optarg, "Invalid Gaussian blur deviation");
          break;

        case 'o':
          g_output_file = optarg;
          break;

        case 't':
          g_threshold = optarg ? get_opt_arg<int>(optarg, "Invalid threshold value") : 0;
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
            g_blurMargin[0] = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_blurMargin[1] = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_blurMargin[2] = std::stoi(std::string(pch));
            if ((pch = strtok(NULL, delim)) == NULL) break;
            g_blurMargin[3] = std::stoi(std::string(pch));
          }
          break;

        case 'v':
          g_verbose++;
          break;

        case -1:
          // done with options
          break;

        case '?':
          // unrecognized option
          usage(true);
          exit(2);

        default:
          error_log("getopt returned character code 0%o", next_option);
          usage(true);
          exit(2);
      }
    } while (next_option != -1);
  } catch (InvalidCliArgException& e) {
    error_log("%s", e.what());
    exit(2);
  }

  if (optind >= argc) {
    error_log0("Mask image(s) expected");
    usage(true);
    exit(1);
  }

  bool error{true};
  do {
    if (g_output_file.empty()) {
      error_log0("output file expected");
      break;
    }
    if (g_input_file.empty()) {
      error_log0("input file expected");
      break;
    }
    error = false;
  } while (0);
  if (error) {
    usage(true);
    exit(2);
  }

  if (g_roi.width <= 0) g_roi.width = 1e6;
  if (g_roi.height <= 0) g_roi.height = 1e6;

  verbose_log("input file: %s", g_input_file.c_str());
  verbose_log("output file: %s", g_output_file.c_str());
  verbose_log("threshold: %f", g_threshold);
  verbose_log("blur kernel size: %d", g_kernelSize);
  verbose_log("blur deviation: %d", g_gaussianBlurDeviation);
  verbose_log("roi: (%d,%d) %dx%d", g_roi.x, g_roi.y, g_roi.width, g_roi.height);
  verbose_log("blur margin: %d %d %d %d", g_blurMargin[0], g_blurMargin[1], g_blurMargin[2], g_blurMargin[3]);

  try {
    while (optind < argc) {
      const char* filename = argv[optind++];
      if (!filename) continue;
      if (!file_exists(filename)) {
        throw InvalidCliArgException("File '%s' doesn't exist", optarg);
      }

      g_mask_files.push_back(std::string(filename));
    }
    if (g_mask_files.empty()) {
      error_log0("No valid mask files provided");
      usage(true);
      exit(2);
    }

    run();
  } catch (ErrorException& e) {
    error_log("Fatal error: %s", e.what());
    exit(2);
  } catch (std::exception& e) {
    error_log("Uncaugth exception: %s", e.what());
    exit(2);
  }

  return 0;
}
// vim: et ts=2 sts=2 sw=2
