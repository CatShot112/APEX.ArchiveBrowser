/*
Original source code: https://www.burtleburtle.net/bob/c/lookup3.c
*/

#pragma once

#include <cstdint>

extern uint32_t hashlittle(const void* key, size_t length, uint32_t initval);
