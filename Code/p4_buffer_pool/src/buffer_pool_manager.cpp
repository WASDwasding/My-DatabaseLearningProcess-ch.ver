#include "buffer_pool_manager.h"

#include <sstream>

namespace p4 {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), pages_(pool_size), replacer_(pool_size) {
  for (frame_id_t i = 0; i < static_cast<frame_id_t>(pool_size); ++i) {
    free_list_.insert(i);
  }
}

auto BufferPoolManager::FindFreeFrame() -> std::optional<frame_id_t> {
  if (!free_list_.empty()) {
    frame_id_t frame = *free_list_.begin();
    free_list_.erase(frame);
    return frame;
  }
  return replacer_.Victim();
}

auto BufferPoolManager::AllocateFrameForPage(page_id_t page_id, bool is_new) -> Page * {
  auto frame_opt = FindFreeFrame();
  if (!frame_opt.has_value()) {
    return nullptr;  // all frames pinned
  }
  frame_id_t frame = *frame_opt;
  Page &page = pages_[frame];

  // Evict old resident if any.
  if (page.GetPageId() != INVALID_PAGE_ID) {
    if (page.IsDirty()) {
      disk_manager_->WritePage(page.GetPageId(), page.GetData());
    }
    page_table_.erase(page.GetPageId());
    replacer_.Remove(frame);
  }

  page.Reset();
  page.SetPageId(page_id);
  page.Pin();
  page.SetDirty(false);

  if (!is_new) {
    disk_manager_->ReadPage(page_id, page.GetData());
  }

  page_table_[page_id] = frame;
  replacer_.SetEvictable(frame, false);
  return &page;
}

auto BufferPoolManager::NewPage(page_id_t *page_id_out) -> Page * {
  page_id_t page_id = disk_manager_->AllocatePage();
  Page *page = AllocateFrameForPage(page_id, /*is_new=*/true);
  if (page == nullptr) {
    disk_manager_->DeallocatePage(page_id);
    return nullptr;
  }
  if (page_id_out != nullptr) {
    *page_id_out = page_id;
  }
  return page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id) -> Page * {
  auto it = page_table_.find(page_id);
  if (it != page_table_.end()) {
    frame_id_t frame = it->second;
    Page &page = pages_[frame];
    page.Pin();
    replacer_.SetEvictable(frame, false);
    // Optional: if already evictable-tracked, RecordAccess would move LRU;
    // here pin>0 means non-evictable, so no RecordAccess.
    return &page;
  }
  return AllocateFrameForPage(page_id, /*is_new=*/false);
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return false;
  }
  frame_id_t frame = it->second;
  Page &page = pages_[frame];
  if (!page.Unpin()) {
    return false;
  }
  if (is_dirty) {
    page.SetDirty(true);
  }
  if (page.GetPinCount() == 0) {
    replacer_.SetEvictable(frame, true);
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return false;
  }
  frame_id_t frame = it->second;
  Page &page = pages_[frame];
  disk_manager_->WritePage(page_id, page.GetData());
  page.SetDirty(false);
  return true;
}

void BufferPoolManager::FlushAllPages() {
  for (const auto &[page_id, _] : page_table_) {
    FlushPage(page_id);
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  auto it = page_table_.find(page_id);
  if (it != page_table_.end()) {
    frame_id_t frame = it->second;
    Page &page = pages_[frame];
    if (page.GetPinCount() > 0) {
      return false;
    }
    replacer_.Remove(frame);
    page_table_.erase(it);
    page.Reset();
    free_list_.insert(frame);
  }
  disk_manager_->DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::CheckInvariants() const -> std::string {
  std::ostringstream err;

  for (const auto &[page_id, frame] : page_table_) {
    if (frame < 0 || static_cast<size_t>(frame) >= pool_size_) {
      err << "page_table maps to invalid frame";
      return err.str();
    }
    if (pages_[frame].GetPageId() != page_id) {
      err << "page_table/frame page_id mismatch";
      return err.str();
    }
    if (pages_[frame].GetPinCount() < 0) {
      err << "pin_count underflow";
      return err.str();
    }
    if (free_list_.count(frame) > 0) {
      err << "occupied frame also in free_list";
      return err.str();
    }
  }

  for (frame_id_t frame : free_list_) {
    if (pages_[frame].GetPageId() != INVALID_PAGE_ID) {
      err << "free frame still occupied";
      return err.str();
    }
  }

  return {};
}

}  // namespace p4
