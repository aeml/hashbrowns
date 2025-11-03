#include "dynamic_array.h"

namespace hashbrowns {

// Template instantiations for common types to reduce compile time
template class DynamicArray<int>;
template class DynamicArray<double>;
template class DynamicArray<std::string>;
template class DynamicArray<std::pair<int, std::string>>;

} // namespace hashbrowns