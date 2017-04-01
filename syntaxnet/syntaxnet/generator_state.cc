/* Copyright 2016 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "syntaxnet/generator_state.h"

#include "syntaxnet/sentence.pb.h"
#include "syntaxnet/term_frequency_map.h"
#include "syntaxnet/utils.h"

namespace syntaxnet {

const char GeneratorState::kRootLabel[] = "ROOT";

GeneratorState::GeneratorState(Sentence *sentence,
                         GeneratorTransitionState *transition_state,
                         const TermFrequencyMap *label_map,
                         const TermFrequencyMap *tag_map,
                         const TermFrequencyMap *word_map)
    : sentence_(sentence),
      transition_state_(transition_state),
      label_map_(label_map),
      tag_map_(tag_map),
      word_map_(word_map),
      root_label_(label_map->LookupIndex(kRootLabel,
                                         kDefaultRootLabel  /* unknown */)),
      next_(0) {

  // Transition system-specific preprocessing.
  if (transition_state_ != nullptr) transition_state_->Init(this);
}

GeneratorState::~GeneratorState() { delete transition_state_; }

GeneratorState *GeneratorState::Clone() const {
  GeneratorState *new_state = new GeneratorState(
    sentence_,
    (transition_state_ == nullptr ? nullptr : transition_state_->Clone()),
    label_map_,
    tag_map_,
    word_map_
  );
  new_state->root_label_ = root_label_;
  new_state->alternative_ = alternative_;
  new_state->next_ = next_;
  new_state->stack_.assign(stack_.begin(), stack_.end());
  new_state->head_.assign(head_.begin(), head_.end());
  new_state->label_.assign(label_.begin(), label_.end());
  new_state->tag_.assign(tag_.begin(), tag_.end());
  new_state->word_.assign(word_.begin(), word_.end());
  new_state->score_ = score_;
  new_state->keep_history_ = keep_history_;
  new_state->history_.assign(history_.begin(), history_.end());
  return new_state;
}

int GeneratorState::RootLabel() const { return root_label_; }

int GeneratorState::NumLabels() const {
  return label_map_->Size() + (RootLabel() == kDefaultRootLabel ? 1 : 0);
}

int GeneratorState::Next() const {
  CHECK_GE(next_, -1);
  return next_;
}

void GeneratorState::Advance() {
  ++next_;
}

void GeneratorState::Advance(int next) {
  next_ = next;
}

void GeneratorState::Push(int index) {
  stack_.push_back(index);
}

int GeneratorState::Pop() {
  CHECK(!StackEmpty()) << utils::Join(history_, ",");
  const int result = stack_.back();
  stack_.pop_back();
  return result;
}

int GeneratorState::Top() const {
  CHECK(!StackEmpty()) << utils::Join(history_, ",");
  return stack_.back();
}

int GeneratorState::Stack(int position) const {
  if (position < 0) return -2;
  const int index = stack_.size() - 1 - position;
  return (index < 0) ? -2 : stack_[index];
}

int GeneratorState::StackSize() const { return stack_.size(); }

bool GeneratorState::StackEmpty() const { return stack_.empty(); }

int GeneratorState::Head(int index) const {
  CHECK_GE(index, -1);
  CHECK_LT(index, head_.size());
  return index == -1 ? -1 : head_[index];
}

int GeneratorState::Label(int index) const {
  CHECK_GE(index, -1);
  CHECK_LT(index, label_.size());
  return index == -1 ? RootLabel() : label_[index];
}

int GeneratorState::Parent(int index, int n) const {
  // Find the n-th parent by applying the head function n times.
  CHECK_GE(index, -1);
  while (n-- > 0) index = Head(index);
  return index;
}

int GeneratorState::LeftmostChild(int index, int n) const {
  CHECK_GE(index, -1);
  while (n-- > 0) {
    // Find the leftmost child by scanning from start until a child is
    // encountered.
    int i;
    for (i = -1; i < index; ++i) {
      if (Head(i) == index) break;
    }
    if (i == index) return -2;
    index = i;
  }
  return index;
}

int GeneratorState::RightmostChild(int index, int n) const {
  CHECK_GE(index, -1);
  while (n-- > 0) {
    // Find the rightmost child by scanning backward from end until a child
    // is encountered.
    int i;
    for (i = (int)(head_.size() - 1); i > index; --i) {
      if (Head(i) == index) break;
    }
    if (i == index) return -2;
    index = i;
  }
  return index;
}

int GeneratorState::LeftSibling(int index, int n) const {
  // Find the n-th left sibling by scanning left until the n-th child of the
  // parent is encountered.
  CHECK_GE(index, -1);
  if (index == -1 && n > 0) return -2;
  int i = index;
  while (n > 0) {
    --i;
    if (i == -1) return -2;
    if (Head(i) == Head(index)) --n;
  }
  return i;
}

int GeneratorState::RightSibling(int index, int n) const {
  // Find the n-th right sibling by scanning right until the n-th child of the
  // parent is encountered.
  CHECK_GE(index, -1);
  if (index == -1 && n > 0) return -2;
  int i = index;
  while (n > 0) {
    ++i;
    if (i == (int)(head_.size())) return -2;
    if (Head(i) == Head(index)) --n;
  }
  return i;
}

bool GeneratorState::MissingWord() const {
  return head_.size() > word_.size();
}

void GeneratorState::Add(int label, int tag) {
  CHECK(StackSize() > 1);
  int s0 = stack_.back();
  stack_.push_back(next_);
  head_.push_back(s0);
  label_.push_back(label);
  tag_.push_back(tag);
  ++next_;
}

void GeneratorState::AddWord(int word) {
  CHECK(StackSize() > 1);
  word_.push_back(word);
}

void GeneratorState::CreateDocument(Sentence *sentence,
                                     bool rewrite_root_labels) const {
  transition_state_->CreateDocument(*this, rewrite_root_labels, sentence);
}

string GeneratorState::LabelAsString(int label) const {
  if (label == root_label_) return kRootLabel;
  if (label >= 0 && label < label_map_->Size()) {
    return label_map_->GetTerm(label);
  }
  return "";
}

string GeneratorState::TagAsString(int tag) const {
  if (tag >= 0 && tag < tag_map_->Size()) {
    return tag_map_->GetTerm(tag);
  }
  return "";
}

string GeneratorState::WordAsString(int word) const {
  if (word >= 0 && word < word_map_->Size()) {
    return word_map_->GetTerm(word);
  }
  return "";
}

string GeneratorState::ToString() const {
  return transition_state_->ToString(*this);
}

}  // namespace syntaxnet
