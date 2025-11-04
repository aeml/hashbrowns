#ifndef HASHBROWNS_HASH_MAP_H
#define HASHBROWNS_HASH_MAP_H

#include "core/data_structure.h"
#include "core/memory_manager.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <functional>
#include <cmath>
#include <new>
#include <stdexcept>

namespace hashbrowns {

enum class HashStrategy { OPEN_ADDRESSING, SEPARATE_CHAINING };

class HashMap : public DataStructure {
public:
    explicit HashMap(HashStrategy strategy = HashStrategy::OPEN_ADDRESSING,
                     std::size_t initial_capacity = 16)
        : strategy_(strategy) {
        if (initial_capacity == 0) initial_capacity = 16;
        if (strategy_ == HashStrategy::OPEN_ADDRESSING) {
            oa_init(capacity_round_up(initial_capacity));
        } else {
            sc_init(capacity_round_up(initial_capacity));
        }
    }

    ~HashMap() override { clear(); free_storage(); }

    // DataStructure interface
    void insert(int key, const std::string& value) override {
        if (strategy_ == HashStrategy::OPEN_ADDRESSING) oa_insert(key, value); else sc_insert(key, value);
    }

    bool search(int key, std::string& value) const override {
        return (strategy_ == HashStrategy::OPEN_ADDRESSING) ? oa_search(key, value) : sc_search(key, value);
    }

    bool remove(int key) override {
        return (strategy_ == HashStrategy::OPEN_ADDRESSING) ? oa_remove(key) : sc_remove(key);
    }

    size_t size() const override { return size_; }
    bool empty() const override { return size_ == 0; }

    void clear() override {
        if (strategy_ == HashStrategy::OPEN_ADDRESSING) oa_clear(); else sc_clear();
        size_ = 0;
    }

    size_t memory_usage() const override {
        if (strategy_ == HashStrategy::OPEN_ADDRESSING) {
            return capacity_ * sizeof(OAEntry) + sizeof(*this);
        } else {
            return capacity_ * sizeof(SCNode*) + sc_node_count_ * sizeof(SCNode) + sizeof(*this);
        }
    }

    std::string type_name() const override { return "HashMap"; }
    std::string insert_complexity() const override { return strategy_ == HashStrategy::OPEN_ADDRESSING ? "O(1) avg" : "O(1) avg"; }
    std::string search_complexity() const override { return strategy_ == HashStrategy::OPEN_ADDRESSING ? "O(1) avg" : "O(1) avg"; }
    std::string remove_complexity() const override { return strategy_ == HashStrategy::OPEN_ADDRESSING ? "O(1) avg" : "O(1) avg"; }

    HashStrategy strategy() const { return strategy_; }
    void set_strategy(HashStrategy s) {
        if (s == strategy_) return;
        // Rebuild into new strategy
        // Extract entries
        // Simple approach: iterate and re-insert into new structure
        // For brevity, only support rebuilding when map is empty
        if (size_ != 0) {
            throw std::runtime_error("Changing HashMap strategy requires empty map");
        }
        free_storage();
        strategy_ = s;
        if (strategy_ == HashStrategy::OPEN_ADDRESSING) oa_init(16); else sc_init(16);
    }

private:
    // Hash helpers
    static std::size_t capacity_round_up(std::size_t n) {
        // round to power of two
        std::size_t cap = 1;
        while (cap < n) cap <<= 1;
        return cap;
    }

    static std::size_t mix(std::uint64_t x) {
        // simple 64-bit mix (splitmix64)
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);
        return static_cast<std::size_t>(x);
    }

    // OPEN ADDRESSING
    enum class OAState : uint8_t { EMPTY, OCCUPIED, TOMBSTONE };
    struct OAEntry {
        int key;
        std::string value;
        OAState state;
        OAEntry() : key(0), value(), state(OAState::EMPTY) {}
    };

    void oa_init(std::size_t cap) {
        capacity_ = cap;
        load_threshold_ = static_cast<std::size_t>(capacity_ * 0.7);
        oa_entries_ = std::allocator_traits<TrackedAllocator<OAEntry>>::allocate(oa_alloc_, capacity_);
        for (std::size_t i = 0; i < capacity_; ++i) {
            std::allocator_traits<TrackedAllocator<OAEntry>>::construct(oa_alloc_, &oa_entries_[i]);
        }
    }

    void oa_clear() {
        if (!oa_entries_) return;
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (oa_entries_[i].state == OAState::OCCUPIED) {
                oa_entries_[i].value.~basic_string();
                // re-construct default to EMPTY without tracking again
                new (&oa_entries_[i]) OAEntry();
            } else if (oa_entries_[i].state == OAState::TOMBSTONE) {
                oa_entries_[i].state = OAState::EMPTY;
            }
        }
    }

    void oa_free() {
        if (!oa_entries_) return;
        for (std::size_t i = 0; i < capacity_; ++i) {
            std::allocator_traits<TrackedAllocator<OAEntry>>::destroy(oa_alloc_, &oa_entries_[i]);
        }
        std::allocator_traits<TrackedAllocator<OAEntry>>::deallocate(oa_alloc_, oa_entries_, capacity_);
        oa_entries_ = nullptr;
    }

    void oa_grow() {
        std::size_t new_cap = capacity_ ? capacity_ * 2 : 16;
        OAEntry* old = oa_entries_;
        std::size_t old_cap = capacity_;
        oa_init(new_cap);
        std::size_t old_size = size_;
        size_ = 0;
        for (std::size_t i = 0; i < old_cap; ++i) {
            if (old[i].state == OAState::OCCUPIED) {
                oa_insert(old[i].key, old[i].value);
            }
        }
        // destroy old entries and deallocate
        for (std::size_t i = 0; i < old_cap; ++i) {
            std::allocator_traits<TrackedAllocator<OAEntry>>::destroy(oa_alloc_, &old[i]);
        }
        std::allocator_traits<TrackedAllocator<OAEntry>>::deallocate(oa_alloc_, old, old_cap);
        size_ = old_size; // oa_insert already updated size_
    }

    void oa_insert(int key, const std::string& value) {
        if (size_ + tombstones_ >= load_threshold_) {
            oa_grow();
            tombstones_ = 0;
        }
        std::size_t mask = capacity_ - 1;
        std::size_t idx = mix(static_cast<std::uint64_t>(key)) & mask;
        std::size_t first_tomb = capacity_;
        while (true) {
            OAEntry& e = oa_entries_[idx];
            if (e.state == OAState::EMPTY) {
                std::size_t target = (first_tomb < capacity_) ? first_tomb : idx;
                OAEntry& slot = oa_entries_[target];
                if (slot.state == OAState::TOMBSTONE) { tombstones_--; }
                slot.key = key;
                new (&slot.value) std::string(value);
                slot.state = OAState::OCCUPIED;
                ++size_;
                return;
            } else if (e.state == OAState::TOMBSTONE) {
                if (first_tomb == capacity_) first_tomb = idx;
            } else if (e.key == key) {
                e.value = value; // update
                return;
            }
            idx = (idx + 1) & mask; // linear probing
        }
    }

    bool oa_search(int key, std::string& value) const {
        std::size_t mask = capacity_ - 1;
        std::size_t idx = mix(static_cast<std::uint64_t>(key)) & mask;
        while (true) {
            const OAEntry& e = oa_entries_[idx];
            if (e.state == OAState::EMPTY) return false;
            if (e.state == OAState::OCCUPIED && e.key == key) { value = e.value; return true; }
            idx = (idx + 1) & mask;
        }
    }

    bool oa_remove(int key) {
        std::size_t mask = capacity_ - 1;
        std::size_t idx = mix(static_cast<std::uint64_t>(key)) & mask;
        while (true) {
            OAEntry& e = oa_entries_[idx];
            if (e.state == OAState::EMPTY) return false;
            if (e.state == OAState::OCCUPIED && e.key == key) {
                e.value.~basic_string();
                e.state = OAState::TOMBSTONE;
                --size_;
                ++tombstones_;
                return true;
            }
            idx = (idx + 1) & mask;
        }
    }

    // SEPARATE CHAINING
    struct SCNode { int key; std::string value; SCNode* next; };

    void sc_init(std::size_t cap) {
        capacity_ = cap;
        buckets_ = std::allocator_traits<TrackedAllocator<SCNode*>>::allocate(sc_bucket_alloc_, capacity_);
        for (std::size_t i = 0; i < capacity_; ++i) {
            buckets_[i] = nullptr;
        }
    }

    void sc_clear() {
        if (!buckets_) return;
        for (std::size_t i = 0; i < capacity_; ++i) {
            SCNode* n = buckets_[i];
            while (n) {
                SCNode* next = n->next;
                destroy_node(n);
                n = next;
            }
            buckets_[i] = nullptr;
        }
        sc_node_count_ = 0;
    }

    void sc_free() {
        if (!buckets_) return;
        std::allocator_traits<TrackedAllocator<SCNode*>>::deallocate(sc_bucket_alloc_, buckets_, capacity_);
        buckets_ = nullptr;
    }

    void sc_grow_if_needed() {
        if (size_ <= capacity_ * 0.75) return;
        std::size_t new_cap = capacity_ ? capacity_ * 2 : 16;
        // rehash
        auto old_buckets = buckets_;
        std::size_t old_cap = capacity_;
        sc_init(new_cap);
        std::size_t old_size = size_;
        size_ = 0;
        for (std::size_t i = 0; i < old_cap; ++i) {
            SCNode* n = old_buckets[i];
            while (n) {
                sc_insert(n->key, n->value);
                SCNode* next = n->next;
                destroy_node(n);
                n = next;
            }
        }
        std::allocator_traits<TrackedAllocator<SCNode*>>::deallocate(sc_bucket_alloc_, old_buckets, old_cap);
        size_ = old_size; // already updated by sc_insert
    }

    void sc_insert(int key, const std::string& value) {
        sc_grow_if_needed();
        std::size_t idx = mix(static_cast<std::uint64_t>(key)) & (capacity_ - 1);
        // check existing
        for (SCNode* n = buckets_[idx]; n; n = n->next) {
            if (n->key == key) { n->value = value; return; }
        }
        SCNode* node = create_node(key, value);
        node->next = buckets_[idx];
        buckets_[idx] = node;
        ++size_;
        ++sc_node_count_;
    }

    bool sc_search(int key, std::string& value) const {
        std::size_t idx = mix(static_cast<std::uint64_t>(key)) & (capacity_ - 1);
        for (SCNode* n = buckets_[idx]; n; n = n->next) {
            if (n->key == key) { value = n->value; return true; }
        }
        return false;
    }

    bool sc_remove(int key) {
        std::size_t idx = mix(static_cast<std::uint64_t>(key)) & (capacity_ - 1);
        SCNode* prev = nullptr;
        SCNode* cur = buckets_[idx];
        while (cur) {
            if (cur->key == key) {
                if (prev) prev->next = cur->next; else buckets_[idx] = cur->next;
                destroy_node(cur);
                --size_;
                --sc_node_count_;
                return true;
            }
            prev = cur; cur = cur->next;
        }
        return false;
    }

    // Node allocation via MemoryPool
    SCNode* create_node(int key, const std::string& value) {
        SCNode* raw = sc_pool_.allocate();
        new (raw) SCNode{key, value, nullptr};
        return raw;
    }

    void destroy_node(SCNode* node) {
        if (!node) return;
        node->~SCNode();
        sc_pool_.deallocate(node);
    }

    void free_storage() {
        if (strategy_ == HashStrategy::OPEN_ADDRESSING) oa_free(); else sc_free();
    }

private:
    // State shared by both
    HashStrategy strategy_;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;

    // Open addressing state
    TrackedAllocator<OAEntry> oa_alloc_{};
    OAEntry* oa_entries_ = nullptr;
    std::size_t load_threshold_ = 0;
    std::size_t tombstones_ = 0;

    // Separate chaining state
    TrackedAllocator<SCNode*> sc_bucket_alloc_{};
    SCNode** buckets_ = nullptr;
    MemoryPool<SCNode> sc_pool_{};
    std::size_t sc_node_count_ = 0;
};

} // namespace hashbrowns

#endif // HASHBROWNS_HASH_MAP_H