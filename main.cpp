#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include "config.h"
#include "execution.h"
#include "statistics.h"
#include "table.h"
using namespace std;

void take_samples(vector<Target>& targets,
                  double min_secs,
                  long min_rep,
                  bool record = true) {
  Timer timer;
  while (min_rep > 0 || timer.seconds() < min_secs) {
    for (Target& target : targets)
      target.execute(record);
    --min_rep;
  }
}

template <typename Vec, typename Mapper>
void print_list(std::ostream& out, const Vec& elements, Mapper f) {
  if (!elements.empty()) {
    f(elements[0]);
    for (int i = 1; i < elements.size(); ++i) {
      out << ", ";
      f(elements[i]);
    }
    out << endl;
  }
}

void export_csv(std::ostream& out, vector<Target>& targets) {
  print_list(out, targets, [&](const Target& t) { out << t.name(); });

  size_t rows = 0;
  for (Target& target : targets)
    rows = max(rows, target.time_samples().size());

  for (size_t i = 0; i < rows; ++i) {
    print_list(out, targets, [&](const Target& t) {
      if (i < t.time_samples().size())
        out << t.time_samples()[i];
    });
  }
}

int main(int argc, const char* argv[]) {
  // Parse
  Config config;
  config.parse_args(argc, argv);

  if (config.show_help || config.targets.empty())
    help();

  vector<Target> targets;
  for (auto& target : config.targets)
    targets.emplace_back(target);

  // Execute
  take_samples(targets, config.min_warmup_seconds, config.min_warmup_samples,
               false);
  take_samples(targets, config.min_seconds, config.min_samples);

  vector<DataSet> sets;
  for (Target& target : targets)
    sets.emplace_back(target.time_samples());

  // Analyze
  int base_index = 0;
  for (int i = 0; i < sets.size(); ++i) {
    if (sets[i].real_mean() > sets[base_index].real_mean())
      base_index = i;
  }

  DataSet& base = sets[base_index];

  Table table({"Name", "Min Speedup", "Min Gain", "Speedup", "Mean", "Std"});

  // Choose display metric prefix
  MetricPrefix scale;
  if (!config.no_prefix) {
    const MetricPrefix* scales =
        config.use_ascii ? ascii_scales : unicode_scales;
    scale = scales[0];
    for (int i = 1; i < sizeof(ascii_scales) / sizeof(MetricPrefix); ++i) {
      MetricPrefix s = scales[i];
      bool ok = true;

      for (DataSet& set : sets) {
        ok = ok && set.real_mean() * s.scale >= 1;
      }

      if (ok)
        scale = s;
      else
        break;
    }
  }

  // Write Samples
  if (config.csv_file) {
    if (config.csv_file->empty())
      export_csv(cout, targets);
    else {
      std::ofstream file(*config.csv_file);
      export_csv(file, targets);
      file.close();
    }
  }

  // Write table
  for (int i = 0; i < targets.size(); ++i) {
    double gain = 0;
    if (i != base_index)
      gain = ttest_lower_bound(base, sets[i], config.confidence);

    double min_speedup = base.real_mean() / (base.real_mean() - gain);
    min_speedup = (min_speedup - 1.) * 100.;

    double speedup = base.real_mean() / sets[i].real_mean();
    speedup = (speedup - 1.) * 100.;

    table.push(targets[i].name());  // Name
    table.push(min_speedup <= 0 ? ""
                                : format(min_speedup) + '%');  // Min Speedup
    table.push(gain <= 0 ? "" : format(gain, scale) + 's');    // Min Gain
    table.push(speedup <= 0 ? "" : format(speedup) + '%');     // Speedup
    table.push(format(sets[i].real_mean(), scale) + 's');      // real_mean()

    const char* plus_minus = config.use_ascii ? "+/- " : "±";
    table.push(plus_minus + format(sets[i].sd, scale) + 's');  // real_mean()
  }

  table.print();

  return 0;
}
