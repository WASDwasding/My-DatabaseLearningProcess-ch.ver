#include "b_plus_tree.h"

#include <climits>
#include <functional>
#include <sstream>

namespace p8 {

BPlusTree::BPlusTree(size_t max_keys)
    : max_keys_(max_keys < 2 ? 2 : max_keys), min_keys_(max_keys_ / 2) {}

BPlusTree::~BPlusTree() { Destroy(root_); }

void BPlusTree::Destroy(Node *node) {
  if (node == nullptr) {
    return;
  }
  if (!node->is_leaf) {
    auto *in = static_cast<InternalNode *>(node);
    for (Node *c : in->children) {
      Destroy(c);
    }
  }
  delete node;
}

auto BPlusTree::Size() const -> size_t {
  size_t n = 0;
  for (auto it = Begin(); !it.IsEnd(); ++it) {
    ++n;
  }
  return n;
}

auto BPlusTree::Height() const -> int {
  if (root_ == nullptr) {
    return 0;
  }
  int h = 1;
  Node *cur = root_;
  while (!cur->is_leaf) {
    cur = static_cast<InternalNode *>(cur)->children[0];
    ++h;
  }
  return h;
}

auto BPlusTree::ChildIndex(const InternalNode *node, int key) -> size_t {
  // First i with key < keys[i]; else last child.
  size_t i = 0;
  while (i < node->keys.size() && !(key < node->keys[i])) {
    ++i;
  }
  return i;
}

auto BPlusTree::FindLeaf(int key) const -> LeafNode * {
  if (root_ == nullptr) {
    return nullptr;
  }
  Node *cur = root_;
  while (!cur->is_leaf) {
    auto *in = static_cast<InternalNode *>(cur);
    cur = in->children[ChildIndex(in, key)];
  }
  return static_cast<LeafNode *>(cur);
}

auto BPlusTree::FindLeafPath(int key, std::vector<InternalNode *> *path)
    -> LeafNode * {
  path->clear();
  if (root_ == nullptr) {
    return nullptr;
  }
  Node *cur = root_;
  while (!cur->is_leaf) {
    auto *in = static_cast<InternalNode *>(cur);
    path->push_back(in);
    cur = in->children[ChildIndex(in, key)];
  }
  return static_cast<LeafNode *>(cur);
}

auto BPlusTree::Get(int key) const -> std::optional<int> {
  LeafNode *leaf = FindLeaf(key);
  if (leaf == nullptr) {
    return std::nullopt;
  }
  for (size_t i = 0; i < leaf->keys.size(); ++i) {
    if (leaf->keys[i] == key) {
      return leaf->values[i];
    }
  }
  return std::nullopt;
}

auto BPlusTree::Insert(int key, int value) -> bool {
  if (root_ == nullptr) {
    auto *leaf = new LeafNode();
    leaf->keys.push_back(key);
    leaf->values.push_back(value);
    root_ = leaf;
    return true;
  }

  std::vector<InternalNode *> path;
  LeafNode *leaf = FindLeafPath(key, &path);

  // Update existing
  for (size_t i = 0; i < leaf->keys.size(); ++i) {
    if (leaf->keys[i] == key) {
      leaf->values[i] = value;
      return true;
    }
  }

  InsertIntoLeaf(leaf, key, value, path);
  return true;
}

void BPlusTree::InsertIntoLeaf(LeafNode *leaf, int key, int value,
                               std::vector<InternalNode *> &path) {
  // Insert in sorted order
  size_t i = 0;
  while (i < leaf->keys.size() && leaf->keys[i] < key) {
    ++i;
  }
  leaf->keys.insert(leaf->keys.begin() + static_cast<std::ptrdiff_t>(i), key);
  leaf->values.insert(leaf->values.begin() + static_cast<std::ptrdiff_t>(i),
                      value);

  if (leaf->keys.size() <= max_keys_) {
    return;
  }
  SplitLeaf(leaf, path);
}

void BPlusTree::SplitLeaf(LeafNode *leaf, std::vector<InternalNode *> &path) {
  auto *right = new LeafNode();
  const size_t total = leaf->keys.size();  // == max_keys_ + 1
  const size_t mid = total / 2;            // left keeps [0, mid), right [mid, total)

  right->keys.assign(leaf->keys.begin() + static_cast<std::ptrdiff_t>(mid),
                     leaf->keys.end());
  right->values.assign(leaf->values.begin() + static_cast<std::ptrdiff_t>(mid),
                       leaf->values.end());
  leaf->keys.resize(mid);
  leaf->values.resize(mid);

  right->next = leaf->next;
  leaf->next = right;

  const int sep = right->keys.front();  // copy separator into parent

  if (path.empty()) {
    // Root was this leaf -> new root
    auto *new_root = new InternalNode();
    new_root->keys.push_back(sep);
    new_root->children.push_back(leaf);
    new_root->children.push_back(right);
    root_ = new_root;
    return;
  }

  InsertIntoParent(path.back(), leaf, sep, right, path, path.size() - 1);
}

void BPlusTree::InsertIntoParent(InternalNode * /*left_parent_hint*/, Node *left,
                                 int key, Node *right,
                                 std::vector<InternalNode *> &path,
                                 size_t path_index) {
  InternalNode *parent = path[path_index];

  // Find position of left among children
  size_t pos = 0;
  while (pos < parent->children.size() && parent->children[pos] != left) {
    ++pos;
  }
  // Insert key at pos, right child at pos+1
  parent->keys.insert(parent->keys.begin() + static_cast<std::ptrdiff_t>(pos),
                      key);
  parent->children.insert(
      parent->children.begin() + static_cast<std::ptrdiff_t>(pos + 1), right);

  if (parent->keys.size() <= max_keys_) {
    return;
  }
  SplitInternal(parent, path, path_index);
}

void BPlusTree::SplitInternal(InternalNode *node,
                              std::vector<InternalNode *> &path,
                              size_t path_index) {
  // node has max_keys_ + 1 keys and max_keys_ + 2 children
  auto *right = new InternalNode();
  const size_t mid = node->keys.size() / 2;  // push up keys[mid]
  const int up_key = node->keys[mid];

  // Right gets keys(mid+1 .. end) and children(mid+1 .. end)
  right->keys.assign(node->keys.begin() + static_cast<std::ptrdiff_t>(mid + 1),
                     node->keys.end());
  right->children.assign(
      node->children.begin() + static_cast<std::ptrdiff_t>(mid + 1),
      node->children.end());

  // Left keeps keys[0..mid) and children[0..mid]
  node->keys.resize(mid);
  node->children.resize(mid + 1);

  if (path_index == 0) {
    // node was root
    auto *new_root = new InternalNode();
    new_root->keys.push_back(up_key);
    new_root->children.push_back(node);
    new_root->children.push_back(right);
    root_ = new_root;
    return;
  }

  InsertIntoParent(path[path_index - 1], node, up_key, right, path,
                   path_index - 1);
}

auto BPlusTree::Remove(int key) -> bool {
  if (root_ == nullptr) {
    return false;
  }
  std::vector<InternalNode *> path;
  LeafNode *leaf = FindLeafPath(key, &path);

  size_t idx = 0;
  while (idx < leaf->keys.size() && leaf->keys[idx] != key) {
    ++idx;
  }
  if (idx == leaf->keys.size()) {
    return false;
  }

  RemoveFromLeaf(leaf, idx, path);
  return true;
}

void BPlusTree::RemoveFromLeaf(LeafNode *leaf, size_t idx,
                               std::vector<InternalNode *> &path) {
  const bool removed_min = (idx == 0);
  leaf->keys.erase(leaf->keys.begin() + static_cast<std::ptrdiff_t>(idx));
  leaf->values.erase(leaf->values.begin() + static_cast<std::ptrdiff_t>(idx));

  // Update parent separator if we removed the leftmost key of this leaf
  // and leaf still has keys (new min may change separator pointing to this leaf)
  if (removed_min && !leaf->keys.empty() && !path.empty()) {
    InternalNode *parent = path.back();
    size_t ci = 0;
    while (ci < parent->children.size() && parent->children[ci] != leaf) {
      ++ci;
    }
    if (ci > 0 && ci < parent->children.size()) {
      // separator keys[ci-1] should be leaf's new min
      UpdateParentKey(parent, ci, leaf->keys.front());
    }
  }

  if (leaf == root_) {
    if (leaf->keys.empty()) {
      delete leaf;
      root_ = nullptr;
    }
    return;
  }

  if (leaf->keys.size() >= min_keys_) {
    return;
  }
  HandleUnderflow(leaf, path);
}

void BPlusTree::UpdateParentKey(InternalNode *parent, size_t child_idx,
                                int new_key) {
  // child_idx > 0: keys[child_idx - 1] separates children[child_idx-1] | children[child_idx]
  if (child_idx == 0 || child_idx > parent->keys.size()) {
    return;
  }
  parent->keys[child_idx - 1] = new_key;
  // May need to propagate if parent is not root and child_idx==0 of grandparent...
  // For leftmost of this parent, separator in grandparent may need update —
  // handled when removing min of leftmost leaf of a subtree via recursive climb
  // in RemoveFromLeaf only updates immediate parent. Climb for leftmost:
}

void BPlusTree::MaybeShrinkRoot() {
  if (root_ == nullptr || root_->is_leaf) {
    return;
  }
  auto *in = static_cast<InternalNode *>(root_);
  if (in->children.size() == 1) {
    Node *child = in->children[0];
    delete in;
    root_ = child;
  }
}

void BPlusTree::HandleUnderflow(Node *node, std::vector<InternalNode *> &path) {
  if (path.empty()) {
    MaybeShrinkRoot();
    return;
  }

  InternalNode *parent = path.back();
  size_t ci = 0;
  while (ci < parent->children.size() && parent->children[ci] != node) {
    ++ci;
  }

  if (node->is_leaf) {
    // Try borrow from left
    if (ci > 0) {
      auto *left = static_cast<LeafNode *>(parent->children[ci - 1]);
      if (left->keys.size() > min_keys_) {
        RedistributeLeaf(parent, ci, true);
        return;
      }
    }
    // Try borrow from right
    if (ci + 1 < parent->children.size()) {
      auto *right = static_cast<LeafNode *>(parent->children[ci + 1]);
      if (right->keys.size() > min_keys_) {
        RedistributeLeaf(parent, ci, false);
        return;
      }
    }
    // Merge with left or right
    if (ci > 0) {
      MergeLeaf(parent, ci - 1);
    } else {
      MergeLeaf(parent, ci);
    }
  } else {
    if (ci > 0) {
      auto *left = static_cast<InternalNode *>(parent->children[ci - 1]);
      if (left->keys.size() > min_keys_) {
        RedistributeInternal(parent, ci, true);
        return;
      }
    }
    if (ci + 1 < parent->children.size()) {
      auto *right = static_cast<InternalNode *>(parent->children[ci + 1]);
      if (right->keys.size() > min_keys_) {
        RedistributeInternal(parent, ci, false);
        return;
      }
    }
    if (ci > 0) {
      MergeInternal(parent, ci - 1);
    } else {
      MergeInternal(parent, ci);
    }
  }

  // After merge, parent may underflow
  path.pop_back();
  if (parent == root_) {
    MaybeShrinkRoot();
    return;
  }
  if (parent->keys.size() < min_keys_) {
    HandleUnderflow(parent, path);
  }
}

void BPlusTree::RedistributeLeaf(InternalNode *parent, size_t child_idx,
                                 bool from_left) {
  auto *cur = static_cast<LeafNode *>(parent->children[child_idx]);
  if (from_left) {
    auto *left = static_cast<LeafNode *>(parent->children[child_idx - 1]);
    // Borrow left's last key
    cur->keys.insert(cur->keys.begin(), left->keys.back());
    cur->values.insert(cur->values.begin(), left->values.back());
    left->keys.pop_back();
    left->values.pop_back();
    parent->keys[child_idx - 1] = cur->keys.front();
  } else {
    auto *right = static_cast<LeafNode *>(parent->children[child_idx + 1]);
    cur->keys.push_back(right->keys.front());
    cur->values.push_back(right->values.front());
    right->keys.erase(right->keys.begin());
    right->values.erase(right->values.begin());
    parent->keys[child_idx] = right->keys.front();
  }
}

void BPlusTree::MergeLeaf(InternalNode *parent, size_t left_idx) {
  auto *left = static_cast<LeafNode *>(parent->children[left_idx]);
  auto *right = static_cast<LeafNode *>(parent->children[left_idx + 1]);

  left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
  left->values.insert(left->values.end(), right->values.begin(),
                      right->values.end());
  left->next = right->next;

  parent->keys.erase(parent->keys.begin() +
                     static_cast<std::ptrdiff_t>(left_idx));
  parent->children.erase(parent->children.begin() +
                         static_cast<std::ptrdiff_t>(left_idx + 1));
  delete right;
}

void BPlusTree::RedistributeInternal(InternalNode *parent, size_t child_idx,
                                     bool from_left) {
  auto *cur = static_cast<InternalNode *>(parent->children[child_idx]);
  if (from_left) {
    auto *left = static_cast<InternalNode *>(parent->children[child_idx - 1]);
    // Pull separator from parent down to cur front; move left's last key up
    cur->keys.insert(cur->keys.begin(), parent->keys[child_idx - 1]);
    cur->children.insert(cur->children.begin(), left->children.back());
    parent->keys[child_idx - 1] = left->keys.back();
    left->keys.pop_back();
    left->children.pop_back();
  } else {
    auto *right = static_cast<InternalNode *>(parent->children[child_idx + 1]);
    cur->keys.push_back(parent->keys[child_idx]);
    cur->children.push_back(right->children.front());
    parent->keys[child_idx] = right->keys.front();
    right->keys.erase(right->keys.begin());
    right->children.erase(right->children.begin());
  }
}

void BPlusTree::MergeInternal(InternalNode *parent, size_t left_idx) {
  auto *left = static_cast<InternalNode *>(parent->children[left_idx]);
  auto *right = static_cast<InternalNode *>(parent->children[left_idx + 1]);
  // Pull down separator between them
  left->keys.push_back(parent->keys[left_idx]);
  left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
  left->children.insert(left->children.end(), right->children.begin(),
                        right->children.end());

  parent->keys.erase(parent->keys.begin() +
                     static_cast<std::ptrdiff_t>(left_idx));
  parent->children.erase(parent->children.begin() +
                         static_cast<std::ptrdiff_t>(left_idx + 1));
  delete right;
}

auto BPlusTree::Iterator::Key() const -> int { return leaf_->keys[index_]; }
auto BPlusTree::Iterator::Value() const -> int {
  return leaf_->values[index_];
}

auto BPlusTree::Iterator::operator++() -> Iterator & {
  if (leaf_ == nullptr) {
    return *this;
  }
  ++index_;
  if (index_ >= leaf_->keys.size()) {
    leaf_ = leaf_->next;
    index_ = 0;
    if (leaf_ != nullptr && leaf_->keys.empty()) {
      // skip empty (shouldn't happen if invariants hold)
      leaf_ = leaf_->next;
    }
  }
  return *this;
}

auto BPlusTree::Begin() const -> Iterator {
  if (root_ == nullptr) {
    return End();
  }
  Node *cur = root_;
  while (!cur->is_leaf) {
    cur = static_cast<InternalNode *>(cur)->children[0];
  }
  auto *leaf = static_cast<LeafNode *>(cur);
  if (leaf->keys.empty()) {
    return End();
  }
  return Iterator(leaf, 0);
}

auto BPlusTree::Begin(int key) const -> Iterator {
  LeafNode *leaf = FindLeaf(key);
  if (leaf == nullptr) {
    return End();
  }
  while (leaf != nullptr) {
    for (size_t i = 0; i < leaf->keys.size(); ++i) {
      if (leaf->keys[i] >= key) {
        return Iterator(leaf, i);
      }
    }
    leaf = leaf->next;
  }
  return End();
}

auto BPlusTree::LeafSizeOk(const LeafNode *n, bool is_root) const -> bool {
  if (is_root) {
    return n->keys.size() <= max_keys_;
  }
  return n->keys.size() >= min_keys_ && n->keys.size() <= max_keys_;
}

auto BPlusTree::InternalSizeOk(const InternalNode *n, bool is_root) const
    -> bool {
  if (n->children.size() != n->keys.size() + 1) {
    return false;
  }
  if (is_root) {
    return n->keys.size() >= 1 && n->keys.size() <= max_keys_;
  }
  return n->keys.size() >= min_keys_ && n->keys.size() <= max_keys_;
}

auto BPlusTree::CheckNode(const Node *node, int depth, int *leaf_depth,
                          int lower_excl_bound, bool has_lower,
                          int upper_excl_bound, bool has_upper,
                          std::string *err) const -> bool {
  if (node->is_leaf) {
    auto *leaf = static_cast<const LeafNode *>(node);
    if (*leaf_depth < 0) {
      *leaf_depth = depth;
    } else if (*leaf_depth != depth) {
      *err = "leaves at different depths";
      return false;
    }
    if (!LeafSizeOk(leaf, node == root_)) {
      *err = "leaf size out of range";
      return false;
    }
    for (size_t i = 0; i < leaf->keys.size(); ++i) {
      if (i > 0 && !(leaf->keys[i - 1] < leaf->keys[i])) {
        *err = "leaf keys not strictly increasing";
        return false;
      }
      const int k = leaf->keys[i];
      if (has_lower && !(lower_excl_bound <= k)) {
        // lower is inclusive bound from separator: keys >= sep
        *err = "leaf key below lower bound";
        return false;
      }
      if (has_upper && !(k < upper_excl_bound)) {
        *err = "leaf key not below upper bound";
        return false;
      }
    }
    if (leaf->keys.size() != leaf->values.size()) {
      *err = "leaf keys/values size mismatch";
      return false;
    }
    return true;
  }

  auto *in = static_cast<const InternalNode *>(node);
  if (!InternalSizeOk(in, node == root_)) {
    *err = "internal size / children mismatch";
    return false;
  }
  for (size_t i = 1; i < in->keys.size(); ++i) {
    if (!(in->keys[i - 1] < in->keys[i])) {
      *err = "internal keys not strictly increasing";
      return false;
    }
  }
  for (size_t i = 0; i < in->children.size(); ++i) {
    int child_lower = lower_excl_bound;
    bool child_has_lower = has_lower;
    int child_upper = upper_excl_bound;
    bool child_has_upper = has_upper;
    if (i > 0) {
      child_lower = in->keys[i - 1];
      child_has_lower = true;
    }
    if (i < in->keys.size()) {
      child_upper = in->keys[i];
      child_has_upper = true;
    }
    if (!CheckNode(in->children[i], depth + 1, leaf_depth, child_lower,
                   child_has_lower, child_upper, child_has_upper, err)) {
      return false;
    }
  }
  return true;
}

auto BPlusTree::CheckLeafChain() const -> std::string {
  if (root_ == nullptr) {
    return {};
  }
  Node *cur = root_;
  while (!cur->is_leaf) {
    cur = static_cast<InternalNode *>(cur)->children[0];
  }
  auto *leaf = static_cast<LeafNode *>(cur);
  int prev = INT_MIN;
  bool first = true;
  while (leaf != nullptr) {
    for (int k : leaf->keys) {
      if (!first && !(prev < k)) {
        return "leaf chain not strictly increasing";
      }
      prev = k;
      first = false;
    }
    leaf = leaf->next;
  }
  return {};
}

auto BPlusTree::CheckInvariants() const -> std::string {
  if (root_ == nullptr) {
    return {};
  }
  std::string err;
  int leaf_depth = -1;
  if (!CheckNode(root_, 1, &leaf_depth, 0, false, 0, false, &err)) {
    return err;
  }
  err = CheckLeafChain();
  if (!err.empty()) {
    return err;
  }
  return {};
}

auto BPlusTree::DebugString() const -> std::string {
  std::ostringstream os;
  os << "height=" << Height() << " size=" << Size() << " max_keys=" << max_keys_
     << "\n";
  std::function<void(Node *, int)> print = [&](Node *node, int depth) {
    std::string indent(static_cast<size_t>(depth) * 2, ' ');
    if (node->is_leaf) {
      auto *leaf = static_cast<LeafNode *>(node);
      os << indent << "L[";
      for (size_t i = 0; i < leaf->keys.size(); ++i) {
        if (i) {
          os << ",";
        }
        os << leaf->keys[i];
      }
      os << "] next=" << leaf->next << "\n";
    } else {
      auto *in = static_cast<InternalNode *>(node);
      os << indent << "I keys=[";
      for (size_t i = 0; i < in->keys.size(); ++i) {
        if (i) {
          os << ",";
        }
        os << in->keys[i];
      }
      os << "]\n";
      for (Node *c : in->children) {
        print(c, depth + 1);
      }
    }
  };
  if (root_) {
    print(root_, 0);
  }
  return os.str();
}

}  // namespace p8
