#ifndef HASHBROWNS_CLI_HANDLERS_H
#define HASHBROWNS_CLI_HANDLERS_H

#include <cstddef>
#include <string>
#include <vector>

namespace hashbrowns {
namespace cli {

/**
 * @brief Interactive wizard mode: prompts the user and runs the benchmark.
 * @return 0 on success, non-zero on error or empty results.
 */
int run_wizard();

/**
 * @brief Quick per-operation timing test for the given structure names.
 * @param names  List of structure identifiers (e.g. "array", "slist", "hashmap").
 * @param size   Number of elements to insert/search/remove.
 * @return 0 always (prints results to stdout).
 */
int run_op_tests(const std::vector<std::string>& names, std::size_t size);

} // namespace cli
} // namespace hashbrowns

#endif // HASHBROWNS_CLI_HANDLERS_H
