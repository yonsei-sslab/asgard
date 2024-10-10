import csv
import argparse

selected_rows = [
    'lib/ld-linux-aarch64.so.1',
    'lib/libc.so.6',
    'lib/libdl.so.2',
    'lib/libm.so.6',
    'lib/libpthread.so.0',
    'lib/libgcc_s.so.1',
    'usr/lib/libstdc++.so.6.0.29'
]

def sum_selected_rows(csv_file, selected_rows):
    total_size = 0
    with open(csv_file, 'r', newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            if row['File name'] in selected_rows:
                total_size += int(row['File size'])
    print(f"Total file size for the selected rows: {total_size} bytes")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('csv_file', help='Path to the .csv file')
    args = parser.parse_args()

    sum_selected_rows(args.csv_file, selected_rows)
