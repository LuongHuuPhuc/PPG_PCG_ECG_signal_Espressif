from scipy.signal import firwin

sample_rate = 8000    # Tan so lay mau ban dau (Hz)
cutoff_hz = 450       # Tan so cat (Cutoff frequency)
num_taps = 64         # So luong taps (bac cua bo loc)

# Sinh hệ số bộ lọc:
fir_coeff = firwin(num_taps, cutoff=cutoff_hz, fs=sample_rate)

# In ra mảng hệ số, cách nhau bằng dấu phẩy:
print(', '.join(f'{coef:.6f}' for coef in fir_coeff))
