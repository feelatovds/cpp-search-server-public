#pragma once

#include <vector>
#include <map>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) :
            buckets_(std::move(std::vector<Bucket>(bucket_count)))
    {
    }

    Access operator[](const Key& key) {
        const auto num_bucket = static_cast<uint64_t>(key) % buckets_.size();
        return {std::lock_guard(buckets_[num_bucket].bucket_guard), buckets_[num_bucket].bucket[key]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& bucket : buckets_) {
            std::lock_guard guard(bucket.bucket_guard);
            result.merge(bucket.bucket);
        }
        return result;
    }

    auto Erase(const Key& key) {
        const auto num_bucket = static_cast<uint64_t>(key) % buckets_.size();
        return buckets_[num_bucket].bucket.erase(key);
    }

    size_t GetSize() const {
        return buckets_.size();
    }

private:
    struct Bucket {
        std::map<Key, Value> bucket;
        std::mutex bucket_guard;
    };

    std::vector<Bucket> buckets_;
};

