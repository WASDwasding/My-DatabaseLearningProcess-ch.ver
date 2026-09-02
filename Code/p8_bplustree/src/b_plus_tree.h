#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace p8 {

/**
 * Minimal in-memory B+Tree (P8 stage: single-thread, no BPM).
 *
 * - key/value are int for teaching clarity
 * - One logical "node" ~ one page
 * - Leaf: (key, value) + next sibling pointer (leaf chain)
 * - Internal: separator keys + children; children.size() == keys.size() + 1
 * - Separator rule: first i with search_key < keys[i] -> children[i]; else last
 *
 * Invariants checked by CheckInvariants / CheckLeafChain.
 */
class BPlusTree {
  struct Node;
  struct LeafNode;
  struct InternalNode;

 public:
  explicit BPlusTree(size_t max_keys = 3);
  ~BPlusTree();

  BPlusTree(const BPlusTree &) = delete;
  auto operator=(const BPlusTree &) -> BPlusTree & = delete;

  auto Insert(int key, int value) -> bool;
  auto Get(int key) const -> std::optional<int>;
  auto Remove(int key) -> bool;

  auto Empty() const -> bool { return root_ == nullptr || Size() == 0; }
  auto Size() const -> size_t;
  auto Height() const -> int;
  auto MaxKeys() const -> size_t { return max_keys_; }
  auto MinKeys() const -> size_t { return min_keys_; }

  auto CheckInvariants() const -> std::string;
  auto CheckLeafChain() const -> std::string;
  auto DebugString() const -> std::string;

  class Iterator {
   public:
    auto IsEnd() const -> bool { return leaf_ == nullptr; }
    auto Key() const -> int;
    auto Value() const -> int;
    auto operator++() -> Iterator &;
    auto operator==(const Iterator &o) const -> bool {
      return leaf_ == o.leaf_ && index_ == o.index_;
    }
    auto operator!=(const Iterator &o) const -> bool { return !(*this == o); }

   private:
    friend class BPlusTree;
    Iterator(LeafNode *leaf, size_t index) : leaf_(leaf), index_(index) {}
    LeafNode *leaf_{nullptr};
    size_t index_{0};
  };

  auto Begin() const -> Iterator;
  auto Begin(int key) const -> Iterator;
  auto End() const -> Iterator { return Iterator(nullptr, 0); }

 private:
  struct Node {
    bool is_leaf{false};
    virtual ~Node() = default;
  };

  struct LeafNode : Node {
    std::vector<int> keys;
    std::vector<int> values;
    LeafNode *next{nullptr};
    LeafNode() { is_leaf = true; }
  };

  struct InternalNode : Node {
    std::vector<int> keys;
    std::vector<Node *> children;
    InternalNode() { is_leaf = false; }
  };

  auto FindLeaf(int key) const -> LeafNode *;
  auto FindLeafPath(int key, std::vector<InternalNode *> *path) -> LeafNode *;
  static auto ChildIndex(const InternalNode *node, int key) -> size_t;

  void InsertIntoLeaf(LeafNode *leaf, int key, int value,
                      std::vector<InternalNode *> &path);
  void SplitLeaf(LeafNode *leaf, std::vector<InternalNode *> &path);
  void InsertIntoParent(InternalNode *left_parent_hint, Node *left, int key,
                        Node *right, std::vector<InternalNode *> &path,
                        size_t path_index);
  void SplitInternal(InternalNode *node, std::vector<InternalNode *> &path,
                     size_t path_index);

  void RemoveFromLeaf(LeafNode *leaf, size_t idx,
                      std::vector<InternalNode *> &path);
  void HandleUnderflow(Node *node, std::vector<InternalNode *> &path);
  void RedistributeLeaf(InternalNode *parent, size_t child_idx, bool from_left);
  void MergeLeaf(InternalNode *parent, size_t left_idx);
  void RedistributeInternal(InternalNode *parent, size_t child_idx,
                            bool from_left);
  void MergeInternal(InternalNode *parent, size_t left_idx);
  void UpdateParentKey(InternalNode *parent, size_t child_idx, int new_key);
  void MaybeShrinkRoot();

  void Destroy(Node *node);
  auto LeafSizeOk(const LeafNode *n, bool is_root) const -> bool;
  auto InternalSizeOk(const InternalNode *n, bool is_root) const -> bool;
  auto CheckNode(const Node *node, int depth, int *leaf_depth,
                 int lower_bound, bool has_lower, int upper_bound,
                 bool has_upper, std::string *err) const -> bool;

  size_t max_keys_;
  size_t min_keys_;
  Node *root_{nullptr};
};

}  // namespace p8
