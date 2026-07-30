#pragma once

#include <cstddef>
#include <cstdint>

namespace p4 {

using page_id_t = int32_t;
using frame_id_t = int32_t;

constexpr page_id_t INVALID_PAGE_ID = -1;
constexpr frame_id_t INVALID_FRAME_ID = -1;
constexpr size_t kPageSize = 4096;

}  // namespace p4
