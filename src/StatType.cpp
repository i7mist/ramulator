#include "StatType.h"

namespace ramulator {

Stats::Temp operator+ (const Stats::Temp &l, const Stats::Temp &r) {
    return Stats::Temp(l, r, "+");
}

Stats::Temp operator- (const Stats::Temp &r) {
    return Stats::Temp(r, "-");
}

Stats::Temp operator- (const Stats::Temp &l, const Stats::Temp &r) {
    return Stats::Temp(l, r, "-");
}

Stats::Temp operator* (const Stats::Temp &l, const Stats::Temp &r) {
    return Stats::Temp(l, r, "*");
}

Stats::Temp operator/ (const Stats::Temp &l, const Stats::Temp &r) {
    return Stats::Temp(l, r, "/");
}

} /* namespace ramulator */

namespace Stats {

// Statistics list
StatList statlist;

// The smallest timing granularity.
Tick curTick = 0;

void
Histogram::grow_out()
{
    int size = cvec.size();
    int zero = size / 2; // round down!
    int top_half = zero + (size - zero + 1) / 2; // round up!
    int bottom_half = (size - zero) / 2; // round down!

    // grow down
    int low_pair = zero - 1;
    for (int i = zero - 1; i >= bottom_half; i--) {
        cvec[i] = cvec[low_pair];
        if (low_pair - 1 >= 0)
            cvec[i] += cvec[low_pair - 1];
        low_pair -= 2;
    }
    assert(low_pair == 0 || low_pair == -1 || low_pair == -2);

    for (int i = bottom_half - 1; i >= 0; i--)
        cvec[i] = Counter();

    // grow up
    int high_pair = zero;
    for (int i = zero; i < top_half; i++) {
        cvec[i] = cvec[high_pair];
        if (high_pair + 1 < size)
            cvec[i] += cvec[high_pair + 1];
        high_pair += 2;
    }
    assert(high_pair == size || high_pair == size + 1);

    for (int i = top_half; i < size; i++)
        cvec[i] = Counter();

    max_bucket *= 2;
    min_bucket *= 2;
    bucket_size *= 2;
}

void
Histogram::grow_convert()
{
    int size = cvec.size();
    int half = (size + 1) / 2; // round up!
    //bool even = (size & 1) == 0;

    int pair = size - 1;
    for (int i = size - 1; i >= half; --i) {
        cvec[i] = cvec[pair];
        if (pair - 1 >= 0)
            cvec[i] += cvec[pair - 1];
        pair -= 2;
    }

    for (int i = half - 1; i >= 0; i--)
        cvec[i] = Counter();

    min_bucket = -max_bucket;// - (even ? bucket_size : 0);
    bucket_size *= 2;
}

void
Histogram::grow_up()
{
    int size = cvec.size();
    int half = (size + 1) / 2; // round up!

    int pair = 0;
    for (int i = 0; i < half; i++) {
        cvec[i] = cvec[pair];
        if (pair + 1 < size)
            cvec[i] += cvec[pair + 1];
        pair += 2;
    }
    assert(pair == size || pair == size + 1);

    for (int i = half; i < size; i++)
        cvec[i] = Counter();

    max_bucket *= 2;
    bucket_size *= 2;
}

void
Histogram::add(Histogram *hs)
{
    int b_size = hs->size();
    assert(size() == b_size);
    assert(min_bucket == hs->min_bucket);

    sum += hs->sum;
    logs += hs->logs;
    squares += hs->squares;
    samples += hs->samples;

    while(bucket_size > hs->bucket_size)
        hs->grow_up();
    while(bucket_size < hs->bucket_size)
        grow_up();

    for (uint32_t i = 0; i < b_size; i++)
        cvec[i] += hs->cvec[i];
}

void
Histogram::sample(Counter val, int number)
{
    assert(min_bucket < max_bucket);
    if (val < min_bucket) {
        if (min_bucket == 0)
            grow_convert();

        while (val < min_bucket)
            grow_out();
    } else if (val >= max_bucket + bucket_size) {
        if (min_bucket == 0) {
            while (val >= max_bucket + bucket_size)
                grow_up();
        } else {
            while (val >= max_bucket + bucket_size)
                grow_out();
        }
    }

    size_type index =
        (int64_t)std::floor((val - min_bucket) / bucket_size);

    assert(index >= 0 && index < size());
    cvec[index] += number;

    sum += val * number;
    squares += val * val * number;
    logs += log(val) * number;
    samples += number;
}

VResult Temp::result() const {
  if (operands.size() == 0) {
    if (leaf != nullptr) {
      return leaf->vresult();
    } else {
      return VResult();
    }
  } else {
    if (operands.size() == 1) {
      const VResult& v = operands[0]->result();
      vres.resize(v.size());
      for (off_type i = 0 ; i < vres.size() ; ++i) {
        vres[i] = op(v[i]);
      }
      return vres;
    } else if (operands.size() == 2) {
      const VResult &lvec = operands[0]->result();
      const VResult &rvec = operands[1]->result();

      assert(lvec.size() > 0 && rvec.size() > 0);
      assert(lvec.size() == rvec.size() ||
             lvec.size() == 1 || rvec.size() == 1);

      if (lvec.size() == 1 && rvec.size() == 1) {
          vres.resize(1);
          vres[0] = op(lvec[0], rvec[0]);
      } else if (lvec.size() == 1) {
          size_type size = rvec.size();
          vres.resize(size);
          for (off_type i = 0; i < size; ++i)
              vres[i] = op(lvec[0], rvec[i]);
      } else if (rvec.size() == 1) {
          size_type size = lvec.size();
          vres.resize(size);
          for (off_type i = 0; i < size; ++i)
              vres[i] = op(lvec[i], rvec[0]);
      } else if (rvec.size() == lvec.size()) {
          size_type size = rvec.size();
          vres.resize(size);
          for (off_type i = 0; i < size; ++i)
              vres[i] = op(lvec[i], rvec[i]);
      }

      return vres;

    } else {
      assert("Only support unary/binary operation" && false);
    }
  }
}

Result Temp::total() const {
  if (operands.size() == 0) {
    assert(leaf != nullptr);
    return leaf->total();
  } else {
    if (operands.size() == 1) {
      const VResult &vec = this->result();
      Result total = 0.0;
      for (off_type i = 0 ; i < size() ; ++i) {
        total += vec[i];
      }
      return total;
    } else if (operands.size() == 2) {
      const VResult &vec = this->result();
      const VResult &lvec = operands[0]->result();
      const VResult &rvec = operands[1]->result();
      Result total = 0.0;
      Result lsum = 0.0;
      Result rsum = 0.0;

      assert(lvec.size() > 0 && rvec.size() > 0);
      assert(lvec.size() == rvec.size() ||
             lvec.size() == 1 || rvec.size() == 1);

      if (lvec.size() == rvec.size() && lvec.size() > 1) {
        for (off_type i = 0 ; i < size() ; ++i) {
          lsum += lvec[i];
          rsum += rvec[i];
        }
        return op(lsum, rsum);
      }

      for (off_type i = 0 ; i < size() ; ++i) {
        total += vec[i];
      }

      return total;

    } else {
      assert("Only support unary/binary operation...\n" && false);
    }
  }
}

size_type Temp::size() const {
  if (operands.size() == 0) {
    if (leaf) {
      return leaf->size();
    } else {
      return 0;
    }
  } else if (operands.size() == 1) {
    return operands[0]->size();
  } else {
    size_type ls = operands[0]->size();
    size_type rs = operands[1]->size();
    if (ls == 1) {
        return rs;
    } else if (rs == 1) {
        return ls;
    } else {
        assert(ls == rs && "Node vector sizes are not equal");
        return ls;
    }
  }
}

} /* namespace Stats */
