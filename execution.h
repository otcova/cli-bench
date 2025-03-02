#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

static int devnull = 1;

inline int get_devnull() {
  if (devnull == 1)
    devnull = open("/dev/null", O_WRONLY);
  if (devnull < 0) {
    perror("Unable to open /dev/null");
    exit(1);
  }
  return devnull;
}

class Timer {
  std::chrono::high_resolution_clock::time_point start;

 public:
  Timer() : start(chrono::high_resolution_clock::now()) {}

  double seconds() {
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    return duration.count();
  }
};

inline std::string executable_path(const std::string& executable) {
  char resolved_path[PATH_MAX];

  // Check if the given executable contains a '/' (absolute or relative path)
  if (executable.find('/') != std::string::npos) {
    if (realpath(executable.c_str(), resolved_path)) {
      return std::string(resolved_path);
    }
  } else {
    // Search in PATH environment variable
    const char* path_env = getenv("PATH");
    if (!path_env)
      return "";

    std::string path_dirs(path_env);
    size_t pos = 0;
    while ((pos = path_dirs.find(':')) != std::string::npos) {
      std::string dir = path_dirs.substr(0, pos);
      std::string full_path = dir + "/" + executable;
      if (realpath(full_path.c_str(), resolved_path)) {
        return std::string(resolved_path);
      }
      path_dirs.erase(0, pos + 1);
    }
  }
  return "";
}

class Target {
  string target_name;
  string exe_path;
  vector<const char*> args;
  vector<double> samples;

 public:
  Target(const vector<const char*>& arguments) : args(arguments) {
    if (args.empty() || args.back() == nullptr) {
      cout << "Invalid target argument. Must provide at least an executable"
           << endl;
      exit(1);
    }

    target_name = args[0];
    for (int i = 1; i < args.size(); ++i)
      target_name = target_name + ' ' + args[i];

    exe_path = executable_path(args[0]);
    args[0] = exe_path.c_str();

    args.push_back(nullptr);
  }

  const vector<double>& time_samples() const { return samples; }
  const string& name() const { return target_name; }

  void execute(bool record = true) {
    /*double seconds = execute_system();*/
    double seconds = execute_posix();

    if (record)
      samples.push_back(seconds);
  }

 private:
  // Slower version that does not relly on posix.
  double execute_system() {
    string cmd = name();
    Timer timer;
    int result = system(cmd.c_str());
    double seconds = timer.seconds();

    if (result != 0) {
      cerr << "'" << name() << "' failed to execute." << endl;
      exit(1);
    }

    return seconds;
  }

  double execute_posix() {
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, get_devnull(), STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, get_devnull(), STDERR_FILENO);

    pid_t pid;
    int result = posix_spawn(&pid, (char*)args[0], &actions, NULL,
                             (char**)args.data(), environ);

    if (result != 0) {
      cerr << "'" << name() << "' failed to execute" << endl;
      perror(args[0]);
      exit(1);
    }

    Timer timer;
    int status;
    waitpid(pid, &status, 0);
    return timer.seconds();
  }
};
