import csv
from datetime import datetime

def parse_time(line):
    time_str = line.split()[1] + ' ' + line.split()[2]
    return datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S")

def parse_line(line):
    data = line.split()
    core = int(data[0])
    ipc = float(data[1])
    misses = int(data[2].rstrip('k'))
    llc = float(data[3])
    return [core, ipc, misses, llc]

def main(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:
        reader = infile.readlines()

        fieldnames = ['TIME', 'CORE', 'IPC', 'MISSES', 'LLC']
        writer = csv.DictWriter(outfile, fieldnames=fieldnames)
        writer.writeheader()

        for i in range(0, len(reader), 3):
            time = i/3
            print(reader[i+2])
            core, ipc, misses, llc = parse_line(reader[i + 2])
            writer.writerow({'TIME': time, 'CORE': core, 'IPC': ipc, 'MISSES': misses, 'LLC': llc})

if __name__ == "__main__":
    input_filename = "output"
    output_filename = "output.csv"
    main(input_filename, output_filename)

