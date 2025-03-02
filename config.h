#pragma once
#include <cassert>
#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>
using namespace std;

enum HandleOutliners {
  Keep,
  Remove,
};

struct ColumnsIndexes {
  int name = -1;
  int min_speedup = -1;
  int min_gain = -1;
  int speedup = -1;
  int gain = -1;
  int mean = -1;
  int std = -1;
  int samples = -1;
  int outliers = -1;
};

struct Config {
  bool show_help = false;

  double min_seconds = 2.;
  int min_samples = 5;

  double min_warmup_seconds = .1;
  int min_warmup_samples = 1;

  double confidence = 0.95;

  optional<string> csv_file;

  vector<const char*> column_names;
  ColumnsIndexes column;

  HandleOutliners outliers = HandleOutliners::Remove;

  vector<vector<const char*>> targets;

  bool use_ascii = false;
  bool no_prefix = false;

  void parse_args(int argc, const char* const argv[]) {
    parse_columns({"name", "minSpeedup", "mean", "std", "samples"});

    for (int arg_index = 1; arg_index < argc; ++arg_index) {
      string_view arg = argv[arg_index];

      if (arg.empty())
        continue;

      if (arg[0] != '-' || arg.size() <= 1) {
        cout << "Invalid argument " << arg_index << " '" << arg << "'" << endl;
        exit(1);
      }

      bool double_dash = arg[1] == '-';
      string_view option_name = arg.substr(double_dash ? 2 : 1);

      if (double_dash) {
        if (option_name == "") {
          parse_targets(arg_index + 1, argc, argv);
          break;
        } else if (option_name == "help") {
          show_help = true;
          continue;
        } else if (option_name == "keep-outlier") {
          outliers = HandleOutliners::Keep;
          continue;
        } else if (option_name == "no-prefix") {
          no_prefix = true;
          continue;
        } else if (option_name == "csv") {
          csv_file = "";
          if (argv[arg_index + 1][0] != '-')
            csv_file = argv[++arg_index];
          continue;
        } else if (option_name == "wt") {
          string_view param = argv[++arg_index];
          min_warmup_seconds = parse_double(param);
          continue;
        } else if (option_name == "wn") {
          string_view param = argv[++arg_index];
          min_warmup_samples = parse_uint(param);
          continue;
        } else if (option_name == "cols") {
          string_view param = argv[++arg_index];
          parse_columns(parse_list(param));
          continue;
        } else if (option_name == "conf") {
          string_view param = argv[++arg_index];
          confidence = parse_double(param);
          assert(0.5 <= confidence && confidence < 1.0);
          confidence /= 100.;
          continue;
        }
      }

      parse_switch_options(option_name);

      if (option_name == "t") {
        string_view param = argv[++arg_index];
        min_seconds = parse_double(param);
      } else if (option_name == "n") {
        string_view param = argv[++arg_index];
        min_samples = parse_uint(param);
      } else if (option_name != "") {
        cout << "Unkown option '" << option_name << "'" << endl;
        exit(1);
      }
    }
  }

 private:
  void parse_targets(int arg_index, int argc, const char* const argv[]) {
    targets.emplace_back();
    for (; arg_index < argc; ++arg_index) {
      string_view arg = argv[arg_index];

      if (arg == "--")
        targets.emplace_back();
      else
        targets.back().push_back(argv[arg_index]);
    }
  }

  vector<string_view> parse_list(string_view s) {
    vector<string_view> result;
    constexpr char delimiter = ',';
    size_t start = 0, end;

    while ((end = s.find(delimiter, start)) != string_view::npos) {
      result.push_back(s.substr(start, end - start));
      start = end + 1;  // Move past the delimiter
    }
    result.push_back(s.substr(start));  // Add last token
    return result;
  }

  unsigned int parse_uint(const std::string_view& s) {
    unsigned int value;
    auto [ptr, ec] = from_chars(s.data(), s.data() + s.size(), value);
    if (ec != errc()) {
      cout << "Invalid argument '" << s;
      cout << "' . Expected a positive integer" << endl;
      exit(1);
    }
    return value;
  }

  double parse_double(const std::string_view& s) {
    double value;
    auto [ptr, ec] = from_chars(s.data(), s.data() + s.size(), value);
    if (ec != errc()) {
      cout << "Invalid argument '" << s << "' . Expected a number" << endl;
      exit(1);
    }
    return value;
  }

  void parse_switch_options(string_view& option_name) {
    if (option_name.empty())
      return;
    else if (option_name[0] == 'h')
      show_help = true;
    else if (option_name[0] == 'a')
      use_ascii = true;
    else
      return;

    option_name = option_name.substr(1);
    parse_switch_options(option_name);
  }

  void parse_columns(const vector<string_view>& names) {
    column = ColumnsIndexes();
    column_names.clear();

    for (int index = 0; index < names.size(); ++index) {
      if (names[index] == "name") {
        column.name = index;
        column_names.push_back("Name");
      } else if (names[index] == "minSpeedup") {
        column.min_speedup = index;
        column_names.push_back("Min Speedup");
      } else if (names[index] == "minGain") {
        column.min_gain = index;
        column_names.push_back("Min Gain");
      } else if (names[index] == "speedup") {
        column.speedup = index;
        column_names.push_back("Speedup");
      } else if (names[index] == "gain") {
        column.gain = index;
        column_names.push_back("Gain");
      } else if (names[index] == "mean") {
        column.mean = index;
        column_names.push_back("Mean");
      } else if (names[index] == "std") {
        column.std = index;
        column_names.push_back("Std");
      } else if (names[index] == "samples") {
        column.samples = index;
        column_names.push_back("Samples");
      } else if (names[index] == "outliers") {
        column.outliers = index;
        column_names.push_back("Outliers");
      } else {
        cout << "Invalid column name '" << names[index];
        cout << "' . Expected a positive integer" << endl;
        exit(1);
      }
    }
  }
};

static const char* HELP =
    "Usage: bench [<options>] [-- <command>] [-- <command>] ...\n"
    "Stadistical Options:\n"
    "    --conf <%>       Statistical confidence of the lowerbound.\n"
    "    --keep-outliers  Do not remove outlier.\n"
    "\n"
    "Sampling Options:\n"
    "    -t <secs>    Minimum seconds inverted in taking samples.\n"
    "    -n <num>     Minimum amount of samples.\n"
    "    --wt <secs>  Minimum seconds inverted in the warmup.\n"
    "    --wn <num>   Minimum amount of samples in the warmup.\n"
    "\n"
    "Display Options:\n"
    "    -a              Only use ASCII characters.\n"
    "    --csv [<file>]  Output a table of samples with csv format.\n"
    "    --no-prefix     Do not use metric prefixes (0.012s instead of 12ms)\n"
    "    --cols <list>   Comma separeted list of columns to show. Options:\n"
    "                      name       - Command name\n"
    "                      speedup    - Mean speedup\n"
    "                      minSpeedup - Statistical speedup lowerbound\n"
    "                      gain       - Mean gain\n"
    "                      minGain    - Statistical gain lowerbound\n"
    "                      mean       - Mean time\n"
    "                      std        - Standard deviation\n"
    "                      samples    - Number of usefull samples\n"
    "                      outliers   - Precentage of removed outliners\n"
    "\n"
    "Examples:\n"
    "    > bench -- bash -ic '' -- bash -c '' -- sh -c ''\n"
    "    > bench --cols mean,std -- sleep 1\n";
