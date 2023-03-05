#include <iostream>
#include <vector>
#include <iterator>
#include <memory>
#include <optional>
#include <algorithm>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    using ItemType = std::pair<const KeyType, ValueType>;

    struct Bucket {
        ItemType *item;
        size_t dist{};

        Bucket() {
            dist = 0;
            item = nullptr;
        }

        Bucket(ItemType *item, size_t dist) : item(item), dist(dist) {}
    };

    size_t capacity_ = 0;
    size_t elems_count_ = 0;
    double expand_load_factor_ = 0.84;
    double squeeze_load_factor_ = 0.25;
    std::vector<std::optional<Bucket>> buckets_;
    Hash hasher_;
    ItemType *last = new ItemType({KeyType(), ValueType()});

public:
    class iterator {

    public:
        iterator() {
            ptr = nullptr;
        }

        explicit iterator(std::optional<Bucket> *ptr) : ptr(ptr) {
        }

        iterator operator++() {
            ++ptr;
            while (true) {
                if (ptr->has_value()) {
                    break;
                }
                ++ptr;
            }
            return *this;
        }

        iterator operator++(int) {
            auto old = *this;
            ++ptr;
            while (true) {
                if (ptr->has_value()) {
                    break;
                }
                ++ptr;
            }
            return old;
        }

        ItemType &operator*() {
            return *(ptr->value().item);
        }

        ItemType *operator->() {
            return ptr->value().item;
        }

        bool operator==(const HashMap::iterator &other) {
            return ptr == other.ptr;
        }

        bool operator!=(const HashMap::iterator &other) {
            return ptr != other.ptr;
        }

        std::optional<Bucket> *ptr;
    };

    class const_iterator {

    public:

        const_iterator() {
            ptr = nullptr;
        }

        explicit const_iterator(const std::optional<Bucket> *ptr) : ptr(ptr) {
        }

        const_iterator operator++() {
            ++ptr;
            while (true) {
                if (ptr->has_value()) {
                    break;
                }
                ++ptr;
            }
            return *this;
        }

        const_iterator operator++(int) {
            auto old = *this;
            ++ptr;
            while (true) {
                if (ptr->has_value()) {
                    break;
                }
                ++ptr;
            }
            return old;
        }

        const ItemType &operator*() const {
            return *(ptr->value().item);
        }

        const ItemType *operator->() const {
            return ptr->value().item;
        }

        bool operator==(const HashMap::const_iterator &other) {
            return ptr == other.ptr;
        }

        bool operator!=(const HashMap::const_iterator &other) {
            return ptr != other.ptr;
        }

        const std::optional<Bucket> *ptr;
    };

    iterator begin() {
        if (elems_count_ == 0) {
            return end();
        }
        return iterator(&buckets_[find_first_after(0)]);
    }

    iterator end() {
        return iterator(&buckets_[capacity_]);
    }

    const_iterator begin() const {
        if (elems_count_ == 0) {
            return end();
        }
        return const_iterator(&buckets_[find_first_after(0)]);
    }

    const_iterator end() const {
        return const_iterator(&buckets_[capacity_]);
    }


    size_t find_first_after(size_t pos) const {
        for (size_t cur_pos = pos; cur_pos < capacity_; ++cur_pos) {
            if (buckets_[cur_pos].has_value()) {
                return cur_pos;
            }
        }
        return 0;
    }


    explicit HashMap(Hash Hasher = Hash()) : hasher_(Hasher) {
        capacity_ = 1;
        buckets_.resize(capacity_ + 1);
        buckets_[capacity_] = Bucket(last, 0);
    }

    HashMap(std::initializer_list<ItemType> init_list, Hash Hasher = Hash()) : hasher_(Hasher) {
        if (capacity_ == 0) {
            capacity_ = 1;
        }
        buckets_.resize(capacity_ + 1);
        buckets_[capacity_] = Bucket(last, 0);
        hasher_ = Hasher;
        for (auto iter = init_list.begin(); iter != init_list.end(); ++iter) {
            insert(*iter);
        }
    }

    template<class ForwardIterator>
    HashMap(ForwardIterator begin, ForwardIterator end, Hash Hasher = Hash()) : hasher_(Hasher) {
        if (capacity_ == 0) {
            capacity_ = 1;
        }
        buckets_.resize(capacity_ + 1);
        buckets_[capacity_] = Bucket(last, 0);
        hasher_ = Hasher;
        for (auto iter = begin; iter != end; ++iter) {
            insert(*iter);
        }
    }


    HashMap(const HashMap<KeyType, ValueType, Hash> &other) {
        elems_count_ = other.elems_count_;
        capacity_ = other.capacity_;
        buckets_.resize(capacity_ + 1);
        buckets_[capacity_] = Bucket(last, 0);
        hasher_ = other.hasher_;
        for (size_t pos = 0; pos < capacity_; ++pos) {
            if (other.buckets_[pos].has_value()) {
                buckets_[pos] = Bucket(new ItemType({*other.buckets_[pos]->item}), other.buckets_[pos]->dist);
            }
        }
    }

    HashMap &operator=(const HashMap<KeyType, ValueType, Hash> &other) {
        if (this == &other) {
            return *this;
        }
        buckets_[capacity_].reset();
        for (size_t pos = 0; pos < capacity_; ++pos) {
            if (buckets_[pos].has_value()) {
                delete buckets_[pos]->item;
                buckets_[pos].reset();
            }
        }
        elems_count_ = other.elems_count_;
        capacity_ = other.capacity_;
        if (buckets_.size() < capacity_ + 1) {
            buckets_.resize(capacity_ + 1);
        } else {
            while (buckets_.size() > capacity_ + 1) {
                buckets_.pop_back();
            }
        }
        buckets_[capacity_] = Bucket(last, 0);
        hasher_ = other.hasher_;
        for (size_t pos = 0; pos < capacity_; ++pos) {
            if (other.buckets_[pos].has_value()) {
                buckets_[pos] = Bucket(new ItemType({*other.buckets_[pos]->item}), other.buckets_[pos]->dist);
            }
        }
        return *this;
    }

    ~HashMap() {
        delete last;
        for (size_t pos = 0; pos < capacity_; ++pos) {
            if (buckets_[pos].has_value()) {
                delete buckets_[pos]->item;
                buckets_[pos].reset();
            }
        }
    }

    void insert(const ItemType &item) {
        double cur_load_factor = (double) elems_count_ / (double) capacity_;
        if (cur_load_factor > expand_load_factor_) {
            refresh_table(true);
        }
        size_t cur_pos = hasher_(item.first) % capacity_;
        auto *new_item = new ItemType(item);
        std::optional<Bucket> cur_item = Bucket(new_item, 0);
        while (true) {
            if (buckets_[cur_pos].has_value()) {
                if (buckets_[cur_pos]->item->first == cur_item->item->first) {
                    delete new_item;
                    return;
                }
                if (cur_item->dist > buckets_[cur_pos]->dist) {
                    std::swap(cur_item, buckets_[cur_pos]);
                }
                ++cur_item->dist;
                cur_pos = (cur_pos + 1) % capacity_;
                continue;
            }
            buckets_[cur_pos] = cur_item;
            break;
        }
        ++elems_count_;
    }

    void refresh_table(bool add) {
        std::vector<ItemType *> temp;
        for (size_t i = 0; i < capacity_; ++i) {
            if (buckets_[i].has_value()) {
                temp.push_back(buckets_[i]->item);
                buckets_[i].reset();
            }
        }
        buckets_[capacity_].reset();
        if (add) {
            capacity_ *= 2;
            buckets_.resize(capacity_ + 1);
            buckets_[capacity_] = Bucket(last, 0);
        } else {
            capacity_ = std::max(capacity_ / 2, (size_t) 1);
            while (buckets_.size() > capacity_) buckets_.pop_back();
            buckets_.push_back(Bucket(last, 0));
        }
        elems_count_ = 0;
        for (auto &item: temp) {
            insert(*item);
            delete item;
        }
    }

    size_t find_pos(const KeyType &key) const {
        size_t cur_pos = hasher_(key) % capacity_;
        size_t distance = 0;
        while (true) {
            if (!buckets_[cur_pos].has_value() || distance > buckets_[cur_pos]->dist) {
                return capacity_;
            }
            if (buckets_[cur_pos]->item->first == key) {
                return cur_pos;
            }
            cur_pos = (cur_pos + 1) % capacity_;
            ++distance;
        }
    }

    const_iterator find(const KeyType &key) const {
        return const_iterator(&buckets_[find_pos(key)]);
    }

    iterator find(const KeyType &key) {
        return iterator(&buckets_[find_pos(key)]);
    }

    void erase(const KeyType &key) {
        double cur_load_factor = (double) elems_count_ / (double) capacity_;
        if (cur_load_factor < squeeze_load_factor_) {
            refresh_table(false);
        }
        size_t cur_pos = hasher_(key) % capacity_;
        size_t distance = 0;
        while (true) {
            if (!buckets_[cur_pos].has_value() || distance > buckets_[cur_pos]->dist) {
                return;
            }
            if (buckets_[cur_pos]->item->first == key) {
                break;
            }
            cur_pos = (cur_pos + 1) % capacity_;
            ++distance;
        }
        delete buckets_[cur_pos]->item;
        buckets_[cur_pos].reset();
        --elems_count_;
        cur_pos = (cur_pos + 1) % capacity_;
        while (buckets_[cur_pos].has_value() && buckets_[cur_pos]->dist > 0) {
            --buckets_[cur_pos]->dist;
            std::swap(buckets_[cur_pos], buckets_[(cur_pos - 1 + capacity_) % capacity_]);
            cur_pos = (cur_pos + 1) % capacity_;
        }
    }

    ValueType &operator[](const KeyType &key) {
        iterator iter = find(key);
        if (iter == end()) {
            insert(ItemType{key, ValueType()});
            iter = find(key);
        }
        return iter->second;
    }

    const ValueType &at(const KeyType &key) const {
        const_iterator iter = find(key);
        if (iter == end()) {
            throw std::out_of_range("No such key!");
        }
        return iter->second;
    }

    void clear() {
        buckets_.back().reset();
        while (!buckets_.empty()) {
            if (buckets_.back().has_value()) {
                delete buckets_.back()->item;
            }
            buckets_.back().reset();
            buckets_.pop_back();
        }
        capacity_ = 1;
        buckets_.resize(capacity_ + 1);
        buckets_[capacity_] = Bucket(last, 0);
        elems_count_ = 0;
    }

    void PrintItems() const {
        for (auto iter = begin(); iter != end(); ++iter) {
            std::cout << (*iter).first << " " << (*iter).second << "\n";
        }
    }

    size_t size() const {
        return elems_count_;
    }

    bool empty() const {
        return elems_count_ == 0;
    }

    Hash hash_function() const {
        return hasher_;
    }
};