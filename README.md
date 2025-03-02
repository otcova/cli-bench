```sh
Usage: bench [<options>] [-- <command>] [-- <command>] ...
Stadistical Options:
    --conf <%>       Statistical confidence of the lowerbound.
    --keep-outliers  Do not remove outlier.

Sampling Options:
    -t <secs>    Minimum seconds inverted in taking samples.
    -n <num>     Minimum amount of samples.
    --wt <secs>  Minimum seconds inverted in the warmup.
    --wn <num>   Minimum amount of samples in the warmup.

Display Options:
    -a              Only use ASCII characters.
    --csv [<file>]  Output a table of samples with csv format.
    --no-prefix     Do not use metric prefixes (0.012s instead of 12ms)
    --cols <list>   Comma separeted list of columns to show. Options:
                      name       - Command name
                      speedup    - Mean speedup
                      minSpeedup - Statistical speedup lowerbound
                      gain       - Mean gain
                      minGain    - Statistical gain lowerbound
                      mean       - Mean time
                      std        - Standard deviation
                      samples    - Number of usefull samples
                      outliers   - Precentage of removed outliners

Examples:
    > bench -- bash -ic '' -- bash -c '' -- sh -c ''
    > bench --cols mean,std -- sleep 1
```
