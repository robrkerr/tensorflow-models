/*

==============================================================================*/

// My custom generator transition system.
//
// This transition system has three types of actions:
//  - The COLLAPSE action removes the first item from the stack and adds it
//    as a child of the next item in the stack.
//  - The ADD action adds a new item to the top of the stack with a dependency
//    relation to item previously on top of the stack (now the second item).
//  - The TAG action assigns a tag to the first item in the stack.
//  - The WORD action assigns a word to the first item in the stack.
//
// The transition system operates with parser actions encoded as integers:
//  - A COLLAPSE action is encoded as 0.
//  - An ADD action is encoded as a number between 1 and 100 (inclusive).
//  - A TAG action is encoded as a number between 101 and 200 (inclusive).
//  - A WORD action is encoded as a number greater than or equal to 201.

#include <string>

#include "syntaxnet/parser_state.h"
#include "syntaxnet/parser_transitions.h"
#include "syntaxnet/utils.h"
#include "tensorflow/core/lib/strings/strcat.h"

namespace syntaxnet {

class GeneratorTransitionState : public ParserTransitionState {
 public:
  // Clones the transition state by returning a new object.
  ParserTransitionState *Clone() const override {
    return new GeneratorTransitionState();
  }

  // Pushes the root on the stack before using the parser state in parsing.
  void Init(ParserState *state) override { state->Push(-1); }

  // Adds transition state specific annotations to the document.
  void AddParseToDocument(const ParserState &state, bool rewrite_root_labels,
                          Sentence *sentence) const override {
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

  // Whether a parsed token should be considered correct for evaluation.
  // TODO: work out if this needs to consider tags, words, or labels...
  bool IsTokenCorrect(const ParserState &state, int index) const override {
    return (state.GoldHead(index) == state.Head(index));
  }

  // Returns a human readable string representation of this state.
  string ToString(const ParserState &state) const override {
    string str;
    str.append("[");
    for (int i = state.StackSize() - 1; i >= 0; --i) {
      const string &word = state.GetToken(state.Stack(i)).word();
      if (i != state.StackSize() - 1) str.append(" ");
      if (word == "") {
        str.append(ParserState::kRootLabel);
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

class GeneratorTransitionSystem : public ParserTransitionSystem {
 public:
  // Action types for the generator transition system.
  enum ParserActionType {
    COLLAPSE = 0,
    ADD = 1,
    TAG = 2,
    WORD = 3,
  };

  // The COLLAPSE action uses the same value as the corresponding action type.
  static ParserAction CollapseAction() { return COLLAPSE; }

  // The ADD action converts the label to a number between 1 and 100.
  static ParserAction AddAction(int label) { return 1 + label; }

  // The TAG action converts the tag to a number between 101 and 200.
  static ParserAction TagAction(int tag) { return 101 + tag; }

  // The WORD action converts the word to a number greater than 200.
  static ParserAction WordAction(int word) { return 201 + word; }

  // Extracts the action type from a given parser action.
  static ParserActionType ActionType(ParserAction action) {
    if (action == 0) {
      return COLLAPSE;
    } else if (action <= 100) {
      return ADD;
    } else if (action <= 200) {
      return TAG;
    } else {
      return WORD;
    }
  }

  // Extracts the label from the ADD parser action (returns -1 for others).
  static int Label(ParserAction action) {
    return (action > 0) && (action <= 100) ? (action - 1) : -1;
  }

  // Extracts the tag from the TAG parser action (returns -1 for others).
  static int Tag(ParserAction action) {
    return (action > 100) && (action <= 200) ? (action - 101) : -1;
  }

  // Extracts the word from the WORD parser action (returns -1 for others).
  static int Word(ParserAction action) {
    return (action > 200) ? (action - 201) : -1;
  }

  // Returns the number of action types.
  int NumActionTypes() const override { return 4; }

  // Returns the number of possible actions.
  int NumActions(int num_labels) const override { return 1 + 2 * num_labels; }

  // The method returns the default action for a given state.
  ParserAction GetDefaultAction(const ParserState &state) const override {
    // If there are further tokens available in the input then Shift.
    if (!state.EndOfInput()) return ShiftAction();

    // Do a "reduce".
    return RightArcAction(2);
  }

  // Returns the next gold action for a given state according to the
  // underlying annotated sentence.
  ParserAction GetNextGoldAction(const ParserState &state) const override {
    // If the stack contains less than 2 tokens, the only valid parser action is
    // shift.
    if (state.StackSize() < 2) {
      DCHECK(!state.EndOfInput());
      return ShiftAction();
    }

    // If the second token on the stack is the head of the first one,
    // return a right arc action.
    if (state.GoldHead(state.Stack(0)) == state.Stack(1) &&
        DoneChildrenRightOf(state, state.Stack(0))) {
      const int gold_label = state.GoldLabel(state.Stack(0));
      return RightArcAction(gold_label);
    }

    // If the first token on the stack is the head of the second one,
    // return a left arc action.
    if (state.GoldHead(state.Stack(1)) == state.Top()) {
      const int gold_label = state.GoldLabel(state.Stack(1));
      return LeftArcAction(gold_label);
    }

    // Otherwise, shift.
    return ShiftAction();
  }

  // Determines if a token has any children to the right in the sentence.
  // Arc standard is a bottom-up parsing method and has to finish all sub-trees
  // first.
  static bool DoneChildrenRightOf(const ParserState &state, int head) {
    int index = state.Next();
    int num_tokens = state.sentence().token_size();
    while (index < num_tokens) {
      // Check if the token at index is the child of head.
      int actual_head = state.GoldHead(index);
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
  bool IsAllowedAction(ParserAction action,
                       const ParserState &state) const override {
    switch (ActionType(action)) {
      case SHIFT:
        return IsAllowedShift(state);
      case LEFT_ARC:
        return IsAllowedLeftArc(state);
      case RIGHT_ARC:
        return IsAllowedRightArc(state);
    }

    return false;
  }

  // Returns true if a shift is allowed in the given parser state.
  bool IsAllowedShift(const ParserState &state) const {
    // We can shift if there are more input tokens.
    return !state.EndOfInput();
  }

  // Returns true if a left-arc is allowed in the given parser state.
  bool IsAllowedLeftArc(const ParserState &state) const {
    // Left-arc requires two or more tokens on the stack but the first token
    // is the root an we do not want and left arc to the root.
    return state.StackSize() > 2;
  }

  // Returns true if a right-arc is allowed in the given parser state.
  bool IsAllowedRightArc(const ParserState &state) const {
    // Right arc requires three or more tokens on the stack.
    return state.StackSize() > 1;
  }

  // Performs the specified action on a given parser state, without adding the
  // action to the state's history.
  void PerformActionWithoutHistory(ParserAction action,
                                   ParserState *state) const override {
    switch (ActionType(action)) {
      case SHIFT:
        PerformShift(state);
        break;
      case LEFT_ARC:
        PerformLeftArc(state, Label(action));
        break;
      case RIGHT_ARC:
        PerformRightArc(state, Label(action));
        break;
    }
  }

  // Makes a shift by pushing the next input token on the stack and moving to
  // the next position.
  void PerformShift(ParserState *state) const {
    DCHECK(IsAllowedShift(*state));
    state->Push(state->Next());
    state->Advance();
  }

  // Makes a left-arc between the two top tokens on stack and pops the second
  // token on stack.
  void PerformLeftArc(ParserState *state, int label) const {
    DCHECK(IsAllowedLeftArc(*state));
    int s0 = state->Pop();
    state->AddArc(state->Pop(), s0, label);
    state->Push(s0);
  }

  // Makes a right-arc between the two top tokens on stack and pops the stack.
  void PerformRightArc(ParserState *state, int label) const {
    DCHECK(IsAllowedRightArc(*state));
    int s0 = state->Pop();
    int s1 = state->Pop();
    state->AddArc(s0, s1, label);
    state->Push(s1);
  }

  // We are in a deterministic state when we either reached the end of the input
  // or reduced everything from the stack.
  bool IsDeterministicState(const ParserState &state) const override {
    return state.StackSize() < 2 && !state.EndOfInput();
  }

  // We are in a final state when we reached the end of the input and the stack
  // is empty.
  bool IsFinalState(const ParserState &state) const override {
    return state.EndOfInput() && state.StackSize() < 2;
  }

  // Returns a string representation of a parser action.
  string ActionAsString(ParserAction action,
                        const ParserState &state) const override {
    switch (ActionType(action)) {
      case SHIFT:
        return "SHIFT";
      case LEFT_ARC:
        return "LEFT_ARC(" + state.LabelAsString(Label(action)) + ")";
      case RIGHT_ARC:
        return "RIGHT_ARC(" + state.LabelAsString(Label(action)) + ")";
    }
    return "UNKNOWN";
  }

  // Returns a new transition state to be used to enhance the parser state.
  ParserTransitionState *NewTransitionState(bool training_mode) const override {
    return new ArcStandardTransitionState();
  }

  // Meta information API. Returns token indices to link parser actions back
  // to positions in the input sentence.
  bool SupportsActionMetaData() const override { return true; }

  // Returns the child of a new arc for reduce actions.
  int ChildIndex(const ParserState &state,
                 const ParserAction &action) const override {
    switch (ActionType(action)) {
      case SHIFT:
        return -1;
      case LEFT_ARC:  // left arc pops stack(1)
        return state.Stack(1);
      case RIGHT_ARC:
        return state.Stack(0);
      default:
        LOG(FATAL) << "Invalid parser action: " << action;
    }
  }

  // Returns the parent of a new arc for reduce actions.
  int ParentIndex(const ParserState &state,
                  const ParserAction &action) const override {
    switch (ActionType(action)) {
      case SHIFT:
        return -1;
      case LEFT_ARC:  // left arc pops stack(1)
        return state.Stack(0);
      case RIGHT_ARC:
        return state.Stack(1);
      default:
        LOG(FATAL) << "Invalid parser action: " << action;
    }
  }

  // Input for the tag map. Not owned.
  TaskInput *input_tag_map_ = nullptr;

  // Tag map used for conversions between integer and string representations
  // part of speech tags. Owned through SharedStore.
  const TermFrequencyMap *tag_map_ = nullptr;

  // Input for the tag to category map. Not owned.
  TaskInput *input_tag_to_category_ = nullptr;

  // Tag to category map. Owned through SharedStore.
  const TagToCategoryMap *tag_to_category_ = nullptr;

  bool join_category_to_pos_ = false;
};

REGISTER_TRANSITION_SYSTEM("generator", GeneratorTransitionSystem);

}  // namespace syntaxnet
