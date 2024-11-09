# num_procs.py
# Prints the number of processes of the provided name which are running.

import sys
import psutil

def count_processes(process_name):
    count = 0
    for process in psutil.process_iter(['pid', 'name']):
        if process.info['name'] == process_name:
            count += 1
    return count

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 num_procs.py <process_name>")
        sys.exit(1)

    process_name = sys.argv[1]

    try:
        processes = [p.info for p in psutil.process_iter(['pid', 'name']) if process_name in p.info['name']]
        num_processes = len(processes)

        if num_processes > 0:
            print(f"Number of processes with name '{process_name}': {num_processes}")
            print("PID\tName")
            for process in processes:
                print(f"{process['pid']}\t{process['name']}")
        else:
            print(f"No processes found with name '{process_name}'.")
    except psutil.Error as e:
        print(f"Error: {e}")

