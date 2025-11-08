/**
@page quickstart_tutorial Quickstart

The Quickstart tutorial covers single-size runs, multi-size series, crossover analysis, and parsing JSON output.

For the full Markdown version in the repository see `docs/tutorials/quickstart.md`.

@code{.bash}
./build/hashbrowns --size 20000 --runs 15 --structures array,slist,hashmap \
  --out-format json --output results/csvs/benchmark_results.json
@endcode

Series sweep example:
@code{.bash}
./build/hashbrowns --size 60000 --series-count 6 --runs 3 \
  --structures array,hashmap --out-format json --series-out results/csvs/series_results.json
@endcode

Crossover analysis:
@code{.bash}
./build/hashbrowns --crossover-analysis --max-size 100000 --runs 4 \
  --structures array,slist,hashmap --out-format json --output results/csvs/crossover_results.json
@endcode

Parse JSON with the helper script:
@code{.bash}
python3 scripts/example_parse.py results/csvs/benchmark_results.json --summary
@endcode
*/
