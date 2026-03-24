/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */

template<
	class Key,
	class T,
	class Hash = std::hash<Key>,
	class Equal = std::equal_to<Key>
> class linked_hashmap {
private:
    // Node structure for both hash table chain and doubly-linked list
    struct Node {
        pair<const Key, T> data;
        Node* next_in_bucket;  // Next node in hash bucket chain
        Node* prev_in_list;    // Previous node in insertion order list
        Node* next_in_list;    // Next node in insertion order list

        Node(const Key& k, const T& v)
            : data(k, v), next_in_bucket(nullptr), prev_in_list(nullptr), next_in_list(nullptr) {}

        Node(const pair<const Key, T>& p)
            : data(p.first, p.second), next_in_bucket(nullptr), prev_in_list(nullptr), next_in_list(nullptr) {}
    };

    // Hash table parameters
    static const size_t INITIAL_CAPACITY = 256;
    static const double LOAD_FACTOR_THRESHOLD;

    // Hash table
    Node** buckets;
    size_t bucket_count;
    size_t element_count;

    // Doubly-linked list for insertion order
    Node* head;
    Node* tail;

    // Hash and equality functors
    Hash hasher;
    Equal key_equal;

    // Helper functions
    size_t hash_index(const Key& key) const {
        return hasher(key) % bucket_count;
    }

    void rehash() {
        size_t new_capacity = bucket_count * 2;
        Node** new_buckets = new Node*[new_capacity]();

        // Rehash all elements
        Node* current = head;
        while (current) {
            Node* next_in_list = current->next_in_list;
            size_t new_index = hasher(current->data.first) % new_capacity;

            // Insert at beginning of chain
            current->next_in_bucket = new_buckets[new_index];
            new_buckets[new_index] = current;

            current = next_in_list;
        }

        // Clean up old buckets
        delete[] buckets;
        buckets = new_buckets;
        bucket_count = new_capacity;
    }

    void clear_buckets() {
        for (size_t i = 0; i < bucket_count; ++i) {
            Node* current = buckets[i];
            while (current) {
                Node* next = current->next_in_bucket;
                delete current;
                current = next;
            }
            buckets[i] = nullptr;
        }
    }

public:
	/**
	 * the internal type of data.
	 * it should have a default constructor, a copy constructor.
	 * You can use sjtu::linked_hashmap as value_type by typedef.
	 */
	typedef pair<const Key, T> value_type;

	/**
	 * see BidirectionalIterator at CppReference for help.
	 *
	 * if there is anything wrong throw invalid_iterator.
	 *     like it = linked_hashmap.begin(); --it;
	 *       or it = linked_hashmap.end(); ++end();
	 */
	class const_iterator;
	class iterator {
	private:
		Node* node_ptr;
        linked_hashmap* container;

	public:
		// The following code is written for the C++ type_traits library.
		// Type traits is a C++ feature for describing certain properties of a type.
		// For instance, for an iterator, iterator::value_type is the type that the
		// iterator points to.
		// STL algorithms and containers may use these type_traits (e.g. the following
		// typedef) to work properly.
		// See these websites for more information:
		// https://en.cppreference.com/w/cpp/header/type_traits
		// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
		// About iterator_category: https://en.cppreference.com/w/cpp/iterator
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::output_iterator_tag;


		iterator() : node_ptr(nullptr), container(nullptr) {}
		iterator(Node* node, linked_hashmap* cont) : node_ptr(node), container(cont) {}
		iterator(const iterator &other) : node_ptr(other.node_ptr), container(other.container) {}

		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
            iterator temp = *this;
            if (node_ptr) {
                node_ptr = node_ptr->next_in_list;
            } else {
                throw invalid_iterator();
            }
            return temp;
        }

		/**
		 * TODO ++iter
		 */
		iterator & operator++() {
            if (node_ptr) {
                node_ptr = node_ptr->next_in_list;
            } else {
                throw invalid_iterator();
            }
            return *this;
        }

		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
            iterator temp = *this;
            if (node_ptr == nullptr) {
                // end() iterator, move to last element
                if (container && container->tail) {
                    node_ptr = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else if (node_ptr->prev_in_list) {
                node_ptr = node_ptr->prev_in_list;
            } else {
                throw invalid_iterator();
            }
            return temp;
        }

		/**
		 * TODO --iter
		 */
		iterator & operator--() {
            if (node_ptr == nullptr) {
                // end() iterator, move to last element
                if (container && container->tail) {
                    node_ptr = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else if (node_ptr->prev_in_list) {
                node_ptr = node_ptr->prev_in_list;
            } else {
                throw invalid_iterator();
            }
            return *this;
        }

		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		value_type & operator*() const {
            if (!node_ptr) {
                throw invalid_iterator();
            }
            return node_ptr->data;
        }

		bool operator==(const iterator &rhs) const {
            return node_ptr == rhs.node_ptr && container == rhs.container;
        }

		bool operator==(const const_iterator &rhs) const;

		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

		bool operator!=(const const_iterator &rhs) const;

		/**
		 * for the support of it->first.
		 * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
		 */
		value_type* operator->() const {
            if (!node_ptr) {
                throw invalid_iterator();
            }
            return &(node_ptr->data);
        }

        // Friend declarations
        friend class linked_hashmap;
        friend class const_iterator;
	};

	class const_iterator {
	private:
		const Node* node_ptr;
        const linked_hashmap* container;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = const value_type*;
		using reference = const value_type&;
		using iterator_category = std::output_iterator_tag;

		const_iterator() : node_ptr(nullptr), container(nullptr) {}
		const_iterator(const Node* node, const linked_hashmap* cont) : node_ptr(node), container(cont) {}
		const_iterator(const const_iterator &other) : node_ptr(other.node_ptr), container(other.container) {}
		const_iterator(const iterator &other) : node_ptr(other.node_ptr), container(other.container) {}

		const_iterator operator++(int) {
            const_iterator temp = *this;
            if (node_ptr) {
                node_ptr = node_ptr->next_in_list;
            } else {
                throw invalid_iterator();
            }
            return temp;
        }

        const_iterator & operator++() {
            if (node_ptr) {
                node_ptr = node_ptr->next_in_list;
            } else {
                throw invalid_iterator();
            }
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator temp = *this;
            if (node_ptr == nullptr) {
                // end() iterator, move to last element
                if (container && container->tail) {
                    node_ptr = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else if (node_ptr->prev_in_list) {
                node_ptr = node_ptr->prev_in_list;
            } else {
                throw invalid_iterator();
            }
            return temp;
        }

        const_iterator & operator--() {
            if (node_ptr == nullptr) {
                // end() iterator, move to last element
                if (container && container->tail) {
                    node_ptr = container->tail;
                } else {
                    throw invalid_iterator();
                }
            } else if (node_ptr->prev_in_list) {
                node_ptr = node_ptr->prev_in_list;
            } else {
                throw invalid_iterator();
            }
            return *this;
        }

        const value_type & operator*() const {
            if (!node_ptr) {
                throw invalid_iterator();
            }
            return node_ptr->data;
        }

        bool operator==(const const_iterator &rhs) const {
            return node_ptr == rhs.node_ptr && container == rhs.container;
        }

        bool operator==(const iterator &rhs) const {
            return node_ptr == rhs.node_ptr && container == rhs.container;
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }

        const value_type* operator->() const {
            if (!node_ptr) {
                throw invalid_iterator();
            }
            return &(node_ptr->data);
        }

        friend class linked_hashmap;
	};

	/**
	 * TODO two constructors
	 */
	linked_hashmap() : bucket_count(INITIAL_CAPACITY), element_count(0), head(nullptr), tail(nullptr) {
        buckets = new Node*[bucket_count]();
    }

	linked_hashmap(const linked_hashmap &other) : bucket_count(other.bucket_count), element_count(0), head(nullptr), tail(nullptr) {
        buckets = new Node*[bucket_count]();
        hasher = other.hasher;
        key_equal = other.key_equal;

        // Copy elements in insertion order
        const Node* current = other.head;
        while (current) {
            insert(current->data);
            current = current->next_in_list;
        }
    }

	/**
	 * TODO assignment operator
	 */
	linked_hashmap & operator=(const linked_hashmap &other) {
        if (this == &other) {
            return *this;
        }

        // Clear current contents
        clear();
        delete[] buckets;

        // Copy from other
        bucket_count = other.bucket_count;
        element_count = 0;
        head = nullptr;
        tail = nullptr;
        hasher = other.hasher;
        key_equal = other.key_equal;

        buckets = new Node*[bucket_count]();

        // Copy elements in insertion order
        const Node* current = other.head;
        while (current) {
            insert(current->data);
            current = current->next_in_list;
        }

        return *this;
    }

	/**
	 * TODO Destructors
	 */
	~linked_hashmap() {
        clear();
        delete[] buckets;
    }

	/**
	 * TODO
	 * access specified element with bounds checking
	 * Returns a reference to the mapped value of the element with key equivalent to key.
	 * If no such element exists, an exception of type `index_out_of_bound'
	 */
	T & at(const Key &key) {
        iterator it = find(key);
        if (it == end()) {
            throw index_out_of_bound();
        }
        return it->second;
    }

	const T & at(const Key &key) const {
        const_iterator it = find(key);
        if (it == cend()) {
            throw index_out_of_bound();
        }
        return it->second;
    }

	/**
	 * TODO
	 * access specified element
	 * Returns a reference to the value that is mapped to a key equivalent to key,
	 *   performing an insertion if such key does not already exist.
	 */
	T & operator[](const Key &key) {
        iterator it = find(key);
        if (it != end()) {
            return it->second;
        }

        // Insert default value
        T default_value = T();
        pair<iterator, bool> result = insert(value_type(key, default_value));
        return result.first->second;
    }

	/**
	 * behave like at() throw index_out_of_bound if such key does not exist.
	 */
	const T & operator[](const Key &key) const {
        return at(key);
    }

	/**
	 * return a iterator to the beginning
	 */
	iterator begin() {
        return iterator(head, this);
    }

	const_iterator cbegin() const {
        return const_iterator(head, this);
    }

	/**
	 * return a iterator to the end
	 * in fact, it returns past-the-end.
	 */
	iterator end() {
        return iterator(nullptr, this);
    }

	const_iterator cend() const {
        return const_iterator(nullptr, this);
    }

	/**
	 * checks whether the container is empty
	 * return true if empty, otherwise false.
	 */
	bool empty() const {
        return element_count == 0;
    }

	/**
	 * returns the number of elements.
	 */
	size_t size() const {
        return element_count;
    }

	/**
	 * clears the contents
	 */
	void clear() {
        clear_buckets();
        head = nullptr;
        tail = nullptr;
        element_count = 0;
    }

	/**
	 * insert an element.
	 * return a pair, the first of the pair is
	 *   the iterator to the new element (or the element that prevented the insertion),
	 *   the second one is true if insert successfully, or false.
	 */
	pair<iterator, bool> insert(const value_type &value) {
        // Check if key already exists
        iterator it = find(value.first);
        if (it != end()) {
            return pair<iterator, bool>(it, false);
        }

        // Check if rehashing is needed
        if (element_count >= bucket_count * LOAD_FACTOR_THRESHOLD) {
            rehash();
        }

        // Create new node
        Node* new_node = new Node(value);

        // Add to hash table
        size_t index = hash_index(value.first);
        new_node->next_in_bucket = buckets[index];
        buckets[index] = new_node;

        // Add to end of insertion order list
        if (!head) {
            head = new_node;
            tail = new_node;
        } else {
            new_node->prev_in_list = tail;
            tail->next_in_list = new_node;
            tail = new_node;
        }

        element_count++;
        return pair<iterator, bool>(iterator(new_node, this), true);
    }

	/**
	 * erase the element at pos.
	 *
	 * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
	 */
	void erase(iterator pos) {
        if (pos.container != this || pos.node_ptr == nullptr) {
            throw invalid_iterator();
        }

        Node* node = pos.node_ptr;

        // Remove from hash table chain
        size_t index = hash_index(node->data.first);
        Node* prev_in_bucket = nullptr;
        Node* current = buckets[index];

        while (current) {
            if (current == node) {
                if (prev_in_bucket) {
                    prev_in_bucket->next_in_bucket = current->next_in_bucket;
                } else {
                    buckets[index] = current->next_in_bucket;
                }
                break;
            }
            prev_in_bucket = current;
            current = current->next_in_bucket;
        }

        // Remove from insertion order list
        if (node->prev_in_list) {
            node->prev_in_list->next_in_list = node->next_in_list;
        } else {
            head = node->next_in_list;
        }

        if (node->next_in_list) {
            node->next_in_list->prev_in_list = node->prev_in_list;
        } else {
            tail = node->prev_in_list;
        }

        delete node;
        element_count--;
    }

	/**
	 * Returns the number of elements with key
	 *   that compares equivalent to the specified argument,
	 *   which is either 1 or 0
	 *     since this container does not allow duplicates.
	 */
	size_t count(const Key &key) const {
        return find(key) != cend() ? 1 : 0;
    }

	/**
	 * Finds an element with key equivalent to key.
	 * key value of the element to search for.
	 * Iterator to an element with key equivalent to key.
	 *   If no such element is found, past-the-end (see end()) iterator is returned.
	 */
	iterator find(const Key &key) {
        size_t index = hash_index(key);
        Node* current = buckets[index];

        while (current) {
            if (key_equal(current->data.first, key)) {
                return iterator(current, this);
            }
            current = current->next_in_bucket;
        }

        return end();
    }

	const_iterator find(const Key &key) const {
        size_t index = hash_index(key);
        Node* current = buckets[index];

        while (current) {
            if (key_equal(current->data.first, key)) {
                return const_iterator(current, this);
            }
            current = current->next_in_bucket;
        }

        return cend();
    }
};

// Static member initialization
template<class Key, class T, class Hash, class Equal>
const double linked_hashmap<Key, T, Hash, Equal>::LOAD_FACTOR_THRESHOLD = 0.75;

// Implement iterator comparison operators that depend on const_iterator
template<class Key, class T, class Hash, class Equal>
bool linked_hashmap<Key, T, Hash, Equal>::iterator::operator==(const const_iterator &rhs) const {
    return node_ptr == rhs.node_ptr && container == rhs.container;
}

template<class Key, class T, class Hash, class Equal>
bool linked_hashmap<Key, T, Hash, Equal>::iterator::operator!=(const const_iterator &rhs) const {
    return !(*this == rhs);
}

}

#endif