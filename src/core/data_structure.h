#ifndef HASHBROWNS_DATA_STRUCTURE_H
#define HASHBROWNS_DATA_STRUCTURE_H

#include <string>
#include <memory>
#include <functional>

namespace hashbrowns {

/**
 * @brief Abstract base class for all data structures in the benchmarking suite
 * 
 * This polymorphic interface enables dynamic benchmarking without type switching.
 * All data structures implement these core operations for consistent measurement.
 * 
 * Key design principles:
 * - Pure virtual interface for polymorphic behavior
 * - Consistent operation signatures across implementations
 * - Memory management handled by derived classes
 * - Exception safety through RAII principles
 */
class DataStructure {
public:
    /**
     * @brief Virtual destructor for proper cleanup in inheritance hierarchy
     */
    virtual ~DataStructure() = default;

    /**
     * @brief Insert a key-value pair into the data structure
     * @param key The integer key to insert
     * @param value The string value associated with the key
     * @throws std::bad_alloc if memory allocation fails
     * @throws std::invalid_argument if key is invalid for the structure
     */
    virtual void insert(int key, const std::string& value) = 0;

    /**
     * @brief Search for a value by key
     * @param key The key to search for
     * @param value Output parameter to store the found value
     * @return true if key found, false otherwise
     * @note value parameter is only modified if key is found
     */
    virtual bool search(int key, std::string& value) const = 0;

    /**
     * @brief Remove a key-value pair from the data structure
     * @param key The key to remove
     * @return true if key was found and removed, false otherwise
     */
    virtual bool remove(int key) = 0;

    /**
     * @brief Get the current number of elements in the structure
     * @return Number of key-value pairs currently stored
     */
    virtual size_t size() const = 0;

    /**
     * @brief Check if the data structure is empty
     * @return true if size() == 0, false otherwise
     */
    virtual bool empty() const = 0;

    /**
     * @brief Remove all elements from the data structure
     * @post size() == 0 && empty() == true
     */
    virtual void clear() = 0;

    /**
     * @brief Get the current memory usage in bytes (approximate)
     * @return Estimated memory footprint of the data structure
     * @note Used for memory usage analysis in benchmarks
     */
    virtual size_t memory_usage() const = 0;

    /**
     * @brief Get a human-readable name for this data structure type
     * @return String identifier (e.g., "DynamicArray", "LinkedList", "HashMap")
     */
    virtual std::string type_name() const = 0;

    /**
     * @brief Get theoretical time complexity for insert operation
     * @return String describing complexity (e.g., "O(1)", "O(n)")
     */
    virtual std::string insert_complexity() const = 0;

    /**
     * @brief Get theoretical time complexity for search operation
     * @return String describing complexity (e.g., "O(1)", "O(n)")
     */
    virtual std::string search_complexity() const = 0;

    /**
     * @brief Get theoretical time complexity for remove operation
     * @return String describing complexity (e.g., "O(1)", "O(n)")
     */
    virtual std::string remove_complexity() const = 0;

protected:
    /**
     * @brief Protected default constructor - only derived classes can instantiate
     */
    DataStructure() = default;

    /**
     * @brief Protected copy constructor - derived classes control copying
     */
    DataStructure(const DataStructure&) = default;

    /**
     * @brief Protected copy assignment - derived classes control assignment
     */
    DataStructure& operator=(const DataStructure&) = default;

    /**
     * @brief Protected move constructor - derived classes control moving
     */
    DataStructure(DataStructure&&) = default;

    /**
     * @brief Protected move assignment - derived classes control move assignment
     */
    DataStructure& operator=(DataStructure&&) = default;
};

/**
 * @brief Smart pointer type for managing DataStructure instances
 * 
 * This alias provides consistent memory management and polymorphic behavior
 * throughout the benchmarking framework.
 */
using DataStructurePtr = std::unique_ptr<DataStructure>;

/**
 * @brief Factory function signature for creating data structure instances
 * 
 * Factory functions allow dynamic creation of different data structure types
 * based on string identifiers or other runtime parameters.
 */
using DataStructureFactory = std::function<DataStructurePtr()>;

} // namespace hashbrowns

#endif // HASHBROWNS_DATA_STRUCTURE_H