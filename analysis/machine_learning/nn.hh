#ifndef _NN_HH
#define _NN_HH

#include "ml_data.hh"

class ML {
public:
  virtual void predict(const Mat &in, Mat &out) const = 0;
  virtual void save(const char *fname) const = 0;
};

class NN : public ML {
private:
  const CvANN_MLP *nn; // CvANN_MLP has a broken copy-constructor, use pointers.
  const Transform t;

public:
  NN(const Data &d, int max_iter, double epsilon, double rate, double momentum,
     const Mat &layers) : nn(NULL), t(d.transform)
  {
    build(d, max_iter, epsilon, rate, momentum, layers);
  }
  NN(const Transform &t, const char *fname) : nn(NULL), t(t) { load(fname); }
  ~NN() { delete nn; }

  // Don't use the "return Mat" structure used everywhere in this code as this
  // function must be extremely fast.  So use a reference.
  virtual void predict(const Mat &in, Mat &out) const;

  // Stream a trained NN to an XML file.
  virtual void save(const char *fname) const;

private:
  void build(const Data &data, int max_iter, double epsilon, double rate,
             double momentum, const Mat &layers);
  void load(const char *fname);
};

class SVM_Classifier : public ML {
private:
  const CvSVM *svm;
  const Transform t;

public:
  SVM_Classifier(const Data &d) : svm(NULL), t(d.transform) { build(d); }
  SVM_Classifier(const Transform &t, const char *fname) : svm(NULL), t(t)
  {
    load(fname);
  }
  ~SVM_Classifier() { delete svm; }

  // Don't use the "return Mat" structure used everywhere in this code as this
  // function must be extremely fast.  So use a reference.
  virtual void predict(const Mat &in, Mat &out) const;

  // Stream a trained NN to an XML file.
  virtual void save(const char *fname) const;

private:
  void build(const Data &data);
  void load(const char *fname);
};

#endif // _NN_HH
