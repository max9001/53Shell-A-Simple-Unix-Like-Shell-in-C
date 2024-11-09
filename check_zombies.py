# check_zombies.py
# Counts the current number of zombie processes.

import psutil

def count_zombies():
    count = 0
    for process in psutil.process_iter(['pid', 'status']):
        if process.info['status'] == psutil.STATUS_ZOMBIE:
            count += 1
    return count

if __name__ == "__main__":
    num_zombies = count_zombies()
    print(f"Number of zombie processes: {num_zombies}")
