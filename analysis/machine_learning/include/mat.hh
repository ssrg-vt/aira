#ifndef _MAT_HH
#define _MAT_HH

#include <opencv/cv.h>
#include <opencv/ml.h>
#include <limits>
#include <cmath>
#include <initializer_list>

// Pre-declare some classes (sub-classes of class MatT).
template <typename Tsys> class RowT;
template <typename Tsys> class ColT;

// MatT<type> -- A wrapper around an OpenCV 2D matrix.
template <typename Tsys>
class MatT {
protected:
  cv::Mat _mat;

public:
  // The "type" of this matrix within the OpenCV type system (at the time of
  // writing float -> 5 and double -> 6).  This is required to create OpenCV
  // matrices of the correct "type".
  static const int Tcv = cv::DataType<Tsys>::type;

  MatT(int rows, int cols) : _mat(cv::Mat::zeros(rows, cols, Tcv)) {
    assert((rows > 0) && (cols > 0) && "All matrix dimensions must be >0.");
  }
  MatT(cv::Mat _m) : _mat(_m) {
    assert((_m.type() == Tcv) && "Template and provided matrix type mismatch");
  }

  // Information about this matrix.
  int rows() const { return _mat.rows; }
  int cols() const { return _mat.cols; }
  bool equals(const MatT<Tsys> &other) const;

  // Access/modify this matrix.
  Tsys& at(int row, int col);
  RowT<Tsys> row(int row);
  ColT<Tsys> col(int col);
  const Tsys& at(int row, int col) const;
  const RowT<Tsys> row(int row) const;
  const ColT<Tsys> col(int col) const;

  // Matrix functionality.
  MatT<Tsys> operator* (const MatT<Tsys> &other) const;
  MatT<Tsys> operator+ (const MatT<Tsys> &other) const;
  MatT<Tsys> operator- (const MatT<Tsys> &other) const;
  MatT<Tsys> operator* (Tsys val) const;
  MatT<Tsys> operator/ (Tsys val) const;
  MatT<Tsys> operator+ (Tsys val) const;
  MatT<Tsys> operator- (Tsys val) const;
  MatT<Tsys> abs() const;
  MatT<Tsys> transpose() const;

  // Reduction functionality.
  Tsys sum() const;
  Tsys mean() const;
  Tsys min() const;
  Tsys max() const;
  Tsys range() const;
  RowT<Tsys> sums() const;
  RowT<Tsys> means() const;
  RowT<Tsys> ranges() const;

  // Conversions
  std::pair< MatT<Tsys>, MatT<Tsys> > split(int col) const;
  MatT<Tsys> concat_vertical(const MatT<Tsys> &other) const;
  MatT<Tsys> concat_horizontal(const MatT<Tsys> &other) const;
  MatT<float>  convert32() const;
  MatT<double> convert64() const;

  // All abstractions are leaky, OpenCV requires access to the raw matrices.
  cv::Mat& mat() { return _mat; }
  const cv::Mat& mat() const { return _mat; }
};

// RowT<type> -- A sub-class of MatT<type>, represents a row.
template <typename Tsys>
class RowT : public MatT<Tsys> {
public:
  RowT(int cols) : MatT<Tsys>(1, cols) {}
  RowT(cv::Mat _m) : MatT<Tsys>(_m) {
    assert((_m.rows == 1) && "A row object can only have one row.");
  }
  RowT(std::initializer_list<Tsys> l);

  Tsys& at(int col) { return MatT<Tsys>::at(0, col); }
  const Tsys& at(int col) const { return MatT<Tsys>::at(0, col); }
  int min_index() const;
  int max_index() const;

private:
  int search(Tsys val) const;
};

// ColT<type> -- A sub-class of MatT<type>, represents a column.
template <typename Tsys>
class ColT : public MatT<Tsys> {
public:
  ColT(int rows) : MatT<Tsys>(rows, 1) {}
  ColT(cv::Mat _m) : MatT<Tsys>(_m) {
    assert((_m.cols == 1) && "A column object can only have one column.");
  }
  ColT(std::initializer_list<Tsys> l);

  Tsys& at(int row) { return MatT<Tsys>::at(row, 0); }
  const Tsys& at(int row) const { return MatT<Tsys>::at(row, 0); }
};

typedef MatT<float>  Mat32F;
typedef MatT<int>    Mat32D;
typedef RowT<float>  Row32F;
typedef RowT<int>    Row32D;
typedef ColT<float>  Col32F;
typedef ColT<int>    Col32D;
typedef MatT<double> Mat64F;
typedef RowT<double> Row64F;
typedef ColT<double> Col64F;

template <typename Tsys>
bool MatT<Tsys>::equals(const MatT<Tsys> &other) const
{
  // If the dimensions don't match then the data can't.
  if (cols() != other.cols()) return false;
  if (rows() != other.rows()) return false;

  // Check to see if any piece of data is different.
  const Tsys epsilon = std::numeric_limits<Tsys>::epsilon();
  for (int i = 0; i < rows(); i++) {
    for (int j = 0; j < cols(); j++) {
      // TODO need a better way of doing this, numerically dodgy.
      if (fabs(at(i,j) - other.at(i,j)) > epsilon) return false;
    }
  }

  // Didn't find any differences, so they're the same!
  return true;
}

template <typename Tsys>
Tsys& MatT<Tsys>::at(int row, int col) {
  assert((row >= 0) && (row < rows()) && "Invalid row index.");
  assert((col >= 0) && (col < cols()) && "Invalid col index.");
  return _mat.at<Tsys>(row, col);
}
template <typename Tsys>
const Tsys& MatT<Tsys>::at(int row, int col) const {
  assert((row >= 0) && (row < rows()) && "Invalid row index.");
  assert((col >= 0) && (col < cols()) && "Invalid col index.");
  return _mat.at<Tsys>(row, col);
}

template <typename Tsys>
RowT<Tsys> MatT<Tsys>::row(int row)
{
  assert((row >= 0) && (row < rows()) && "Invalid row index.");
  return RowT<Tsys>(_mat.row(row));
}
template <typename Tsys>
const RowT<Tsys> MatT<Tsys>::row(int row) const
{
  assert((row >= 0) && (row < rows()) && "Invalid row index.");
  return RowT<Tsys>(_mat.row(row));
}

template <typename Tsys>
ColT<Tsys> MatT<Tsys>::col(int col)
{
  assert((col >= 0) && (col < cols()) && "Invalid col index.");
  return ColT<Tsys>(_mat.col(col));
}
template <typename Tsys>
const ColT<Tsys> MatT<Tsys>::col(int col) const
{
  assert((col >= 0) && (col < cols()) && "Invalid col index.");
  return ColT<Tsys>(_mat.col(col));
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator*(const MatT<Tsys> &other) const
{
  assert((cols() == other.rows()) && "Invalid dimensions for matrix multiply");
  cv::Mat product = _mat * other.mat();
  return MatT<Tsys>(product);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator+(const MatT<Tsys> &other) const
{
  assert((rows() == other.rows()) && "Invalid dimensions for matrix addition");
  assert((cols() == other.cols()) && "Invalid dimensions for matrix addition");
  cv::Mat addition = _mat + other.mat();
  return MatT<Tsys>(addition);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator-(const MatT<Tsys> &other) const
{
  assert((rows() == other.rows()) && "Invalid dimensions for matrix subtract");
  assert((cols() == other.cols()) && "Invalid dimensions for matrix subtract");
  cv::Mat subtraction = _mat - other.mat();
  return MatT<Tsys>(subtraction);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator*(Tsys val) const
{
  cv::Mat product = _mat * val;
  return MatT<Tsys>(product);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator/(Tsys val) const
{
  cv::Mat division = _mat / val;
  return MatT<Tsys>(division);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator+(Tsys val) const
{
  cv::Mat addition = _mat + val;
  return MatT<Tsys>(addition);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::operator-(Tsys val) const
{
  cv::Mat subtraction = _mat - val;
  return MatT<Tsys>(subtraction);
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::abs() const
{
  return MatT<Tsys>(cv::abs(_mat));
}

template <typename Tsys>
MatT<Tsys> MatT<Tsys>::transpose() const
{
  return MatT<Tsys>(_mat.t());
}

// WARNING: MxN Tsys values will frequently not fit inside a single Tsys value.
template <typename Tsys>
Tsys MatT<Tsys>::sum() const
{
  Tsys sum = 0.0;

  for (int i = 0; i < rows(); i++) {
    for (int j = 0; j < cols(); j++) {
      sum += at(i,j);
    }
  }

  return sum;
}

template <typename Tsys>
Tsys MatT<Tsys>::mean() const
{
  return sum() / (cols() * rows());
}

template <typename Tsys>
Tsys MatT<Tsys>::min() const
{
  Tsys min = std::numeric_limits<Tsys>::max();

  for (int i = 0; i < rows(); i++) {
    for (int j = 0; j < cols(); j++) {
      Tsys val = at(i,j);
      min = val < min ? val : min;
    }
  }

  return min;
}

template <typename Tsys>
Tsys MatT<Tsys>::max() const
{
  Tsys max = -std::numeric_limits<Tsys>::max();

  for (int i = 0; i < rows(); i++) {
    for (int j = 0; j < cols(); j++) {
      Tsys val = at(i,j);
      max = val > max ? val : max;
    }
  }

  return max;
}

template <typename Tsys>
Tsys MatT<Tsys>::range() const
{
  return max() - min();
}

template <typename Tsys>
RowT<Tsys> MatT<Tsys>::sums() const
{
  RowT<Tsys> m(cols());
  for (int i = 0; i < cols(); i++) {
    m.at(i) = col(i).sum();
  }
  return m;
}

template <typename Tsys>
RowT<Tsys> MatT<Tsys>::means() const
{
  RowT<Tsys> m(cols());
  for (int i = 0; i < cols(); i++) {
    m.at(i) = col(i).mean();
  }
  return m;
}

template <typename Tsys>
RowT<Tsys> MatT<Tsys>::ranges() const
{
  RowT<Tsys> r(cols());
  for (int i = 0; i < cols(); i++) {
    r.at(i) = col(i).range();
  }
  return r;
}

// TODO: Not actually const!!! The two non-const matrices point to const data.
template <typename Tsys>
std::pair< MatT<Tsys>, MatT<Tsys> > MatT<Tsys>::split(int col) const
{
  cv::Mat split_left = _mat.colRange(0, col);
  cv::Mat split_right = _mat.colRange(col, cols());

  return std::pair< MatT<Tsys>, MatT<Tsys> >(split_left, split_right);
}
  
template <typename Tsys>
MatT<Tsys> MatT<Tsys>::concat_vertical(const MatT<Tsys> &other) const
{
  assert((cols() == other.cols()) && "Dimension mismatch");
  cv::Mat v;
  vconcat(_mat, other.mat(), v);
  return MatT<Tsys>(v);
}
  
template <typename Tsys>
MatT<Tsys> MatT<Tsys>::concat_horizontal(const MatT<Tsys> &other) const
{
  assert((rows() == other.rows()) && "Dimension mismatch");
  cv::Mat h;
  hconcat(_mat, other.mat(), h);
  return MatT<Tsys>(h);
}

template <typename TsysIn, typename TsysOut>
MatT<TsysOut> conversion_helper(const MatT<TsysIn> &in)
{
  int TcvOut = MatT<TsysOut>::Tcv;
  cv::Mat temp = cv::Mat::zeros(in.rows(), in.cols(), TcvOut);
  in.mat().convertTo(temp, TcvOut);
  return MatT<TsysOut>(temp);
}

template <typename Tsys>
Mat32F MatT<Tsys>::convert32() const
{
  return conversion_helper<Tsys, float>(*this);
}

template <typename Tsys>
Mat64F MatT<Tsys>::convert64() const
{
  return conversion_helper<Tsys, double>(*this);
}

Mat32F load_csv32(const std::string &file, int skip_rows=0);
Mat64F load_csv64(const std::string &file, int skip_rows=0);

template<typename Tsys>
std::ostream& operator<< (std::ostream &s, const MatT<Tsys> &m) {
  return s << m.mat(); // Just re-use cv::Mat::operator<<.
}

template <typename Tsys>
RowT<Tsys>::RowT(std::initializer_list<Tsys> l) : MatT<Tsys>(1, l.size())
{
  int index = 0;
  for (auto i : l) at(index++) = i;
}

template <typename Tsys>
int RowT<Tsys>::search(Tsys val) const
{
  for (int i = 0; i < MatT<Tsys>::cols(); i++) {
    if (at(i) == val) return i;
  }
  return -1;
}

template <typename Tsys>
int RowT<Tsys>::min_index() const
{
  return search(MatT<Tsys>::min());
}

template <typename Tsys>
int RowT<Tsys>::max_index() const
{
  return search(MatT<Tsys>::max());
}

template <typename Tsys>
ColT<Tsys>::ColT(std::initializer_list<Tsys> l) : MatT<Tsys>(l.size(), 1)
{
  int index = 0;
  for (auto i : l) at(index++) = i;
}

#endif // _MAT_HH
