#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "log.hxx"
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

  // Localize the best match with minMaxLoc

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
  cv::Mat tpl_img(cv::imread(g_sample_file, 1));
  if (tpl_img.empty()) {
    throw ErrorException("failed to read sample image " + g_sample_file);
  }
  if (tpl_img.channels() > 1) {
    cv::cvtColor(tpl_img, tpl_img, CV_BGR2GRAY);
  }

  cv::Mat in_img(cv::imread(g_input_file, 1));
  if (in_img.empty()) {
    throw ErrorException("failed to read input image " + g_sample_file);
  }
  cv::Mat in_gray_img = in_img.clone();
  if (in_gray_img.channels() > 1) {
    cv::cvtColor(in_gray_img, in_gray_img, CV_BGR2GRAY);
  }

  cv::Mat out_img(in_img.clone());

  cv::Point match_loc;
  match_template(match_loc, in_gray_img, tpl_img);
  printf("match_loc: %d,%d %dx%d\n",
      match_loc.x, match_loc.y, tpl_img.rows, tpl_img.cols);


  cv::Rect roi(match_loc.x, match_loc.y, tpl_img.cols, tpl_img.rows);
  cv::Mat roi_img(out_img(roi));

  auto mssim = get_avg_MSSIM(roi_img, in_img(roi));
  printf("MSSIM: %f\n", mssim);

  cv::GaussianBlur(roi_img, roi_img, cv::Size(3, 3), 10);

  printf("writing to file %s\n", g_output_file.c_str());
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

        case 's':
          if (!file_exists(optarg)) {
            throw InvalidCliArgException("File '%s' doesn't exist", optarg);
          }
          g_sample_file = optarg;
          break;

        case 'i':
          if (!file_exists(optarg)) {
            throw InvalidCliArgException("File '%s' doesn't exist", optarg);
          }
          g_input_file = optarg;
          break;

        case 'o':
          g_output_file = optarg;
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

  bool error{true};
  do {
    if (g_sample_file.empty()) {
      error_log0("sample file expected");
      break;
    }
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

  printf("sample file: %s\n", g_sample_file.c_str());
  printf("input file: %s\n", g_input_file.c_str());
  printf("output file: %s\n", g_output_file.c_str());

  try {
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
