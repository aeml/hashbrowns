#ifndef HASHBROWNS_DYNAMIC_ARRAY_H
#define HASHBROWNS_DYNAMIC_ARRAY_H

#include "core/data_structure.h"
#include "core/memory_manager.h"
#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace hashbrowns {

/**
 * @brief Growth strategy enumeration for dynamic array resizing
 */
enum class GrowthStrategy {
    MULTIPLICATIVE_2_0,  // Growth factor of 2.0
    MULTIPLICATIVE_1_5,  // Growth factor of 1.5
    FIBONACCI,           // Fibonacci sequence growth
    ADDITIVE             // Fixed increment growth
};

// Stream operator for GrowthStrategy enum
inline std::ostream& operator<<(std::ostream& os, GrowthStrategy strategy) {
    switch (strategy) {
        case GrowthStrategy::MULTIPLICATIVE_2_0:
            return os << "MULTIPLICATIVE_2_0";
        case GrowthStrategy::MULTIPLICATIVE_1_5:
            return os << "MULTIPLICATIVE_1_5";
        case GrowthStrategy::FIBONACCI:
            return os << "FIBONACCI";
        case GrowthStrategy::ADDITIVE:
            return os << "ADDITIVE";
        default:
            return os << "UNKNOWN";
    }
}

/**
 * @brief High-performance dynamic array with configurable growth strategies
 * 
 * This implementation showcases advanced C++ techniques:
 * - Template metaprogramming with SFINAE
 * - Custom allocator support
 * - STL-compatible iterators
 * - Exception safety guarantees
 * - Move semantics and perfect forwarding
 * - Multiple growth strategies for performance analysis
 * 
 * @tparam T Element type
 * @tparam Allocator Allocator type (defaults to TrackedAllocator)
 */
template<typename T, typename Allocator = TrackedAllocator<T>>
class DynamicArray : public DataStructure {
public:
    // Type aliases for STL compatibility
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    // Forward declare iterator classes
    template<typename ValueType>
    class iterator_impl;
    
    using iterator = iterator_impl<T>;
    using const_iterator = iterator_impl<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    pointer data_;
    size_type size_;
    size_type capacity_;
    GrowthStrategy growth_strategy_;
    Allocator allocator_;

    // Fibonacci growth state
    mutable size_type fib_prev_ = 1;
    mutable size_type fib_curr_ = 1;

public:
    /**
     * @brief Construct empty array with specified growth strategy
     * @param strategy Growth strategy to use for resizing
     * @param alloc Allocator instance
     */
    explicit DynamicArray(GrowthStrategy strategy = GrowthStrategy::MULTIPLICATIVE_2_0,
                         const Allocator& alloc = Allocator())
        : data_(nullptr), size_(0), capacity_(0), growth_strategy_(strategy), allocator_(alloc) {}

    /**
     * @brief Construct array with initial capacity
     * @param initial_capacity Initial capacity to reserve
     * @param strategy Growth strategy to use
     * @param alloc Allocator instance
     */
    explicit DynamicArray(size_type initial_capacity, 
                         GrowthStrategy strategy = GrowthStrategy::MULTIPLICATIVE_2_0,
                         const Allocator& alloc = Allocator())
        : data_(nullptr), size_(0), capacity_(0), growth_strategy_(strategy), allocator_(alloc) {
        reserve(initial_capacity);
    }

    /**
     * @brief Construct from initializer list
     * @param init Initializer list of elements
     * @param strategy Growth strategy to use
     * @param alloc Allocator instance
     */
    DynamicArray(std::initializer_list<T> init,
                GrowthStrategy strategy = GrowthStrategy::MULTIPLICATIVE_2_0,
                const Allocator& alloc = Allocator())
        : data_(nullptr), size_(0), capacity_(0), growth_strategy_(strategy), allocator_(alloc) {
        reserve(init.size());
        for (const auto& item : init) {
            push_back(item);
        }
    }

    /**
     * @brief Copy constructor
     */
    DynamicArray(const DynamicArray& other)
        : data_(nullptr), size_(0), capacity_(0), growth_strategy_(other.growth_strategy_), 
          allocator_(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.allocator_)) {
        reserve(other.size_);
        for (size_type i = 0; i < other.size_; ++i) {
            push_back(other.data_[i]);
        }
    }

    /**
     * @brief Move constructor
     */
    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_),
          growth_strategy_(other.growth_strategy_), allocator_(std::move(other.allocator_)),
          fib_prev_(other.fib_prev_), fib_curr_(other.fib_curr_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    /**
     * @brief Destructor
     */
    ~DynamicArray() {
        clear();
        deallocate();
    }

    /**
     * @brief Copy assignment operator
     */
    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray temp(other);
            swap(temp);
        }
        return *this;
    }

    /**
     * @brief Move assignment operator
     */
    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            clear();
            deallocate();
            
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            growth_strategy_ = other.growth_strategy_;
            allocator_ = std::move(other.allocator_);
            fib_prev_ = other.fib_prev_;
            fib_curr_ = other.fib_curr_;
            
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    // DataStructure interface implementation
    void insert(int key, const std::string& value) override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            push_back(std::make_pair(key, value));
        } else {
            throw std::invalid_argument("DynamicArray<T> does not support key-value insertion unless T is std::pair<int, std::string>");
        }
    }

    bool search(int key, std::string& value) const override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            auto it = std::find_if(begin(), end(), [key](const auto& pair) {
                return pair.first == key;
            });
            if (it != end()) {
                value = it->second;
                return true;
            }
        }
        return false;
    }

    bool remove(int key) override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            auto it = std::find_if(begin(), end(), [key](const auto& pair) {
                return pair.first == key;
            });
            if (it != end()) {
                erase(it);
                return true;
            }
        }
        return false;
    }

    size_t size() const override { return size_; }
    bool empty() const override { return size_ == 0; }
    
    void clear() override {
        for (size_type i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::destroy(allocator_, &data_[i]);
        }
        size_ = 0;
    }

    size_t memory_usage() const override {
        return capacity_ * sizeof(T) + sizeof(*this);
    }

    std::string type_name() const override { return "DynamicArray"; }
    std::string insert_complexity() const override { return "O(1) amortized"; }
    std::string search_complexity() const override { return "O(n)"; }
    std::string remove_complexity() const override { return "O(n)"; }

    // Array-specific interface
    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    template<typename... Args>
    reference emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            grow();
        }
        std::allocator_traits<Allocator>::construct(allocator_, &data_[size_], std::forward<Args>(args)...);
        return data_[size_++];
    }

    void pop_back() {
        if (empty()) {
            throw std::out_of_range("pop_back() called on empty array");
        }
        --size_;
        std::allocator_traits<Allocator>::destroy(allocator_, &data_[size_]);
    }

    reference at(size_type index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    const_reference at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    reference operator[](size_type index) { return data_[index]; }
    const_reference operator[](size_type index) const { return data_[index]; }

    reference front() { return data_[0]; }
    const_reference front() const { return data_[0]; }
    reference back() { return data_[size_ - 1]; }
    const_reference back() const { return data_[size_ - 1]; }

    T* data() noexcept { return data_; }
    const T* data() const noexcept { return data_; }

    size_type capacity() const noexcept { return capacity_; }
    
    void reserve(size_type new_capacity) {
        if (new_capacity > capacity_) {
            reallocate(new_capacity);
        }
    }

    void shrink_to_fit() {
        if (size_ < capacity_) {
            reallocate(size_);
        }
    }

    void resize(size_type new_size) {
        if (new_size > capacity_) {
            reserve(new_size);
        }
        
        if (new_size > size_) {
            for (size_type i = size_; i < new_size; ++i) {
                std::allocator_traits<Allocator>::construct(allocator_, &data_[i]);
            }
        } else if (new_size < size_) {
            for (size_type i = new_size; i < size_; ++i) {
                std::allocator_traits<Allocator>::destroy(allocator_, &data_[i]);
            }
        }
        size_ = new_size;
    }

    void resize(size_type new_size, const T& value) {
        if (new_size > capacity_) {
            reserve(new_size);
        }
        
        if (new_size > size_) {
            for (size_type i = size_; i < new_size; ++i) {
                std::allocator_traits<Allocator>::construct(allocator_, &data_[i], value);
            }
        } else if (new_size < size_) {
            for (size_type i = new_size; i < size_; ++i) {
                std::allocator_traits<Allocator>::destroy(allocator_, &data_[i]);
            }
        }
        size_ = new_size;
    }

    // Iterator interface
    iterator begin() { return iterator(data_); }
    const_iterator begin() const { return const_iterator(data_); }
    const_iterator cbegin() const { return const_iterator(data_); }
    
    iterator end() { return iterator(data_ + size_); }
    const_iterator end() const { return const_iterator(data_ + size_); }
    const_iterator cend() const { return const_iterator(data_ + size_); }
    
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        auto diff = last - first;
        auto index = first - cbegin();
        
        // Move elements
        for (size_type i = index; i + diff < size_; ++i) {
            data_[i] = std::move(data_[i + diff]);
        }
        
        // Destroy moved-from elements
        for (size_type i = size_ - diff; i < size_; ++i) {
            std::allocator_traits<Allocator>::destroy(allocator_, &data_[i]);
        }
        
        size_ -= diff;
        return iterator(data_ + index);
    }

    // Utility methods
    void swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(growth_strategy_, other.growth_strategy_);
        std::swap(allocator_, other.allocator_);
        std::swap(fib_prev_, other.fib_prev_);
        std::swap(fib_curr_, other.fib_curr_);
    }

    GrowthStrategy growth_strategy() const { return growth_strategy_; }
    void set_growth_strategy(GrowthStrategy strategy) { growth_strategy_ = strategy; }

private:
    void grow() {
        size_type new_capacity = calculate_growth();
        reallocate(new_capacity);
    }

    size_type calculate_growth() const {
        if (capacity_ == 0) return 1;
        
        switch (growth_strategy_) {
            case GrowthStrategy::MULTIPLICATIVE_1_5: {
                size_type new_cap = capacity_ + (capacity_ + 1) / 2; // Ceiling division
                return std::max(new_cap, capacity_ + 1);
            }
            case GrowthStrategy::MULTIPLICATIVE_2_0:
                return capacity_ * 2;
            case GrowthStrategy::FIBONACCI: {
                size_type next_fib = fib_prev_ + fib_curr_;
                fib_prev_ = fib_curr_;
                fib_curr_ = next_fib;
                return std::max(capacity_ + 1, next_fib);
            }
            case GrowthStrategy::ADDITIVE:
                return capacity_ + 10; // Fixed increment for testing
            default:
                return capacity_ * 2;
        }
    }

    void reallocate(size_type new_capacity) {
        pointer new_data = std::allocator_traits<Allocator>::allocate(allocator_, new_capacity);
        
        // Move construct existing elements
        for (size_type i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::construct(allocator_, &new_data[i], std::move(data_[i]));
            std::allocator_traits<Allocator>::destroy(allocator_, &data_[i]);
        }
        
        deallocate();
        data_ = new_data;
        capacity_ = new_capacity;
    }

    void deallocate() {
        if (data_) {
            std::allocator_traits<Allocator>::deallocate(allocator_, data_, capacity_);
            data_ = nullptr;
        }
    }
};

/**
 * @brief STL-compatible iterator implementation
 */
template<typename T, typename Allocator>
template<typename ValueType>
class DynamicArray<T, Allocator>::iterator_impl {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::remove_cv_t<ValueType>;
    using difference_type = std::ptrdiff_t;
    using pointer = ValueType*;
    using reference = ValueType&;

private:
    pointer ptr_;

public:
    iterator_impl() : ptr_(nullptr) {}
    explicit iterator_impl(pointer ptr) : ptr_(ptr) {}
    
    // Copy constructor from non-const to const
    template<typename U, typename = std::enable_if_t<std::is_same_v<U, value_type> && std::is_const_v<ValueType>>>
    iterator_impl(const iterator_impl<U>& other) : ptr_(other.get_ptr()) {}

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }
    
    iterator_impl& operator++() { ++ptr_; return *this; }
    iterator_impl operator++(int) { iterator_impl temp(*this); ++ptr_; return temp; }
    
    iterator_impl& operator--() { --ptr_; return *this; }
    iterator_impl operator--(int) { iterator_impl temp(*this); --ptr_; return temp; }
    
    iterator_impl& operator+=(difference_type n) { ptr_ += n; return *this; }
    iterator_impl& operator-=(difference_type n) { ptr_ -= n; return *this; }
    
    iterator_impl operator+(difference_type n) const { return iterator_impl(ptr_ + n); }
    iterator_impl operator-(difference_type n) const { return iterator_impl(ptr_ - n); }
    
    difference_type operator-(const iterator_impl& other) const { return ptr_ - other.ptr_; }
    
    reference operator[](difference_type n) const { return ptr_[n]; }
    
    bool operator==(const iterator_impl& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const iterator_impl& other) const { return ptr_ != other.ptr_; }
    bool operator<(const iterator_impl& other) const { return ptr_ < other.ptr_; }
    bool operator<=(const iterator_impl& other) const { return ptr_ <= other.ptr_; }
    bool operator>(const iterator_impl& other) const { return ptr_ > other.ptr_; }
    bool operator>=(const iterator_impl& other) const { return ptr_ >= other.ptr_; }

    friend iterator_impl operator+(difference_type n, const iterator_impl& it) {
        return iterator_impl(it.ptr_ + n);
    }

    // Allow access to ptr_ for conversions
    pointer get_ptr() const { return ptr_; }
};

// Non-member functions
template<typename T, typename Allocator>
void swap(DynamicArray<T, Allocator>& lhs, DynamicArray<T, Allocator>& rhs) noexcept {
    lhs.swap(rhs);
}

template<typename T, typename Allocator>
bool operator==(const DynamicArray<T, Allocator>& lhs, const DynamicArray<T, Allocator>& rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename T, typename Allocator>
bool operator!=(const DynamicArray<T, Allocator>& lhs, const DynamicArray<T, Allocator>& rhs) {
    return !(lhs == rhs);
}

} // namespace hashbrowns

#endif // HASHBROWNS_DYNAMIC_ARRAY_H