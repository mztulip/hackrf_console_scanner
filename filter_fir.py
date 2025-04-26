#!python

from numpy import cos, sin, pi, absolute, arange, log10, maximum
from scipy.signal import kaiserord, lfilter, firwin, freqz
from pylab import figure, clf, plot, xlabel, ylabel, xlim, ylim, title, grid, axes, show


#------------------------------------------------
# Create a signal for demonstration.
#------------------------------------------------

sample_rate = 20_000_000.0


#------------------------------------------------
# Create a FIR filter and apply it to x.
#------------------------------------------------

# The Nyquist rate of the signal.
nyq_rate = sample_rate / 2.0

# The desired width of the transition from pass to stop,
# relative to the Nyquist rate.  We'll design the filter
# with a ... Hz transition width.
width = 200000.0/nyq_rate

# The desired attenuation in the stop band, in dB.
ripple_db = 70.0

# Compute the order and Kaiser parameter for the FIR filter.
N, beta = kaiserord(ripple_db, width)

# The cutoff frequency of the filter.
cutoff_hz = 400_000.0

# Use firwin with a Kaiser window to create a lowpass FIR filter.
taps = firwin(N, cutoff_hz/nyq_rate, window=('kaiser', beta))
print(f"#define IIRFilter_TAP_NUM {len(taps)}")
print("static float filter_taps[IIRFilter_TAP_NUM] = {")
for tap in taps:
    print(f"{tap},")
print("};")

#------------------------------------------------
# Plot the FIR filter coefficients.
#------------------------------------------------

# figure(1)
# plot(taps, 'bo-', linewidth=2)
# title('Filter Coefficients (%d taps)' % N)
# grid(True)

#------------------------------------------------
# Plot the magnitude response of the filter.
#------------------------------------------------

fig2 = figure(2)
clf()
w, h = freqz(taps, worN=8000)
# plot((w/pi)*nyq_rate, absolute(h), linewidth=2)
plot((w/pi)*nyq_rate/ 1_000_000, 20 * log10(maximum(abs(h), 1e-5)), linewidth=2)
xlabel('Frequency (MHz)')
ylabel('Gain')
title('Frequency Response')
ax2 = fig2.gca()
ax2.set_xticks(arange(0, 10, 0.5))
# ylim(-0.05, 1.05)
grid(visible=True, which='both')

#To jest do niczego nie potrzebne bo trzeba limity dobrze poustawiać
# # Upper inset plot.
# ax1 = axes([0.42, 0.6, .45, .25])
# plot((w/pi)*nyq_rate, absolute(h), linewidth=2)
# xlim(0,8.0)
# ylim(0.9985, 1.001)
# grid(True)

# # Lower inset plot
# ax2 = axes([0.42, 0.25, .45, .25])
# plot((w/pi)*nyq_rate, absolute(h), linewidth=2)
# xlim(12.0, 20.0)
# ylim(0.0, 0.0025)
# grid(True)

#------------------------------------------------
# Plot the original and filtered signals.
#------------------------------------------------

nsamples = 400
base_freq = 100_000
t = arange(nsamples) / sample_rate
# x = cos(2*pi*base_freq*t) + 0.2*sin(2*pi*2.5*base_freq*t+0.1) + \
#         0.2*sin(2*pi*base_freq*15.3*t) + 0.1*sin(2*pi*16.7*base_freq*t + 0.1) + \
#             0.1*sin(2*pi*base_freq*23.45*t+.8) 

x = 200*sin(2*pi*1_000_000*t)

# Use lfilter to filter x with the FIR filter.
filtered_x = lfilter(taps, 1.0, x)

# The phase delay of the filtered signal.
delay = 0.5 * (N-1) / sample_rate



figure(3)
# Plot the original signal.
plot(t, x)
# Plot the filtered signal, shifted to compensate for the phase delay.
plot(t-delay, filtered_x, 'r-')
# Plot just the "good" part of the filtered signal.  The first N-1
# samples are "corrupted" by the initial conditions.
plot(t[N-1:]-delay, filtered_x[N-1:], 'g', linewidth=4)

# xlabel('t')
# grid(True)

show()