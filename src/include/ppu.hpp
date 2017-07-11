#pragma once
#include "common.hpp"

namespace PPU {
enum Mirroring { VERTICAL, HORIZONTAL };

u8 access_write(u16 addr, u8 v = 0);
u8 access_read(u16 addr, u8 v = 0);

template <bool write> u8 access(u16 index, u8 v = 0) {
    return access_write(index,v);
};
template<> u8 access<false>(u16 index, u8 v){
    return access_read(index,v);
}

void set_mirroring(Mirroring mode);
void step();
void reset();

}
