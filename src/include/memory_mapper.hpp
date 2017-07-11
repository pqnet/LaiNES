namespace memory_mapper {

template<typename address_t,typename E>
class map {
    template<size_t size>
    auto get_map(address_t address) {
        return static_cast<E&>(*this).get_map<size>(address);
    }
    template<size_t size>
    auto get_map(address_t address) const {
        return static_cast<E const &>(*this).get_map<size>(address);
    }
    // can read and can write operations should be determined at compile time.
};

template<size_t ssize>
struct map_wrapper
{
    const void*storage;
    ptrdiff_t offset;
    const std::function<void*(void*,ptrdiff_t,const void*,size_t)> writeCb;
    const std::function<const void*(const void*,ptrdiff_t,size_t)> readCb;

    static constexpr const void* nop_read(const void* storage,ptrdiff_t offset, size_t size) { return storage+offset; }
    static constexpr void* nop_write(void* storage,ptrdiff_t offset, void* data, size_t size) { return memcpy(storage+offset,data,size); }

    template<class Rcb,Wcb>
    map_wrapper(const void* a_storage, ptrdiff_t a_offset, Rcb a_readCb = nullptr, Wc a_writeCb = nullptr)
    : storage{a_storage}
    , offset{a_offset}
    , readCb{a_readCb}
    , writeCb{a_writeCb}
    {}

    template<typename U>
    auto operator std::enable_if_t<sizeof(std::remove_reference_t<U>) <= ssize, U&&>() const {
        return std::forward<U>(*static_cast<U*>(readCb?readCb(storage,offset,sizeof<U>):storage+offset));
    }

    template<typename U>
    auto operator=( std::enable_if_t<sizeof(std::remove_reference_t<U>) <= ssize, U&&> rhs ) {
        return *static_cast<U*>(writeCb?writeCb(const_cast<void*>(storage),offset,&rhs,sizeof<U>):memcpy(storage+offset,&rhs,size));
    }

    bool is_valid() const { return storage != nullptr; }
};

template<typename address_t, address_t offset, typename T>
struct objectMap: public map<address_t, objectMap<address_t,offset,T>> {
    static constexpr address_t map_start = offset;
    static constexpr address_t map_end = offset+sizeof(T);

    T storage;
    template<size_t size>
    auto get_map(address_t address) {
        bool valid = address>=map_start && address+size<=map_end;
        return map_wrapper<size>(valid?storage:nullptr,address-map_start);
    }

    template<size_t size>
    const auto get_map(address_t address) const {
        bool valid = address>=map_start && address+size<=map_end;
        return map_wrapper<size>(valid?storage:nullptr,address-map_start);
    }
};


template<typename address_t, address_t offset, size_t size, class Fr, class Fw>
struct funmap: public map<address_t, funmap<address_t,offset,size,Fr,Fw> > {
    static constexpr address_t map_start = offset;
    static constexpr address_t map_end = offset+size;
    void* storage;

    template<size_t size>
    auto get_map(address_t address) {
        bool valid = address>=map_start && address+size<=map_end;
        return map_wrapper(valid?storage:nullptr,address-map_start,reader,writer);
    }

    template<size_t size>
    auto get_map(address_t address) const {
        bool valid = address>=map_start && address+size<=map_end;
        return const_map_wrapper(valid?storage:nullptr,address-map_start,reader);
    }
private:
    Fr reader;
    Fw writer;
};

// template<typename address_t,typename E1,typename E2>
// struct seq_combine: public map<address_t,seq_combine<address_t,E1,E2>> {
//     static constexpr address_t map_start = min(E1::map_start,E2::map_start);
//     static constexpr address_t map_end = min(E1::map_end,E2::map_end);
//     static const bool left_const = std::is_const<E1>;
//     static const bool right_const = std::is_const<E2>;
//     using left_t = std::remove_reference_t<E1>
//     using right_t = std::remove_reference_t<E2>
//     std::conditional_t<left_const, const left_t, left_t> e1;
//     std::conditional_t<right_const, const right_t, right_t> e2;
//     seq_combine(map<left_t> const &&e1; map<right_t> const &&e2)
//     :e1{static_cast<E1&&>(std::move(e1))}
//     ,e2{static_cast<E2&&>(std::move(e2))}
//     {}

//     template<size_t size>
//     auto get_map(address_t address) {
//         auto m1 = e1.get_map<size>(address);
//         auto m2 = e1.get_map<size>(address);
//         auto rcb = []
//         return map_wrapper(nullptr,0,rcb,wcb);
//     }

// }

template<typename address_t, typename E1, typename E2>
struct pivot_combine: public map<address_t,pivot_combine<address_t,E1,E2>> {
    static constexpr address_t map_start = E1::map_start;
    static constexpr address_t map_end = E2::map_end;
    static_assert(map_start < map_end);
    pivot_combine()

    struct pair_wrapper {
        union{}
    }
    template<size_t size>
    auto get_map(address_t address) {
        bool valid = address>=map_start && address+size<=map_end;
        return map_wrapper(valid?storage:nullptr,address-map_start,nop_read,nop_write);
    }

    template<size_t size>
    auto get_map(address_t address) const {
        bool valid = address>=map_start && address+size<=map_end;
        return const_map_wrapper(valid?storage:nullptr,address-map_start,nop_read);
    }
};

}

