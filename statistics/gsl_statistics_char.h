/* statistics/gsl_statistics_char.h
 * 
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Jim Davies, Brian Gough
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __GSL_STATISTICS_CHAR_H__
#define __GSL_STATISTICS_CHAR_H__

#include <stddef.h>

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

__BEGIN_DECLS

double gsl_stats_char_mean (const char data[], const size_t stride, const size_t n);
double gsl_stats_char_variance (const char data[], const size_t stride, const size_t n);
double gsl_stats_char_sd (const char data[], const size_t stride, const size_t n);
double gsl_stats_char_variance_with_fixed_mean (const char data[], const size_t stride, const size_t n, const double mean);
double gsl_stats_char_sd_with_fixed_mean (const char data[], const size_t stride, const size_t n, const double mean);
double gsl_stats_char_absdev (const char data[], const size_t stride, const size_t n);
double gsl_stats_char_skew (const char data[], const size_t stride, const size_t n);
double gsl_stats_char_kurtosis (const char data[], const size_t stride, const size_t n);
double gsl_stats_char_lag1_autocorrelation (const char data[], const size_t stride, const size_t n);

double gsl_stats_char_covariance (const char data1[], const size_t stride1,const char data2[], const size_t stride2, const size_t n);
double gsl_stats_char_correlation (const char data1[], const size_t stride1,const char data2[], const size_t stride2, const size_t n);

double gsl_stats_char_variance_m (const char data[], const size_t stride, const size_t n, const double mean);
double gsl_stats_char_sd_m (const char data[], const size_t stride, const size_t n, const double mean);
double gsl_stats_char_absdev_m (const char data[], const size_t stride, const size_t n, const double mean);
double gsl_stats_char_skew_m_sd (const char data[], const size_t stride, const size_t n, const double mean, const double sd);
double gsl_stats_char_kurtosis_m_sd (const char data[], const size_t stride, const size_t n, const double mean, const double sd);
double gsl_stats_char_lag1_autocorrelation_m (const char data[], const size_t stride, const size_t n, const double mean);

double gsl_stats_char_covariance_m (const char data1[], const size_t stride1,const char data2[], const size_t stride2, const size_t n, const double mean1, const double mean2);


double gsl_stats_char_pvariance (const char data1[], const size_t stride1, const size_t n1, const char data2[], const size_t stride2, const size_t n2);
double gsl_stats_char_ttest (const char data1[], const size_t stride1, const size_t n1, const char data2[], const size_t stride2, const size_t n2);

char gsl_stats_char_max (const char data[], const size_t stride, const size_t n);
char gsl_stats_char_min (const char data[], const size_t stride, const size_t n);
void gsl_stats_char_minmax (char * min, char * max, const char data[], const size_t stride, const size_t n);

size_t gsl_stats_char_max_index (const char data[], const size_t stride, const size_t n);
size_t gsl_stats_char_min_index (const char data[], const size_t stride, const size_t n);
void gsl_stats_char_minmax_index (size_t * min_index, size_t * max_index, const char data[], const size_t stride, const size_t n);

double gsl_stats_char_median_from_sorted_data (const char sorted_data[], const size_t stride, const size_t n) ;
double gsl_stats_char_quantile_from_sorted_data (const char sorted_data[], const size_t stride, const size_t n, const double f) ;

__END_DECLS

#endif /* __GSL_STATISTICS_CHAR_H__ */
