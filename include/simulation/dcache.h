#pragma once
#include "declarations.h"
#include "utility/error.h"
#include <cstddef>

namespace kupi {

using Addr_t = dark::target_size_t;
using Time_t = std::size_t;

static constexpr Addr_t CacheLineSize  = 64;
static constexpr Addr_t CacheMaxSize   = 4; // 每个组中的缓存行数
static constexpr Addr_t CacheGroupSize = 2; // 缓存组数

struct CacheLine {
    Addr_t id{};
    bool valid{};
    bool dirty{};
    Time_t timestamp{};

    void update(Addr_t new_id, Time_t cur_timestamp) {
        this->id        = new_id;
        this->valid     = true;
        this->dirty     = false;
        this->timestamp = cur_timestamp;
    }
};

struct CacheCounter {
    Time_t count_load_real{};
    Time_t count_write_back{};
};

class CacheGroup {
public:
    explicit CacheGroup() = default;

    bool load(Addr_t low, Addr_t high, CacheCounter &counter) {
        ++cur_timestamp;
        auto line_id = low / CacheLineSize;
        auto lineLow = low / CacheLineSize * CacheLineSize;
        if (lineLow + CacheLineSize < high)
            dark::unreachable("CacheGroup::load address range too large");
        if (this->check_hit(line_id, false))
            return true;
        this->allocate_one(line_id, counter);
        return false;
    }

    bool store(Addr_t low, Addr_t high, CacheCounter &counter) {
        ++cur_timestamp;
        auto line_id = low / CacheLineSize;
        auto lineLow = low / CacheLineSize * CacheLineSize;
        if (lineLow + CacheLineSize < high)
            dark::unreachable("CacheGroup::store address range too large");
        if (this->check_hit(line_id, true))
            return true;
        this->allocate_one(line_id, counter);
        return false;
    }

private:
    Time_t cur_timestamp = 0;
    CacheLine cache[CacheMaxSize];

    void allocate_one(Addr_t id, CacheCounter &counter) {
        CacheLine *min_line = &cache[0];
        for (auto &line : cache) {
            if (!line.valid)
                return line.update(id, cur_timestamp);
            if (line.timestamp < min_line->timestamp)
                min_line = &line;
        }

        counter.count_load_real += 1;
        counter.count_write_back += min_line->dirty;
        min_line->update(id, cur_timestamp);
    }

    bool check_hit(Addr_t id, bool is_write) {
        for (auto &line : cache) {
            if (line.valid && line.id == id) {
                line.timestamp = cur_timestamp;
                line.dirty |= is_write;
                return true;
            }
        }
        return false;
    }
};

class Cache {
public:
    explicit Cache() = default;

    bool load(Addr_t low, Addr_t high) {
        Addr_t group_index = get_group_index(low);
        return groups[group_index].load(low, high, counter);
    }

    bool store(Addr_t low, Addr_t high) {
        Addr_t group_index = get_group_index(low);
        return groups[group_index].store(low, high, counter);
    }

    auto get_load() const -> std::size_t { return counter.count_load_real; }

    auto get_store() const -> std::size_t { return counter.count_write_back; }

private:
    friend class CacheGroup;

    CacheGroup groups[CacheGroupSize]; // 组关联缓存中的各个组
    CacheCounter counter;

    static constexpr Addr_t get_group_index(Addr_t addr) {
        return (addr / CacheLineSize) % CacheGroupSize;
    }
};

} // namespace kupi
