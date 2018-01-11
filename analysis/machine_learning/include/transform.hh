#ifndef _TRANSFORM_HH
#define _TRANSFORM_HH

#include "mat.hh"
#include <memory>
#include <set>

class Transform2 {
protected:
  virtual ~Transform2() = 0;

public:
  virtual int input_dimension() const = 0;
  virtual int output_dimension() const = 0;
	virtual std::string type() const = 0;
	virtual cv::Mat data() const = 0;
  virtual Mat32F apply(const Mat32F &in) const = 0;
};

class TransformShift : public Transform2 {
private:
  const Row32F shift;

public:
  TransformShift(const Row32F &p) : shift(p) {}
  virtual ~TransformShift() {}
  
  virtual int input_dimension() const { return shift.cols(); }
  virtual int output_dimension() const { return shift.cols(); }
	virtual std::string type() const { return "shift"; }
	virtual cv::Mat data() const { return shift.mat(); }
  virtual Mat32F apply(const Mat32F &in) const;
};

class TransformScale : public Transform2 {
private:
  const Row32F scale;

public:
  TransformScale(const Row32F &p) : scale(p) {}
  virtual ~TransformScale() {}

  virtual int input_dimension() const { return scale.cols(); }
  virtual int output_dimension() const { return scale.cols(); }
	virtual std::string type() const { return "scale"; }
	virtual cv::Mat data() const { return scale.mat(); }
  virtual Mat32F apply(const Mat32F &in) const;
};

class TransformProject : public Transform2 {
private:
  const Mat32F projection;

public:
  TransformProject(const Mat32F &p) : projection(p) {}
  virtual ~TransformProject() {}
  
  virtual int input_dimension() const { return projection.cols(); }
  virtual int output_dimension() const { return projection.rows(); }
	virtual std::string type() const { return "project"; }
	virtual cv::Mat data() const { return projection.mat(); }
  virtual Mat32F apply(const Mat32F &in) const;
};

typedef std::shared_ptr<const Transform2> Transform2Ptr;
typedef std::shared_ptr<const TransformShift> TransformShiftPtr;
typedef std::shared_ptr<const TransformScale> TransformScalePtr;
typedef std::shared_ptr<const TransformProject> TransformProjectPtr;

class TransformManager {
private:
  std::vector<Transform2Ptr> _transform;

public:
  void clear();
  void add(Transform2Ptr t);
  void add(const TransformManager &tm);
  int input_dimension() const;
  int output_dimension() const;
  int length() const;
  Mat32F apply(const Mat32F &features) const;
  Row32F apply(const Row32F &features) const;
	void save(std::string &file) const;
	void load(std::string &file);
};

// Produce a project matrix that will project a 'dim' dimensional matrix to a
// 'dim-c.size()' dimensional matrix.  Columns corresponding to a number in c
// will be cut from the output.
TransformProjectPtr BuildTransformCut(int dim, const std::set<int> &c);

#endif // _TRANSFORM_HH
