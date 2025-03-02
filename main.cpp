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

  if (config.show_help || config.targets.empty()) {
    cout << HELP;
    return 0;
  }

  vector<Target> targets;
  for (auto& target : config.targets)
    targets.emplace_back(target);

  // Execute
  take_samples(targets, config.min_warmup_seconds, config.min_warmup_samples,
               false);
  take_samples(targets, config.min_seconds, config.min_samples);

  vector<DataSet> sets;
  for (Target& target : targets)
    sets.emplace_back(target.time_samples(), config.outliers);

  // Analyze
  int base_index = 0;
  for (int i = 0; i < sets.size(); ++i) {
    if (sets[i].mean > sets[base_index].mean)
      base_index = i;
  }

  DataSet& base = sets[base_index];

  Table table(config.column_names);

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
        ok = ok && set.mean * s.scale >= 1;
      }

      if (ok)
        scale = s;
      else
        break;
    }
  }

  // Choose speedup format
  bool speedup_precentage = false;
  for (int i = 0; i < sets.size(); ++i) {
    if (i != base_index) {
      double speedup = base.mean / sets[i].mean;
      if (speedup < 2.)
        speedup_precentage = true;
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
    double min_gain = 0;
    if (i != base_index)
      min_gain = ttest_lower_bound(base, sets[i], config.confidence);

    double min_speedup = base.mean / (base.mean - min_gain);
    if (speedup_precentage)
      min_speedup = (min_speedup - 1.) * 100.;

    double speedup = base.mean / sets[i].mean;
    if (speedup_precentage)
      speedup = (speedup - 1.) * 100.;

    double gain = base.mean - sets[i].mean;

    double outliners = 100. * double(sets[i].outliers) / double(sets[i].n);

    if (min_speedup > 0 && speedup_precentage)
      table.push(config.column.min_speedup, format(min_speedup) + '%');
    if (min_speedup > 1 && !speedup_precentage)
      table.push(config.column.min_speedup, 'x' + format(min_speedup));

    if (speedup > 0 && speedup_precentage)
      table.push(config.column.speedup, format(speedup) + '%');
    if (speedup > 1 && !speedup_precentage)
      table.push(config.column.speedup, 'x' + format(speedup));

    if (min_gain > 0)
      table.push(config.column.min_gain, format(min_gain, scale) + 's');

    if (gain > 0)
      table.push(config.column.gain, format(gain, scale) + 's');
    if (outliners > 0)
      table.push(config.column.outliers, format(outliners) + '%');

    const char* plus_minus = config.use_ascii ? "+/- " : "±";
    table.push(config.column.std, plus_minus + format(sets[i].sd, scale) + 's');
    table.push(config.column.name, targets[i].name());
    table.push(config.column.mean, format(sets[i].mean, scale) + 's');
    table.push(config.column.samples, to_string(sets[i].n));

    table.fill_row(i);
  }

  table.print();

  return 0;
}
