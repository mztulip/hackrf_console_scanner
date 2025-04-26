#ifndef IIRFilter_H_
#define IIRFilter_H_

#include "filter_coeffs.h"

typedef struct {
  float history_i[IIRFilter_TAP_NUM];
  float history_q[IIRFilter_TAP_NUM];
  int last_index;
} IIRFilter;

void IIRFilter_init(IIRFilter* f);
void IIRFilter_put(IIRFilter* f, float input_i, float input_q);
void IIRFilter_get(IIRFilter* f, float *i_out, float *q_out);

#endif