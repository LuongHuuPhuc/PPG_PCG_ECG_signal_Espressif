import pandas as pd
import matplotlib.pyplot as plt

# Đọc file CSV không có header
file_path = r"D:\Esp-idf\PPG_PCG_ECG_synchro\Data_text\test5.csv" # Đổi thành đường dẫn file của bạn
column_index = 1       # Đổi số cột bạn muốn vẽ (0 là cột đầu tiên)

# Đọc dữ liệu
data = pd.read_csv(file_path, header=None)

# Kiểm tra chỉ số cột có hợp lệ không
if column_index >= data.shape[1]:
    raise ValueError(f"File chỉ có {data.shape[1]} cột, không có cột {column_index}")

# Trích cột cần vẽ
signal = data.iloc[:, column_index]

# Vẽ đồ thị
plt.figure(figsize=(10, 5))
plt.plot(signal, label=f'Column {column_index}')
plt.xlabel('Sample Index')
plt.ylabel('Value')
plt.title(f'Data from Column {column_index}')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
