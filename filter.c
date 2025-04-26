#include <stdio.h>
#include "filter.h"

void IIRFilter_init(IIRFilter* f) 
{
  int i;
  for(i = 0; i < IIRFilter_TAP_NUM; ++i)
  {
    f->history_i[i] = 0;
    f->history_q[i] = 0;
  }
  f->last_index = 0;
}

void IIRFilter_put(IIRFilter* f, float input_i, float input_q)
{
  f->history_i[f->last_index] = input_i;
  f->history_q[f->last_index] = input_q;
  f->last_index++;
  if(f->last_index == IIRFilter_TAP_NUM)
    f->last_index = 0;
}

void IIRFilter_get(IIRFilter* f, float *i_out, float *q_out) 
{
    float acc_i = 0;
    float acc_q = 0;
    int index = f->last_index, i;

    for(i = 0; i < IIRFilter_TAP_NUM; ++i) 
    {
        index = index != 0 ? index-1 : IIRFilter_TAP_NUM-1;
        acc_i += f->history_i[index] * filter_taps[i];
        acc_q += f->history_q[index] * filter_taps[i];
    };
    *i_out = acc_i;
    *q_out = acc_q;
}