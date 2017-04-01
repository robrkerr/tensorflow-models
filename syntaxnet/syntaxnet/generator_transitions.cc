/*

==============================================================================*/

// My custom generator transition system.
//
// This transition system has three types of actions:
//  - The COLLAPSE action removes the first item from the stack and adds it
//    as a child of the next item in the stack.
//  - The ADD action adds a new item to the top of the stack with a dependency
//    relation to item previously on top of the stack (now the second item)
//    and a tag.
//  - The WORD action assigns a word to the first item in the stack.
//
// The transition system operates with parser actions encoded as integers:
//  - A COLLAPSE action is encoded as 0.
//  - An ADD action is encoded as a number between 1 and n (inclusive), where
//    n = num_labels * num_tags.
//  - A WORD action is encoded as a number greater than or equal to 201.

#include <string>

#include "syntaxnet/generator_state.h"
#include "syntaxnet/utils.h"
#include "tensorflow/core/lib/strings/strcat.h"

namespace syntaxnet {

// Clones the transition state by returning a new object.
GeneratorTransitionState *GeneratorTransitionState::Clone() const {
 return new GeneratorTransitionState(this);
}

// Pushes the root on the stack before using the parser state in parsing.
void GeneratorTransitionState::Init(GeneratorState *state) {
  state->Push(-1);
}

// Adds transition state specific annotations to the document.
void GeneratorTransitionState::CreateDocument(const GeneratorState &state, bool rewrite_root_labels, Sentence *sentence) const {
  for (int i = 0; i < state.NumTokens(); ++i) {
    Token *token = sentence->add_token();
    token->set_label(state.LabelAsString(state.Label(i)));
    token->set_tag(state.TagAsString(state.Tag(i)));
    token->set_word(state.WordAsString(state.Word(i)));
    if (state.Head(i) != -1) {
      token->set_head(state.Head(i));
    } else {
      token->clear_head();
      if (rewrite_root_labels) {
        token->set_label(state.LabelAsString(state.RootLabel()));
      }
    }
  }
}

// Returns a human readable string representation of this state.
string GeneratorTransitionState::ToString(const GeneratorState &state) const {
  string str;
  str.append("[");
  for (int i = state.StackSize() - 1; i >= 0; --i) {
    const string &word = state.GetToken(state.Stack(i)).word();
    if (i != state.StackSize() - 1) str.append(" ");
    if (word == "") {
      str.append(GeneratorState::kRootLabel);
    } else {
      str.append(word);
    }
  }
  str.append("]");
  for (int i = state.Next(); i < state.NumTokens(); ++i) {
    tensorflow::strings::StrAppend(&str, " ", state.GetToken(i).word());
  }
  return str;
}

GeneratorTransitionSystem::~GeneratorTransitionSystem() {
  SharedStore::Release(label_map_);
  SharedStore::Release(tag_map_);
  SharedStore::Release(word_map_);
}

// Determines label, tag, and word map locations.
void GeneratorTransitionSystem::Setup(TaskContext *context) {
  input_label_map_ = context->GetInput("label-map", "text", "");
  input_tag_map_ = context->GetInput("tag-map", "text", "");
  input_word_map_ = context->GetInput("word-map", "text", "");
}

// Pushes the root on the stack before using the parser state in parsing.
void GeneratorTransitionSystem::Init(TaskContext *context) {
  const string label_map_path = TaskContext::InputFile(*input_label_map_);
  const string tag_map_path = TaskContext::InputFile(*input_tag_map_);
  const string word_map_path = TaskContext::InputFile(*input_word_map_);
  label_map_ = SharedStoreUtils::GetWithDefaultName<TermFrequencyMap>(label_map_path, 0, 0);
  tag_map_ = SharedStoreUtils::GetWithDefaultName<TermFrequencyMap>(tag_map_path, 0, 0);
  word_map_ = SharedStoreUtils::GetWithDefaultName<TermFrequencyMap>(word_map_path, 0, 0);
}

// The COLLAPSE action uses the same value as the corresponding action type.
GeneratorAction GeneratorTransitionSystem::CollapseAction() { return COLLAPSE; }

// The ADD action converts the label to a number between 1 and 100.
GeneratorAction GeneratorTransitionSystem::AddAction(int label, int tag) const {
  return 1 + label + label_map_->Size() * tag;
}

// The WORD action converts the word to a number greater than 200.
GeneratorAction GeneratorTransitionSystem::WordAction(int word) const {
  return 1 + label_map_->Size() * tag_map_->Size() + word;
}

// Extracts the action type from a given parser action.
GeneratorTransitionSystem::GeneratorActionType GeneratorTransitionSystem::ActionType(GeneratorAction action) const {
  if (action == 0) {
    return COLLAPSE;
  } else if (action <= label_map_->Size() * tag_map_->Size()) {
    return ADD;
  } else {
    return WORD;
  }
}

// Extracts the label from the ADD parser action (returns -1 for others).
int GeneratorTransitionSystem::Label(GeneratorAction action) const {
  if ((action > 0) && (action <= label_map_->Size() * tag_map_->Size())) {
    return (action - 1) % label_map_->Size();
  } else {
    return -1;
  }
}

// Extracts the tag from the TAG parser action (returns -1 for others).
int GeneratorTransitionSystem::Tag(GeneratorAction action) const {
  if ((action > 0) && (action <= label_map_->Size() * tag_map_->Size())) {
    return (action - 1) / label_map_->Size();
  } else {
    return -1;
  }
}

// Extracts the word from the WORD parser action (returns -1 for others).
int GeneratorTransitionSystem::Word(GeneratorAction action) const {
  if (action > label_map_->Size() * tag_map_->Size()) {
    return action - label_map_->Size() * tag_map_->Size() - 1;
  } else {
    return -1;
  }
}

// Returns the number of action types.
int GeneratorTransitionSystem::NumActionTypes() const { return 3; }

// Returns the number of possible actions.
int GeneratorTransitionSystem::NumActions() const {
  return 1 + label_map_->Size() * tag_map_->Size() + word_map_->Size();
}

// The method returns the default action for a given state - why is this here??
GeneratorAction GeneratorTransitionSystem::GetDefaultAction(const GeneratorState &state) const {
  return CollapseAction();
}

// Checks if the action is allowed in a given parser state.
bool GeneratorTransitionSystem::IsAllowedAction(GeneratorAction action, const GeneratorState &state) const {
  switch (ActionType(action)) {
    case COLLAPSE:
      return IsAllowedCollapse(state);
    case ADD:
      return IsAllowedAdd(state);
    case WORD:
      return IsAllowedWord(state);
  }
  return false;
}

// Returns true if a shift is allowed in the given parser state.
bool GeneratorTransitionSystem::IsAllowedCollapse(const GeneratorState &state) const {
  return !(state.MissingWord()) && state.StackSize() > 2;
}

// Returns true if a left-arc is allowed in the given parser state.
bool GeneratorTransitionSystem::IsAllowedAdd(const GeneratorState &state) const {
  return !(state.MissingWord());
}

// Returns true if a right-arc is allowed in the given parser state.
bool GeneratorTransitionSystem::IsAllowedWord(const GeneratorState &state) const {
  return state.MissingWord();
}

// Performs the specified action on a given parser state, without adding the
// action to the state's history.
void GeneratorTransitionSystem::PerformActionWithoutHistory(GeneratorAction action, GeneratorState *state) const {
  switch (ActionType(action)) {
    case COLLAPSE:
      PerformCollapse(state);
      break;
    case ADD:
      PerformAdd(state, Label(action), Tag(action));
      break;
    case WORD:
      PerformWord(state, Word(action));
      break;
  }
}

// Makes a shift by pushing the next input token on the stack and moving to
// the next position.
void GeneratorTransitionSystem::PerformCollapse(GeneratorState *state) const {
  DCHECK(IsAllowedCollapse(*state));
  state->Pop();
}

// Makes a left-arc between the two top tokens on stack and pops the second
// token on stack.
void GeneratorTransitionSystem::PerformAdd(GeneratorState *state, int label, int tag) const {
  DCHECK(IsAllowedAdd(*state));
  state->Add(label, tag);
}

// Makes a right-arc between the two top tokens on stack and pops the stack.
void GeneratorTransitionSystem::PerformWord(GeneratorState *state, int word) const {
  DCHECK(IsAllowedWord(*state));
  state->AddWord(word);
}

// We are in a deterministic state when we either reached the end of the input
// or reduced everything from the stack.
bool GeneratorTransitionSystem::IsDeterministicState(const GeneratorState &state) const {
  return state.StackSize() < 2;
}

// We are in a final state when we reached the end of the input and the stack
// is empty.
bool GeneratorTransitionSystem::IsFinalState(const GeneratorState &state) const {
  return state.StackSize() < 2;
}

// Returns a string representation of a parser action.
string GeneratorTransitionSystem::ActionAsString(GeneratorAction action, const GeneratorState &state) const {
  switch (ActionType(action)) {
    case COLLAPSE:
      return "COLLAPSE";
    case ADD:
      return "ADD(" + state.LabelAsString(Label(action)) + ", " + state.TagAsString(Tag(action)) + ")";
    case WORD:
      return "WORD(" + state.WordAsString(Word(action)) + ")";
  }
  return "UNKNOWN";
}

// Returns a new transition state to be used to enhance the parser state.
GeneratorTransitionState * GeneratorTransitionSystem::NewTransitionState(bool training_mode) const {
  return new GeneratorTransitionState();
  // GeneratorTransitionState(this);
}

// Meta information API. Returns token indices to link parser actions back
// to positions in the input sentence.
bool GeneratorTransitionSystem::SupportsActionMetaData() const { return true; }

// Returns the child of a new arc for reduce actions.
int GeneratorTransitionSystem::ChildIndex(const GeneratorState &state, const GeneratorAction &action) const {
  switch (ActionType(action)) {
    case COLLAPSE:
      return -1;
    case ADD:
      return state.Stack(0);
    case WORD:
      return -1;
    default:
      LOG(FATAL) << "Invalid generator action: " << action;
  }
}

// Returns the parent of a new arc for reduce actions.
int GeneratorTransitionSystem::ParentIndex(const GeneratorState &state, const GeneratorAction &action) const {
  switch (ActionType(action)) {
    case COLLAPSE:
      return -1;
    case ADD:
      return state.Stack(1);
    case WORD:
      return -1;
    default:
      LOG(FATAL) << "Invalid generator action: " << action;
  }
}

REGISTER_TRANSITION_SYSTEM("generator", GeneratorTransitionSystem);

}  // namespace syntaxnet
