#ifndef HASHBROWNS_LINKED_LIST_H
#define HASHBROWNS_LINKED_LIST_H

#include "core/data_structure.h"
#include "core/memory_manager.h"
#include <utility>
#include <string>
#include <stdexcept>
#include <cstddef>
#include <type_traits>

namespace hashbrowns {

// Singly-linked list with memory pool allocation
template<typename T = std::pair<int, std::string>>
class SinglyLinkedList : public DataStructure {
private:
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& v) : value(v), next(nullptr) {}
        explicit Node(T&& v) : value(std::move(v)), next(nullptr) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;
    MemoryPool<Node> pool_;

public:
    SinglyLinkedList() = default;
    ~SinglyLinkedList() override { clear(); }

    SinglyLinkedList(const SinglyLinkedList& other) {
        for (Node* n = other.head_; n; n = n->next) {
            push_back(n->value);
        }
    }

    SinglyLinkedList& operator=(const SinglyLinkedList& other) {
        if (this != &other) {
            clear();
            for (Node* n = other.head_; n; n = n->next) {
                push_back(n->value);
            }
        }
        return *this;
    }

    SinglyLinkedList(SinglyLinkedList&& other) noexcept {
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
        other.head_ = other.tail_ = nullptr;
        other.size_ = 0;
    }

    SinglyLinkedList& operator=(SinglyLinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // DataStructure interface
    void insert(int key, const std::string& value) override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            push_back(std::make_pair(key, value));
        } else {
            throw std::invalid_argument("SinglyLinkedList<T> insert only supported for T=std::pair<int,std::string>");
        }
    }

    bool search(int key, std::string& value) const override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            for (Node* n = head_; n; n = n->next) {
                if (n->value.first == key) { value = n->value.second; return true; }
            }
        }
        return false;
    }

    bool remove(int key) override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            Node* prev = nullptr;
            Node* cur = head_;
            while (cur) {
                if (cur->value.first == key) {
                    if (prev) prev->next = cur->next; else head_ = cur->next;
                    if (cur == tail_) tail_ = prev;
                    destroy_and_release(cur);
                    --size_;
                    return true;
                }
                prev = cur; cur = cur->next;
            }
        }
        return false;
    }

    size_t size() const override { return size_; }
    bool empty() const override { return size_ == 0; }
    void clear() override {
        Node* n = head_;
        while (n) {
            Node* next = n->next;
            destroy_and_release(n);
            n = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    size_t memory_usage() const override {
        return size_ * sizeof(Node) + sizeof(*this);
    }

    std::string type_name() const override { return "SinglyLinkedList"; }
    std::string insert_complexity() const override { return "O(1) amortized at tail"; }
    std::string search_complexity() const override { return "O(n)"; }
    std::string remove_complexity() const override { return "O(n)"; }

    // List-specific API
    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v) { emplace_back(std::move(v)); }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        Node* raw = pool_.allocate();
        // placement-new construct
        new (raw) Node(T(std::forward<Args>(args)...));
        raw->next = nullptr;
        if (!tail_) { head_ = tail_ = raw; }
        else { tail_->next = raw; tail_ = raw; }
        ++size_;
    }

private:
    void destroy_and_release(Node* node) {
        if (!node) return;
        node->~Node();
        pool_.deallocate(node);
    }
};

// Doubly-linked list with memory pool allocation
template<typename T = std::pair<int, std::string>>
class DoublyLinkedList : public DataStructure {
private:
    struct Node {
        T value;
        Node* prev;
        Node* next;
        explicit Node(const T& v) : value(v), prev(nullptr), next(nullptr) {}
        explicit Node(T&& v) : value(std::move(v)), prev(nullptr), next(nullptr) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;
    MemoryPool<Node> pool_;

public:
    DoublyLinkedList() = default;
    ~DoublyLinkedList() override { clear(); }

    DoublyLinkedList(const DoublyLinkedList& other) {
        for (Node* n = other.head_; n; n = n->next) {
            push_back(n->value);
        }
    }

    DoublyLinkedList& operator=(const DoublyLinkedList& other) {
        if (this != &other) {
            clear();
            for (Node* n = other.head_; n; n = n->next) {
                push_back(n->value);
            }
        }
        return *this;
    }

    DoublyLinkedList(DoublyLinkedList&& other) noexcept {
        head_ = other.head_;
        tail_ = other.tail_;
        size_ = other.size_;
        other.head_ = other.tail_ = nullptr;
        other.size_ = 0;
    }

    DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // DataStructure interface
    void insert(int key, const std::string& value) override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            push_back(std::make_pair(key, value));
        } else {
            throw std::invalid_argument("DoublyLinkedList<T> insert only supported for T=std::pair<int,std::string>");
        }
    }

    bool search(int key, std::string& value) const override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            for (Node* n = head_; n; n = n->next) {
                if (n->value.first == key) { value = n->value.second; return true; }
            }
        }
        return false;
    }

    bool remove(int key) override {
        if constexpr (std::is_same_v<T, std::pair<int, std::string>>) {
            for (Node* n = head_; n; n = n->next) {
                if (n->value.first == key) {
                    if (n->prev) n->prev->next = n->next; else head_ = n->next;
                    if (n->next) n->next->prev = n->prev; else tail_ = n->prev;
                    destroy_and_release(n);
                    --size_;
                    return true;
                }
            }
        }
        return false;
    }

    size_t size() const override { return size_; }
    bool empty() const override { return size_ == 0; }
    void clear() override {
        Node* n = head_;
        while (n) {
            Node* next = n->next;
            destroy_and_release(n);
            n = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    size_t memory_usage() const override {
        return size_ * sizeof(Node) + sizeof(*this);
    }

    std::string type_name() const override { return "DoublyLinkedList"; }
    std::string insert_complexity() const override { return "O(1) amortized at tail"; }
    std::string search_complexity() const override { return "O(n)"; }
    std::string remove_complexity() const override { return "O(1) when node known; O(n) to find"; }

    // List-specific API
    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v) { emplace_back(std::move(v)); }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        Node* raw = pool_.allocate();
        new (raw) Node(T(std::forward<Args>(args)...));
        raw->prev = tail_;
        raw->next = nullptr;
        if (!tail_) { head_ = tail_ = raw; }
        else { tail_->next = raw; tail_ = raw; }
        ++size_;
    }

private:
    void destroy_and_release(Node* node) {
        if (!node) return;
        node->~Node();
        pool_.deallocate(node);
    }
};

} // namespace hashbrowns

#endif // HASHBROWNS_LINKED_LIST_H