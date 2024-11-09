# 53Shell: Simple Unix-Like Shell in C

This C program implements a simple terminal shell that can run commands, manage background processes, handle input/output redirection, and provide some shell-specific features like history tracking and job management. Below is an overview of the core functionalities this terminal shell offers:

## Running the Shell
- **Build the C program:**
  ```
  make all
  ```
- **Run the C program:**
  ```
  ./bin/53shell
  ```
## Example Usage

- **Run a single command**:
  ```
  ls -l
  ```
- **Run a command with input redirection**:
  ```
  cat < input.txt
  ```
- **Run a command with output redirection**:
  ```
  echo "Hello, World!" > output.txt
  ```
- **Run a command in the background**:
  ```
  sleep 10 &
  ```
- **Check background jobs**:
  ```
  bglist
  ```
- **Run a command with a pipe**:
  ```
  ls | grep "txt"
  ```
  
- **Access command history**:
  ```
  history
  ```
  ```
  !3
  ```
- **Quit the program**:
  ```
  exit
  ```

## Summary of Features:
- **Job Management**: Supports background and foreground processes, job tracking, and process reaping.
- **History**: Command history with re-execution via !1, !2, etc.
- **Redirection and Piping**: Supports input/output redirection and pipelines.
- **Built-in Commands**: cd, exit, estatus, bglist, history.
- **Signal Handling**: Handles SIGCHLD and SIGUSR2 for process termination and reporting.
- **Error Handling**: Robust error checking for system calls and command execution.
