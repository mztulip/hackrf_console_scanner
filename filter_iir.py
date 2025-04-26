import numpy as np
from scipy.signal import iirfilter, freqz, freqs, lfilter
from pylab import figure, clf, plot, xlabel, ylabel, xlim, ylim, title, grid, axes, show
# https://schaumont.dyn.wpi.edu/ece4703b20/lecture5.html
sample_rate = 20000000.0
nyq_rate = sample_rate / 2.0

cutoff = 900_000 / nyq_rate
b, a = iirfilter(   N = 15, 
                    Wn = [cutoff],
                    btype='lowpass',
                    analog=False,
                    ftype='cheby2',
                    rp = 10,
                    rs= 50)

print(f"b: {b}")
print(f"a: {a}")
N = len(b)
M = len(a)

w, h = freqz(b, a, worN = 800)

fig2 = figure()
clf()
plot((w/np.pi)*nyq_rate/ 1_000_000, 20 * np.log10(np.maximum(abs(h), 1e-5)), linewidth=2)
xlabel('Frequency (MHz)')
ylabel('Gain')
title('Frequency Response')
ax2 = fig2.gca()
ax2.set_xticks(np.arange(0, 10, 0.5))
# ylim(-0.05, 1.05)
grid(visible=True, which='both')


#------------------------------------------------
# Plot the original and filtered signals.
#------------------------------------------------

nsamples = 800
base_freq = 100_000
t = np.arange(nsamples) / sample_rate

x = 200*np.sin(2*np.pi*1_000_000*t)

# Use lfilter to filter x with the FIR filter.
filtered_x = lfilter(b = b,a = a,x = x)

# The phase delay of the filtered signal.
delay = 0.5 * (M+N-1) / sample_rate


figure(3)
# Plot the original signal.
plot(t, x)
# Plot the filtered signal, shifted to compensate for the phase delay.
plot(t-delay, filtered_x, 'r-')
# Plot just the "good" part of the filtered signal.  The first N-1
# samples are "corrupted" by the initial conditions.
plot(t[M+N-1:]-delay, filtered_x[M+N-1:], 'g', linewidth=4)

show()