#ifndef _PCA_HH
#define _PCA_HH

#include "mat.hh"
#include "transform.hh"

TransformProjectPtr calculate_pca(const Mat32F &m, int dimension);
TransformProjectPtr calculate_pca(const Mat32F &m, double variance);

#endif // _PCA_HH
