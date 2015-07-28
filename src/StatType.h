#ifndef __STATTYPE_H
#define __STATTYPE_H

#include <string>
#include <vector>

#include <cassert>
#include <cmath>

namespace ramulator {

class ScalarStat;
class AverageStat;
class VectorStat;
class AverageVectorStat;
} // namespace ramulator

namespace Stats {

typedef unsigned int size_type;
typedef unsigned int off_type;
typedef double Counter;
typedef double Result;
typedef uint64_t Tick;
typedef std::vector<Counter> VCounter;
typedef std::vector<Result> VResult;
typedef std::numeric_limits<Counter> CounterLimits;

// Flags
const uint16_t init      = 0x00000001;
const uint16_t printable = 0x00000002;
const uint16_t total     = 0x00000010;
const uint16_t pdf       = 0x00000020;
const uint16_t cdf       = 0x00000040;
const uint16_t dist      = 0x00000080;
const uint16_t nozero    = 0x00000100;
const uint16_t nonan     = 0x00000200;

class Flags {
 protected:
  uint16_t flags;
 public:
  Flags(){}
  Flags(uint16_t flags):flags(flags){}
  void operator=(uint16_t _flags){flags = _flags;}
  bool is_total() {return flags & total;}
  bool is_pdf() {return flags & pdf;}
  bool is_nozero() {return flags & nozero;}
  bool is_nonan() {return flags & nonan;}
  bool is_cdf() {return flags & cdf;}
};

class StatBase {
 public:
  virtual void print() = 0;

  virtual size_type size() const = 0;
  virtual bool zero() const = 0;
  virtual void prepare() = 0;
  virtual void reset() = 0;

  virtual VResult vresult() const { return VResult(); };
  virtual Result total() const { return Result(); };
};

class StatList {
 protected:
  std::vector<StatBase*> list;
 public:
  void add(StatBase* stat) {
    list.push_back(stat);
  }
  void printall() {
    for(int i = 0 ; i < list.size() ; ++i) {
      list[i]->prepare();
      list[i]->print();
    }
  }
};

extern StatList statlist;

template<class Derived>
class Stat : public StatBase {
 protected:
  std::string _name;
  std::string _desc;
  int _precision = 1;
  Flags _flags;
  std::string separatorString;
 public:
  Stat() {
    statlist.add(selfptr());
  }
  Derived &self() {return *static_cast<Derived*>(this);}
  Derived *selfptr() {return static_cast<Derived*>(this);}
  Derived &name(const std::string &__name) {
    _name = __name;
    return self();
  };
  Derived &desc(const std::string &__desc) {
    _desc = __desc;
    return self();
  };
  Derived &precision(int __precision) {
    _precision = __precision;
    return self();
  };
  Derived &flags(Flags __flags) {
    _flags = __flags;
    return self();
  };

  template <class GenericStat>
  Derived &prereq(const GenericStat & prereq) {
    // TODO deal with prereq;
    // only print the stat if the prereq is not zero.
    return self();
  }

  Derived &setSeparator(std::string str) {
    separatorString = str;
    return self();
  }
  const std::string& setSeparator() const {return separatorString;}

  size_type size() const { return 0; }

  virtual void print() {};
  virtual void printname() {
    printf("%s\t", _name.c_str());
  }

  virtual void printdesc() {
    printf("\t# %s\n", _desc.c_str());
  }
};

template <class ScalarType>
class ScalarBase: public Stat<ScalarType> {
 public:
  virtual Counter value() const = 0;
  virtual Result result() const = 0;
  virtual Result total() const = 0;

  size_type size() const {return 1;}
  VResult vresult() const {return VResult(1, result());}

  virtual void print() {
    Stat<ScalarType>::printname();
    // TODO deal with flag
    printf("%10.*lf",Stat<ScalarType>::_precision, Stat<ScalarType>::self().result());
    Stat<ScalarType>::printdesc();
  }
};

class ConstValue: public ScalarBase<ConstValue> {
 private:
  Counter _value;
 public:
  ConstValue(Counter __value):_value(__value){}
  Counter value() const {return _value;}
  Result result() const {return (Result)_value;}
  Result total() const {return result();}
  bool zero() const {return _value;}
  void prepare() {}
  void reset() {}
};

class Scalar: public ScalarBase<Scalar> {
 private:
  Counter _value;
 public:
  Scalar():_value(0) {}
  void operator ++ () { ++_value; }
  void operator -- () { --_value; }
  void operator ++ (int) { _value++; }
  void operator -- (int) { _value--; }

  template <typename U>
  void operator = (const U &v) { _value = v; }

  template <typename U>
  void operator += (const U &v) { _value += v;}

  template <typename U>
  void operator -= (const U &v) { _value -= v;}

  Counter value() const {return _value;}
  Result result() const {return (Result)_value;}
  Result total() const {return (Result)_value;}

  virtual bool zero() const {return _value == Counter();}
  void prepare() {}
  void reset() {_value = Counter();}

};

extern Tick curTick;

class Average: public ScalarBase<Average> {
 private:
  Counter current;
  Tick lastReset;
  Result total_val;
  Tick last;
 public:
  Average():current(0), lastReset(0), total_val(0), last(0){}

  void set(Counter val) {
    total_val += current * (curTick - last);
    last = curTick;
    current = val;
  }
  void inc(Counter val) {
    set(current + val);
  }
  void dec(Counter val) {
    set(current - val);
  }

  bool zero() const { return total_val == 0.0; }
  void prepare() {
    total_val += current * (curTick - last);
    last = curTick;
  }
  void reset() {
    total_val = 0.0;
    last = curTick;
    lastReset = curTick;
  }

  Counter value() const { return current; }
  Result result() const {
    assert(last == curTick);
    return (Result)(total_val + current)/ (Result)(curTick - lastReset + 1);
  }
  Result total() const {return result();}
};

template<class Derived, class Element>
class VectorBase: public Stat<Derived> {
 private:
  size_type _size;
  std::vector<Element> data;

 public:
  void init(size_type __size) {
    _size = __size;
    data.resize(size());
  }
  size_type size() const {return _size;}
  // Copy the values to a local vector and return a reference to it.
  void value(VCounter& vec) const {
    vec.resize(size());
    for (off_type i = 0 ; i < size() ; ++i) {
      vec[i] = data[i].value();
    }
  }
  // Copy the results to a local vector and return a reference to it.
  void result(VResult& vec) const {
    vec.resize(size());
    for (off_type i = 0 ; i < size() ; ++i) {
      vec[i] = data[i].result();
    }
  }

  Result total() const {
    Result sum = 0.0;
    for (off_type i = 0 ; i < size() ; ++i) {
      sum += data[i].result();
    }
    return sum;
  }

  VResult vresult() const {return VResult(data);}

  bool check() const {
    // We don't separate storage and access as gem5 does.
    // So here is always true.
    return true;
  }
  Element &operator[](off_type index) {
    assert(index >= 0 && index < size());
    return data[index];
  }

  bool zero() const {
    for (off_type i = 0 ; i < size() ; ++i) {
      if (data[i]->zero()) {
        return false;
      }
      return true;
    }
  }
  void prepare() {
    for (off_type i = 0 ; i < size() ; ++i) {
      data[i].prepare();
    }
  }
  void reset() {
    for (off_type i = 0 ; i < size() ; ++i) {
      data[i].reset();
    }
  }
  void print() {
    Stat<Derived>::printname();
    printf("%10.*lf", Stat<Derived>::_precision, total());
    Stat<Derived>::printdesc();
  }
};

class Vector: public VectorBase<Vector, Scalar> {
};

class AverageVector: public VectorBase<AverageVector, Average> {
};

class Distribution: public Stat<Distribution> {
 private:
  // Parameter part:
  Counter param_min;
  Counter param_max;
  Counter param_bucket_size;
  Counter param_buckets;

  // The minimum value to track
  Counter min_track;
  // The maximum value to track
  Counter max_track;
  // The number of entries in each bucket
  Counter bucket_size;

  Counter min_val;
  Counter max_val;
  // The number of values sampled less than min
  Counter underflow;
  // The number of values sampled more than max
  Counter overflow;
  // The current sum
  Counter sum;
  // The sum of squares
  Counter squares;
  // The number of samples
  Counter samples;
  // Counter for each bucket
  VCounter cvec;

 public:
  void init(Counter min, Counter max, Counter bkt) {
    param_min = min;
    param_max = max;
    param_bucket_size = bkt;
    param_buckets = (size_type)ceil((max - min + 1.0) / bkt);

    reset();
  }
  void sample(Counter val, int number) {
    if (val < min_track)
      underflow += number;
    else if (val > max_track)
      overflow += number;
    else {
      size_type index =
          (size_type)std::floor((val - min_track) / bucket_size);
      assert(index < size());
      cvec[index] += number;
    }

    if (val < min_val)
      min_val = val;

    if (val > max_val)
      max_val = val;

    sum += val * number;
    squares += val * val * number;
    samples += number;
  }

  size_type size() const {return cvec.size();}
  bool zero() const {
    return samples == Counter();
  }
  void prepare() {};
  void reset() {
    min_track = param_min;
    max_track = param_max;
    bucket_size = param_bucket_size;

    min_val = CounterLimits::max();
    max_val = CounterLimits::min();
    underflow = Counter();
    overflow = Counter();

    size_type _size = cvec.size();
    for (off_type i = 0 ; i < _size ; ++i) {
      cvec[i] = Counter();
    }

    sum = Counter();
    squares = Counter();
    samples = Counter();
  };
};

class Histogram: public Stat<Histogram> {
 private:
  size_type param_buckets;

  Counter min_bucket;
  Counter max_bucket;
  Counter bucket_size;

  Counter sum;
  Counter logs;
  Counter squares;
  Counter samples;
  VCounter cvec;

 public:
  Histogram(size_type __buckets) {
    init(__buckets);
  }
  void init(size_type __buckets) {
    cvec.resize(__buckets);
    param_buckets = __buckets;
    reset();
  }

  void grow_up();
  void grow_out();
  void grow_convert();
  void add(Histogram* hs);

  bool zero() const {
    return samples == Counter();
  }
  void prepare() {}
  void reset() {
    min_bucket = 0;
    max_bucket = param_buckets - 1;
    bucket_size = 1;

    size_type size = param_buckets;
    for (off_type i = 0 ; i < size ; ++i) {
      cvec[i] = Counter();
    }

    sum = Counter();
    squares = Counter();
    samples = Counter();
    logs = Counter();
  }

  size_type size() const {return param_buckets;}
};

class StandardDeviation: public Stat<StandardDeviation> {
 private:
  Counter sum;
  Counter squares;
  Counter samples;

 public:
  StandardDeviation():sum(Counter()), squares(Counter()),
      samples(Counter()) {}
  void sample(Counter val, int number) {
    Counter value = val * number;
    sum += value;
    squares += value * value;
    samples += number;
  }
  size_type size() const {return 1;}
  bool zero() const {return samples == Counter();}
  void prepare() {}
  void reset() {
    sum = Counter();
    squares = Counter();
    samples = Counter();
  }
};

class AverageDeviation: public Stat<AverageDeviation> {
 private:
  Counter sum;
  Counter squares;

 public:
  AverageDeviation():sum(Counter()), squares(Counter()) {}
  void sample(Counter val, int number) {
    Counter value = val * number;
    sum += value;
    squares += value * value;
  }
  size_type size() const {return 1;}
  bool zero() const {return sum == Counter();}
  void prepare() {}
  void reset() {
    sum = Counter();
    squares = Counter();
  }
};

class Op {
 private:
  std::string opstring;
 public:
  Op() {}
  Op(std::string __opstring):opstring(__opstring){}
  Result operator() (Result r) const {
    if (opstring == "-") {
      return -r;
    } else {
      assert("Unary operation can only be unary negation." && false);
    }
  }
  Result operator() (Result l, Result r) const {
    if (opstring == "+") {
      return l + r;
    } else if (opstring == "-") {
      return l - r;
    } else if (opstring == "*") {
      return l * r;
    } else if (opstring == "/") {
      assert(fabs(r) > 1e-8 || "divide zero error");
      return l / r;
    } else {
      assert("invalid binary opstring " && false);
    }
  }
};

// Node for constructing Formula
class Temp {
 private:
  Op op;
  std::vector<Temp*> operands;
  const StatBase* leaf;
  mutable VResult vres;
 public:
  Temp(Temp* val, std::string  __op):op(__op), leaf(nullptr) {
    operands.push_back(val);
  }
  Temp(Temp* l, Temp* r, std::string __op):op(__op), leaf(nullptr) {
    operands.push_back(l);
    operands.push_back(r);
  }
  Temp(const Temp& val, std::string  __op):op(__op), leaf(nullptr) {
    operands.push_back(new Temp(val));
  }
  Temp(const Temp& l, const Temp& r, std::string __op):op(__op), leaf(nullptr) {
    operands.push_back(new Temp(l));
    operands.push_back(new Temp(r));
  }
  Temp(const Scalar &s):op(), leaf(&s) {}
  Temp(const Average &s):op(), leaf(&s) {}
  Temp(const Vector &s):op(), leaf(&s) {}
  Temp(const AverageVector &s):op(), leaf(&s) {}
  Temp(const ramulator::ScalarStat &s);
  Temp(const ramulator::AverageStat &s);
  Temp(const ramulator::VectorStat &s);
  Temp(const ramulator::AverageVectorStat &s);
  Temp(signed char value):
    op(), leaf(new ConstValue(value)) {}
  Temp(unsigned char value):
    op(), leaf(new ConstValue(value)) {}
  Temp(signed short value):
    op(), leaf(new ConstValue(value)) {}
  Temp(unsigned short value):
    op(), leaf(new ConstValue(value)) {}
  Temp(signed int value):
    op(), leaf(new ConstValue(value)) {}
  Temp(unsigned int value):
    op(), leaf(new ConstValue(value)) {}
  Temp(signed long value):
    op(), leaf(new ConstValue(value)) {}
  Temp(unsigned long value):
    op(), leaf(new ConstValue(value)) {}
  Temp(signed long long value):
    op(), leaf(new ConstValue(value)) {}
  Temp(unsigned long long value):
    op(), leaf(new ConstValue(value)) {}
  Temp(float value):
    op(), leaf(new ConstValue(value)) {}
  Temp(double value):
    op(), leaf(new ConstValue(value)) {}

  VResult result() const;
  Result total() const;
  size_type size() const;

  void destroy_descendants() {
    for (off_type i = 0 ; i < operands.size() ; ++i) {
      operands[i]->destroy_descendants();
      delete operands[i];
    }
  }
};


class Formula: public Stat<Formula> {
 private:
   std::shared_ptr<Temp> root;
 public:
  Formula() {}
  ~Formula() {
    root->destroy_descendants();
  }
  Formula(Temp r) {
    root = std::shared_ptr<Temp>(new Temp(r));
  }
  Formula &operator=(Temp r) {
    if (root != nullptr) {
      *root = r;
    } else {
      root = std::shared_ptr<Temp>(new Temp(r));
    }
    return self();
  }
  Formula &operator+=(Temp r) {
    Temp* rvalue = new Temp(r);
    root = std::shared_ptr<Temp>(new Temp(root.get(), rvalue, "+")) ;
    return self();
  }
  Formula &operator/=(Temp r) {
    Temp* rvalue = new Temp(r);
    root = std::shared_ptr<Temp>(new Temp(root.get(), rvalue, "/"));
    return self();
  }
  void result(VResult &vec) const {
    VResult rvec = root->result();
    vec.resize(rvec.size());
    for (off_t i = 0 ; i < vec.size() ; ++i) {
      vec[i] = rvec[i];
    }
  }
  Result total() const {
    if (root) {
      return root->total();
    } else {
      return 0;
    };
  }
  size_type size() const {
    if (root) {
      return root->size();
    } else {
      return 0;
    }
  }
  bool zero() const {return root == nullptr;}
  void prepare() {}
  void reset() {
    root.reset();
  }

  void print() {
    Stat<Formula>::printname();
    printf("%10.*lf", Stat<Formula>::_precision, total());
    Stat<Formula>::printdesc();
  }
};

} // namespace Stats

namespace ramulator {

Stats::Temp operator+ (const Stats::Temp &l, const Stats::Temp &r);
Stats::Temp operator- (const Stats::Temp &r);

Stats::Temp operator- (const Stats::Temp &l, const Stats::Temp &r);

Stats::Temp operator* (const Stats::Temp &l, const Stats::Temp &r);
Stats::Temp operator/ (const Stats::Temp &l, const Stats::Temp &r);
}  // namespace ramulator
#endif