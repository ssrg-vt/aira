#ifndef _PREDICT_HH
#define _PREDICT_HH

#include "mat.hh"
#include "data.hh"
#include <string>
#include <memory>

class PredictionManager {
protected:
  TransformManager transform;
  bool higher; // Whether higher values 'better' or not.

  PredictionManager() : transform(TransformManager()), higher(false) {}
  virtual ~PredictionManager() = 0;

public:
  virtual int train(const DataManager &data) = 0;
  virtual void load(const std::string &file) = 0;

  int input_dimension() const { return transform.input_dimension(); }
  virtual void describe(std::ostream &s) const = 0;

  virtual int choose(const Row32F &features) const = 0;
	virtual Row32F predict(const Row32F &features) const;
  virtual void save(const std::string &file) const = 0;
};

class RegressionManager : public PredictionManager {
protected:
  RegressionManager() : PredictionManager() {}

public:
  virtual Row32F predict(const Row32F &features) const = 0;
  virtual int choose(const Row32F &features) const;
};
typedef std::shared_ptr<RegressionManager> RegressionManagerPtr;

// Wrap N 1-output regression models to produce an N-way classifier.
class RegressionMulti : public RegressionManager {
private:
  RegressionManagerPtr manager;
  std::vector<Row32F> extras;

public:
  RegressionMulti(RegressionManagerPtr rm) : RegressionManager(), manager(rm) {}

  void add(const Row32F &extra);

  virtual int train(const DataManager &data);
  virtual void load(const std::string &file);

  virtual void describe(std::ostream &s) const ;

  virtual Row32F predict(const Row32F &features) const;
  virtual void save(const std::string &file) const;
};

class NeuralNetwork : public RegressionManager {
private:
  const CvANN_MLP *nn; // CvANN_MLP has a broken copy-constructor, use pointers.
  const int max_iter;
  const double epsilon;
  const int type;
  const double rate;
  const double momentum;
  const Col32D &middle_layers;

public:
  NeuralNetwork(int mi, double e, int t, double r, double m, const Col32D &ml)
    : RegressionManager(), nn(nullptr), max_iter(mi), epsilon(e), type(t),
      rate(r), momentum(m), middle_layers(ml) {}
	NeuralNetwork()
		: RegressionManager(), nn(nullptr), max_iter(0), epsilon(0.0), type(0),
			rate(0.0), momentum(0.0), middle_layers(Col32D(1)) {}
  virtual ~NeuralNetwork() { delete nn; }

  virtual int train(const DataManager &data);
  virtual void load(const std::string &file);

  virtual void describe(std::ostream &s) const;

  virtual Row32F predict(const Row32F &features) const;
  virtual void save(const std::string &file) const;
};

class SupportVectorMachineC : public PredictionManager {
private:
  const CvSVM *svm;
  const int max_iter;
  const double epsilon;
  const int folds;
  const int type;
  const int kernel;

public:
  SupportVectorMachineC(int i, double e, int f, int t, int k)
    : PredictionManager(), svm(nullptr), max_iter(i), epsilon(e), folds(f),
      type(t), kernel(k)
    {}
  virtual ~SupportVectorMachineC() { delete svm; }

  virtual int train(const DataManager &data);
  virtual void load(const std::string &file);

  virtual void describe(std::ostream &s) const;

  virtual int choose(const Row32F &features) const;
  virtual void save(const std::string &file) const;
};

class DecisionTreeC : public PredictionManager {
private:
  const CvDTree* dtree;
  const int max_depth;
  const int min_sample_count;
  const int max_categories;

  void getHeightInternal(const CvDTreeNode* curNode, int &curHeight);

public:
  DecisionTreeC(int d, int sc, int c)
    : PredictionManager(), dtree(nullptr), max_depth(d), min_sample_count(sc),
      max_categories(c) {}
  virtual ~DecisionTreeC() { delete dtree; }

  virtual int train(const DataManager &data);
  virtual void load(const std::string &file);

  virtual void describe(std::ostream &s) const;

  virtual int choose(const Row32F &features) const;
  virtual void save(const std::string &file) const;

  int getHeight();
};

#endif // _PREDICT_HH
