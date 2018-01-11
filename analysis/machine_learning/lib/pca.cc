#include "pca.hh"

using namespace cv;

TransformProjectPtr calculate_pca(const Mat32F &m, int dimension)
{
  assert((dimension > 0) && "PCA requires >0 dimension");
  PCA pca(m.mat(), noArray(), CV_PCA_DATA_AS_ROW, dimension);
  return std::make_shared<TransformProject>(pca.eigenvectors);
}

TransformProjectPtr calculate_pca(const Mat32F &m, double variance)
{
  assert(((variance > 0.0) && (variance <= 1.0)) && "Invalid variance");
  PCA pca(m.mat(), noArray(), CV_PCA_DATA_AS_ROW, variance);
  return std::make_shared<TransformProject>(pca.eigenvectors);
}
