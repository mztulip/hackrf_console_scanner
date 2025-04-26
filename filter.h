#ifndef SAMPLEFILTER_H_
#define SAMPLEFILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 20000000 Hz

* 0 Hz - 900000 Hz
  gain = 1
  desired ripple = 2 dB
  actual ripple = 1.4965661813002376 dB

* 1100000 Hz - 10000000 Hz
  gain = 0
  desired attenuation = -30 dB
  actual attenuation = -30.245318656992403 dB

*/

#define SAMPLEFILTER_TAP_NUM 111

typedef struct {
  float history_i[SAMPLEFILTER_TAP_NUM];
  float history_q[SAMPLEFILTER_TAP_NUM];
  int last_index;
} SampleFilter;

void SampleFilter_init(SampleFilter* f);
void SampleFilter_put(SampleFilter* f, float input_i, float input_q);
void SampleFilter_get(SampleFilter* f, float *i_out, float *q_out);

#endif