#include "transform.hh"
#include <algorithm>

// Yes, we must provide an implementation of this destructor even though it is
// pure-virtual -- yes, this seems broken to me as well.  The reason this
// required is that parent destructors are *always* called, even if they are
// pure virtual, so the absence of an implementation is a link error.
Transform2::~Transform2() {}

Mat32F TransformShift::apply(const Mat32F &in) const
{
  assert((in.cols() == shift.cols()) && "Number of columns do not match.");
  Mat32F shifted(in.rows(), in.cols());
  for (int i = 0; i < in.rows(); i++) {
    for (int j = 0; j < in.cols(); j++) {
      shifted.at(i,j) = in.at(i,j) + shift.at(j);
    }
  }
  return shifted;
}

Mat32F TransformScale::apply(const Mat32F &in) const
{
  assert((in.cols() == scale.cols()) && "Number of columns do not match.");
  Mat32F scaled(in.rows(), in.cols());
  for (int i = 0; i < in.rows(); i++) {
    for (int j = 0; j < in.cols(); j++) {
      scaled.at(i,j) = in.at(i,j) * scale.at(j);
      assert(std::isfinite(scaled.at(i,j))
             && "Exceeded floating point accuracy.");
    }
  }
  return scaled;
}

Mat32F TransformProject::apply(const Mat32F &in) const
{
  assert((in.cols() == input_dimension()) && "Projection does not apply.");
  Mat32F projected_t = projection * in.transpose();
  Mat32F projected = projected_t.transpose();
  return projected;
}

void TransformManager::clear()
{
  _transform.clear();
}

void TransformManager::add(Transform2Ptr t)
{
  assert((_transform.empty() || (output_dimension() == t->input_dimension()))
         && "Transformation can not be appended due to dimension mismatch");
  _transform.push_back(t);
}

void TransformManager::add(const TransformManager &tm)
{
  for (auto t : tm._transform) {
    add(t);
  }
}

int TransformManager::input_dimension() const
{
  if (_transform.empty()) return -1;
  return _transform[0]->input_dimension();
}

int TransformManager::output_dimension() const
{
  if (_transform.empty()) return -1;
  return _transform.back()->output_dimension();
}

int TransformManager::length() const
{
  return _transform.size();
}

Mat32F TransformManager::apply(const Mat32F &data) const
{
  Mat32F transformed = data;
  for (auto t : _transform) {
    transformed = t->apply(transformed);
  }
  return transformed;
}

Row32F TransformManager::apply(const Row32F &data) const
{
  const Mat32F &mat = data;
  return apply(mat).row(0);
}

TransformProjectPtr BuildTransformCut(int dim, const std::set<int> &c) {
  assert((*std::min_element(c.begin(), c.end()) >= 0)
         && "Cut element outside matrix dimensions.");
  assert((*std::max_element(c.begin(), c.end()) < dim)
         && "Cut element outside matrix dimensions.");
  assert((dim > (signed)c.size()) && "Can't cut all elements from matrix.");

  int rowIdx = 0;
  Mat32F mCut(dim - c.size(), dim);
  for (int i = 0; i < dim; i++) {
    if (c.count(i) == 0) {
      mCut.at(rowIdx++, i) = 1.0;
    }
  }
  assert((rowIdx == mCut.rows()) && "Not all rows were initialised.");

  return std::make_shared<TransformProject>(mCut);
}

void TransformManager::save(std::string& file) const
{
	cv::FileStorage fs(file.c_str(), cv::FileStorage::WRITE);

	fs << "numTransforms" << (int)_transform.size();
	fs << "transforms" << "[";
	for (auto t : _transform)
		fs << "{:" << "type" << t->type() << "data" << t->data() << "}";
	fs << "]";
}

void TransformManager::load(std::string& file)
{
	cv::FileStorage fs(file.c_str(), cv::FileStorage::READ);

	_transform.resize((int)fs["numTransforms"]);
	cv::FileNode transforms = fs["transforms"];
	size_t i = 0;
	for(cv::FileNodeIterator fnode = transforms.begin();
			fnode != transforms.end();
			fnode++, i++)
	{
		Transform2Ptr trans;
		cv::Mat data;

		std::string type = (*fnode)["type"];
		(*fnode)["data"] >> data;

		if(type == "shift")
			trans = TransformShiftPtr(new TransformShift(Row32F(data)));
		else if(type == "scale")
			trans = TransformScalePtr(new TransformScale(Row32F(data)));
		else
			trans = TransformProjectPtr(new TransformProject(Mat32F(data)));
		_transform[i] = trans;
	}
}

