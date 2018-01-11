#ifndef _KNN_HH
#define _KNN_HH

#include "ml_data.hh"
#include "utils.hh"
#include <vector>

class KNN {
private:
  std::vector<KNearest*> knns;

public:
  KNN(const Data &data);
  ~KNN();

  Data fill_gaps(const Data &data, int k) const;

private:
  void build(const Data &data, int label_index);
};

#endif // _KNN_HH
