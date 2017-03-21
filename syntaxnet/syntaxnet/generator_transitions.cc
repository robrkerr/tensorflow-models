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

class GeneratorTransitionState {
 public:
  explicit GeneratorTransitionState(const TaggerTransitionState *state) {
   tag_ = state->tag_;
   // not sure what this is...
  }

  // Clones the transition state by returning a new object.
  GeneratorTransitionState *Clone() const {
   return new GeneratorTransitionState(this);
  }

  // Pushes the root on the stack before using the parser state in parsing.
  void Init(GeneratorState *state) { state->Push(-1); }

  // Adds transition state specific annotations to the document.
  void CreateDocument(const GeneratorState &state, bool rewrite_root_labels, Sentence *sentence) const {
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
  string ToString(const GeneratorState &state) const {
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
};

class GeneratorTransitionSystem {
 public:
  // Action types for the generator transition system.
  enum GeneratorActionType {
    COLLAPSE = 0,
    ADD = 1,
    WORD = 2,
  };

  explicit GeneratorTransitionSystem(const int num_labels, const int num_tags, const int num_words)
     : num_labels(num_labels), num_tags(num_tags), num_words(num_words) {}

  // The COLLAPSE action uses the same value as the corresponding action type.
  static GeneratorAction CollapseAction() { return COLLAPSE; }

  // The ADD action converts the label to a number between 1 and 100.
  static GeneratorAction AddAction(int label, int tag) {
    return 1 + label + num_labels * tag;
  }

  // The WORD action converts the word to a number greater than 200.
  static GeneratorAction WordAction(int word) {
    return 1 + num_labels * num_tags + word;
  }

  // Extracts the action type from a given parser action.
  static GeneratorActionType ActionType(GeneratorAction action) {
    if (action == 0) {
      return COLLAPSE;
    } else if (action <= num_labels * num_tags) {
      return ADD;
    } else {
      return WORD;
    }
  }

  // Extracts the label from the ADD parser action (returns -1 for others).
  static int Label(GeneratorAction action) {
    if ((action > 0) && (action <= num_labels * num_tags)) {
      return (action - 1) % num_labels;
    } else {
      return -1;
    }
  }

  // Extracts the tag from the TAG parser action (returns -1 for others).
  static int Tag(GeneratorAction action) {
    if ((action > 0) && (action <= num_labels * num_tags)) {
      return (action - 1) / num_labels;
    } else {
      return -1;
    }
  }

  // Extracts the word from the WORD parser action (returns -1 for others).
  static int Word(GeneratorAction action) {
    if (action > num_labels * num_tags) {
      return action - num_labels * num_tags - 1;
    } else {
      return -1;
    }
  }

  // Returns the number of action types.
  int NumActionTypes() const { return 3; }

  // Returns the number of possible actions.
  int NumActions() const { return 1 + num_labels * num_tags + num_words; }

  // The method returns the default action for a given state - why is this here??
  GeneratorAction GetDefaultAction(const GeneratorState &state) const {
    return CollapseAction()
  }

  // Determines if a token has any children to the right in the sentence.
  // Arc standard is a bottom-up parsing method and has to finish all sub-trees
  // first.
  static bool DoneChildrenRightOf(const GeneratorState &state, int head) {
    int index = state.Next();
    int num_tokens = state.sentence().token_size();
    while (index < num_tokens) {
      // Check if the token at index is the child of head.
      int actual_head = state.GoldHead(index); /// TODO: why is the gold head used here??
      if (actual_head == head) return false;

      // If the head of the token at index is to the right of it there cannot be
      // any children in-between, so we can skip forward to the head.  Note this
      // is only true for projective trees.
      if (actual_head > index) {
        index = actual_head;
      } else {
        ++index;
      }
    }
    return true;
  }

  // Checks if the action is allowed in a given parser state.
  bool IsAllowedAction(GeneratorAction action, const GeneratorState &state) const {
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
  bool IsAllowedCollapse(const GeneratorState &state) const {
    return !state->MissingWord() && state.StackSize() > 2;
  }

  // Returns true if a left-arc is allowed in the given parser state.
  bool IsAllowedAdd(const GeneratorState &state) const {
    return !state->MissingWord();
  }

  // Returns true if a right-arc is allowed in the given parser state.
  bool IsAllowedWord(const GeneratorState &state) const {
    return state->MissingWord();
  }

  // Performs the specified action on a given parser state, without adding the
  // action to the state's history.
  void PerformActionWithoutHistory(GeneratorAction action, GeneratorState *state) const {
    switch (ActionType(action)) {
      case COLLAPSE:
        PerformCollapse(state);
        break;
      case ADD:
        PerformAdd(state, Label(action), Tag(Action));
        break;
      case WORD:
        PerformWord(state, Word(action));
        break;
    }
  }

  // Makes a shift by pushing the next input token on the stack and moving to
  // the next position.
  void PerformCollapse(GeneratorState *state) const {
    DCHECK(IsAllowedCollapse(*state));
    state->Pop();
  }

  // Makes a left-arc between the two top tokens on stack and pops the second
  // token on stack.
  void PerformAdd(GeneratorState *state, int label, int tag) const {
    DCHECK(IsAllowedAdd(*state));
    state->Add(label, tag);
  }

  // Makes a right-arc between the two top tokens on stack and pops the stack.
  void PerformWord(GeneratorState *state, int word) const {
    DCHECK(IsAllowedWord(*state));
    state->AddWord(word);
  }

  // We are in a deterministic state when we either reached the end of the input
  // or reduced everything from the stack.
  bool IsDeterministicState(const GeneratorState &state) const {
    return state.StackSize() < 2;
  }

  // We are in a final state when we reached the end of the input and the stack
  // is empty.
  bool IsFinalState(const GeneratorState &state) const {
    return state.StackSize() < 2;
  }

  // Returns a string representation of a parser action.
  string ActionAsString(GeneratorAction action, const GeneratorState &state) const {
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
  GeneratorTransitionState *NewTransitionState(bool training_mode) const {
    return new GeneratorTransitionState();
  }

  // Meta information API. Returns token indices to link parser actions back
  // to positions in the input sentence.
  bool SupportsActionMetaData() const { return true; }

  // Returns the child of a new arc for reduce actions.
  int ChildIndex(const GeneratorState &state, const GeneratorAction &action) const {
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
  int ParentIndex(const GeneratorState &state, const GeneratorAction &action) const {
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
};

REGISTER_TRANSITION_SYSTEM("generator", GeneratorTransitionSystem);

}  // namespace syntaxnet
