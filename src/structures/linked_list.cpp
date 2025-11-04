#include "linked_list.h"

namespace hashbrowns {

// Explicit template instantiations for common types
template class SinglyLinkedList<std::pair<int, std::string>>;
template class DoublyLinkedList<std::pair<int, std::string>>;

} // namespace hashbrowns