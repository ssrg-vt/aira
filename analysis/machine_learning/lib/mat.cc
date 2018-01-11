#include "mat.hh"

using namespace cv;

template <typename Tsys>
static MatT<Tsys> load_csv(const std::string &file, int skip_rows)
{
  CvMLData loader;
  loader.read_csv(file.c_str());
  Mat data(loader.get_values());
  assert((data.rows > 0) && "No data loaded");
  assert((data.cols > 0) && "No data loaded");
  assert((data.rows > skip_rows) && "Insufficient data loaded");

  // Scale to relevant datasize:
  Mat data2 = Mat::zeros(data.rows, data.cols, MatT<Tsys>::Tcv);
  data.convertTo(data2, MatT<Tsys>::Tcv);

  // Cut however many early rows we are told to.  Main use-case is to cut the
  // first row, as that is the column headers.
  return MatT<Tsys>(data2.rowRange(skip_rows, data2.rows));
}

Mat32F load_csv32(const std::string &file, int skip_rows)
{
  return load_csv<float>(file, skip_rows);
}

Mat64F load_csv64(const std::string &file, int skip_rows)
{
  return load_csv<double>(file, skip_rows);
}
