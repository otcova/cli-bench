#include <cmath>
#include <iostream>
#include <limits>

#define STOP 1.0e-8
#define TINY 1.0e-16

// Source: https://github.com/codeplea/incbeta
inline double incbeta(double a, double b, double x) {
  if (x < 0.0 || x > 1.0)
    return 1.0 / 0.0;

  /*The continued fraction converges nicely for x < (a+1)/(a+b+2)*/
  if (x > (a + 1.0) / (a + b + 2.0)) {
    return (1.0 -
            incbeta(b, a, 1.0 - x)); /*Use the fact that beta is symmetrical.*/
  }

  /*Find the first part before the continued fraction.*/
  const double lbeta_ab = lgamma(a) + lgamma(b) - lgamma(a + b);
  const double front = exp(log(x) * a + log(1.0 - x) * b - lbeta_ab) / a;

  /*Use Lentz's algorithm to evaluate the continued fraction.*/
  double f = 1.0, c = 1.0, d = 0.0;

  int i, m;
  for (i = 0; i <= 200; ++i) {
    m = i / 2;

    double numerator;
    if (i == 0) {
      numerator = 1.0; /*First numerator is 1.0.*/
    } else if (i % 2 == 0) {
      numerator = (m * (b - m) * x) /
                  ((a + 2.0 * m - 1.0) * (a + 2.0 * m)); /*Even term.*/
    } else {
      numerator = -((a + m) * (a + b + m) * x) /
                  ((a + 2.0 * m) * (a + 2.0 * m + 1)); /*Odd term.*/
    }

    /*Do an iteration of Lentz's algorithm.*/
    d = 1.0 + numerator * d;
    if (fabs(d) < TINY)
      d = TINY;
    d = 1.0 / d;

    c = 1.0 + numerator / c;
    if (fabs(c) < TINY)
      c = TINY;

    const double cd = c * d;
    f *= cd;

    /*Check for stop.*/
    if (fabs(1.0 - cd) < STOP) {
      return front * (f - 1.0);
    }
  }

  return 1.0 / 0.0; /*Needed more loops, did not converge.*/
}

inline double student_t_cdf(double t, double v) {
  double x = (t + sqrt(t * t + v)) / (2.0 * sqrt(t * t + v));
  double prob = incbeta(v / 2.0, v / 2.0, x);
  return prob;
}

inline double student_t_pdf(double t, double df) {
  double log_coeff = std::lgamma((df + 1.0) / 2.0) - std::lgamma(df / 2.0) -
                     0.5 * std::log(df * M_PI);
  double exponent = -((df + 1.0) / 2.0) * std::log(1.0 + (t * t) / df);

  return std::exp(log_coeff + exponent);
}

// Newton-Raphson method to find t-quantile
inline double t_quantile(double alpha,
                         double df,
                         int max_iter = 20,
                         double tol = 1e-6) {
  if (df <= 0)
    return std::numeric_limits<double>::quiet_NaN();

  double t = 0.0;  // Initial guess
  for (int i = 0; i < max_iter; ++i) {
    double f = student_t_cdf(t, df) - alpha;
    double f_prime = student_t_pdf(t, df);

    if (std::abs(f) < tol)
      break;  // Convergence achieved

    t -= f / f_prime;  // Newton-Raphson step
  }

  return t;
}
