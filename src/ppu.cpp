#include "ppu.hpp"
#include <thread>

namespace PPU {

struct PPURegisters {

};

// a pointer with deep copy semantic. Wrap values which can be copied but cannot be moved, enables move semantic.
// Also exposes all constructors of the wrapped value through make_unique
template<class T> struct deep_ptr {
    // proxies all overloads of make_unique to this object.
    // Which on turn proxies all public constructors of T
    // The final result is that we have a constructor for each constructor of T with the same parameters (and same semantic)
    template<class... Args>
    deep_ptr(Args&&... args): ref(std::make_unique(std::forward<Args>(args)...)){}

    auto operator=(deep_ptr&& other) { ref = std::move(other.ref); return *this; }
    auto operator=(deep_ptr const & other) { ref = std::make_unique(*other); return *this; }

    deep_ptr(deep_ptr&& other):ref(std::move(other.ref)) {}
    deep_ptr(const deep_ptr& other):ref(std::make_unique(*other)) {}

    auto operator *() { return *ref; }

private:
    std::unique_ptr<T> ref;
};

// a one-way lossy queue, where consumer can skip frames
template<class Data>
struct TripleBuffering {
    // memory guarantees needed:
    // if fetch_buffer returns true, the commit_buffer which stored that true (any part of it) must synchronize_with it
    // if feed_buffer_idx obtain a value, this need to be a writable buffer (the exchange)
    void commit_buffer(bool copy = true) {
        // writing true means that the buffer is ready to be drawn. Writes to the buffer shouldn't be moved past this point
        buffer_ready[feed_buffer_idx].store(true,std::memory_order_release);
        if(copy) {
            auto old_buffer = buffers[feed_buffer_idx];
            // if the number read has been written by the other thread, we can't start writing to this buffer until the other thread has
            // completed reading it
            swap_buffer_idx.exchange(feed_buffer_idx,std::memory_order_acquire);
            buffers[feed_buffer_idx] = std::move(old_buffer);
        } else
            swap_buffer_idx.exchange(feed_buffer_idx,std::memory_order_acquire);

    }
    // called by work thread, when the current buffer work is completed and the
    bool fetch_buffer() {
        // this may be relaxed, as it is only read from the same thread
        buffer_ready[work_buffer_idx].store(false,std::memory_order_relaxed);
        // this need to be commit, because it is marking the buffer as writable.
        swap_buffer_idx.exchange(work_buffer_idx,std::memory_order_release);
        // this need to be acquire, because if it's true the frame has to be marked complete
        return buffer_ready[work_buffer_idx].load(std::memory_order_acquire);
    }
    auto work_buffer(){
        return cref(*buffers[work_buffer_idx]);
    }
    auto feed_buffer(){
        return ref(*buffers[feed_buffer_idx]);
    }
    deep_ptr<Data> buffers[3];
    std::atomic_bool buffer_ready;
    int work_buffer_idx{0}; // only used by work thread
    std::atomic_int swap_buffer_idx{1}; // the buffer that noone is using right now
    int feed_buffer_idx{2}; // only used by command thread

};

struct PPU {
    std::thread ppu_thread;
    Mirroring mirroring;
    // triple buffering
    TripleBuffering<PPURegisters> registers;
    void thread_loop();
    // starts ppu thread
    void start() {
        ppu_thread = std::thread([&]{thread_loop();});
    }
    // stops ppu thread synchronously
    void stop() {
        if (ppu_thread.joinable())
        {
            stop_flag = true;
            ppu_thread.join();
        }
    }
    void step();
    void reset(){
        stop();
        // do reset things
        start();
    }
    u8 access_read(u16 addr, u8 v = 0);
    u8 access_write(u16 addr, u8 v = 0);
} g_ppu;

u8 access_write(u16 addr, u8 v = 0){
    return g_ppu.access_write(addr,v);
}
u8 access_read(u16 addr, u8 v = 0) {
    return g_ppu.access_read(addr,v);
}

// called only once, by the mapper, when a new cartridge is loaded.
// We can stop the thread, and be sure it is going to be started again
void set_mirroring(Mirroring mode);
void step();
void reset();
}