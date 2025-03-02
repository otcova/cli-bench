#include <sys/ioctl.h>
#include <unistd.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// Print config
static int significant_digits = 3;

struct MetricPrefix {
  double scale = 1;
  const char* prefix = "";
};

const MetricPrefix unicode_scales[] = {
    {1e9, "n"},  {1e6, "µ"},  {1e3, "m"},  {1, ""},
    {-1e3, "k"}, {-1e6, "M"}, {-1e9, "G"},
};

const MetricPrefix ascii_scales[] = {
    {1e9, "n"},  {1e6, "u"},  {1e3, "m"},  {1, ""},
    {-1e3, "k"}, {-1e6, "M"}, {-1e9, "G"},
};

inline string format(double x, MetricPrefix scale = {1, ""}) {
  double value = x * scale.scale;

  int order = static_cast<int>(floor(log10(fabs(value))));
  int decimalPlaces = significant_digits - order - 1;
  decimalPlaces = (decimalPlaces < 0) ? 0 : decimalPlaces;

  std::ostringstream oss;
  oss << fixed << setprecision(decimalPlaces) << value;
  oss << ' ' << scale.prefix;
  return oss.str();
}

inline int get_terminal_width() {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
    return 100;
  return w.ws_col;
}

struct Column {
  int max_width;
  vector<string> data;

  void push(const string& value) {
    if (max_width < value.size())
      max_width = value.size();
    data.push_back(value);
  }

  void print_row(int index) {
    constexpr int MARGIN = 4;
    int correction = (data[index].find("µ") != string::npos) +
                     (data[index].find("±") != string::npos);

    cout << left << setw(max_width + MARGIN + correction) << data[index];
  }
};

class Table {
  vector<Column> columns;

 public:
  Table(vector<const char*> header) : columns(header.size()) {
    for (int col = 0; col < header.size(); ++col)
      push(col, header[col]);
  }

  void push(int column, string value) {
    if (column >= 0)
      columns[column].push(value);
  }

  void fill_row(int row) {
    ++row;  // Acount for the header
    for (Column& column : columns) {
      while (column.data.size() <= row)
        column.push("");
    }
  }

  void print() {
    int width = get_terminal_width();

    for (int r = 0; r < columns[0].data.size(); ++r) {
      for (int c = 0; c < columns.size() - 1; ++c)
        columns[c].print_row(r);
      cout << columns.back().data[r] << endl;
    }
  }
};
