#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "config.h"
#include "t_quantile.h"
using namespace std;

inline double sq(double x) {
  return x * x;
}

inline double median(const vector<double>& data, size_t start, size_t end) {
  size_t n = end - start;
  if (n % 2 == 0) {
    return (data[start + n / 2 - 1] + data[start + n / 2]) / 2.0;
  } else {
    return data[start + n / 2];
  }
}

inline void calculateQuartiles(vector<double>& data, double& Q1, double& Q3) {
  size_t n = data.size();
  sort(data.begin(), data.end());

  // Median of the lower half
  Q1 = median(data, 0, n / 2);

  // Median of the upper half
  // (n + 1) / 2 handles the case when n is odd
  Q3 = median(data, (n + 1) / 2, n);
}

// Function to remove outliers using the IQR method
inline void removeOutliersIQR(vector<double>& data) {
  double Q1, Q3;
  calculateQuartiles(data, Q1, Q3);

  double IQR = Q3 - Q1;
  double lower_bound = Q1 - 1.5 * IQR;
  double upper_bound = Q3 + 1.5 * IQR;

  // Remove elements outside of the IQR bounds inplace
  data.erase(remove_if(data.begin(), data.end(),
                       [lower_bound, upper_bound](double val) {
                         return val < lower_bound || val > upper_bound;
                       }),
             data.end());
}

struct DataSet {
  double mean;
  double sd;
  int n;
  int outliers;

  DataSet(const vector<double>& data, HandleOutliners handleOutliers) {
    vector<double> v = data;

    if (handleOutliers == HandleOutliners::Remove)
      removeOutliersIQR(v);

    mean = arithmetic_mean(v);
    sd = standard_deviation(v, mean);
    n = v.size();
    outliers = data.size() - v.size();
  }

 private:
  static inline double arithmetic_mean(const vector<double>& arr) {
    double sum = 0;
    for (double x : arr)
      sum += x;
    return sum / arr.size();
  }

  static inline double standard_deviation(const vector<double>& arr,
                                          double mean) {
    double sum = 0;
    for (double x : arr)
      sum = sum + (x - mean) * (x - mean);

    return sqrt(sum / (arr.size() - 1));
  }
};

// Function to find t-test of two set of statistical data.
// in R: t.test(arr1, arr2, alternative="greater", conf.level=0.90)
//
// Returns the lower_bound of real_mean(arr1) - real_mean(arr2) with the
// specifyied confidence
inline double ttest_lower_bound(DataSet& x, DataSet& y, double conf) {
  double x_sem = sq(x.sd) / x.n;
  double y_sem = sq(y.sd) / y.n;

  double se = sqrt(x_sem + y_sem);
  double df =
      sq(x_sem + y_sem) / (sq(x_sem) / (x.n - 1) + sq(y_sem) / (y.n - 1));
  double qt = t_quantile(conf, df);

  double lower_bound = (x.mean - y.mean) - qt * se;

  return lower_bound;
}
