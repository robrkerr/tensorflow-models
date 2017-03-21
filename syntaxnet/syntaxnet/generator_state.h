/*

==============================================================================*/

// Generator state for the transition-based sentence generator.

#ifndef SYNTAXNET_GENERATOR_STATE_H_
#define SYNTAXNET_GENERATOR_STATE_H_

#include <string>
#include <vector>

#include "syntaxnet/generator_transitions.h"
#include "syntaxnet/sentence.pb.h"
#include "syntaxnet/utils.h"

namespace syntaxnet {

class TermFrequencyMap;

// A ParserState object represents the state of the parser during the parsing of
// a sentence. The state consists of a pointer to the next input token and a
// stack of partially processed tokens. The parser state can be changed by
// applying a sequence of transitions. Some transitions also add relations
// to the dependency tree of the sentence. The parser state also records the
// (partial) parse tree for the sentence by recording the head of each token and
// the label of this relation. The state is used for both training and parsing.
class GeneratorState {
 public:
  // String representation of the root label.
  static const char kRootLabel[];

  // Default value for the root label in case it's not in the label map.
  static const int kDefaultRootLabel = -1;

  // Initializes the parser state for a sentence, using an additional transition
  // state for preprocessing and/or additional information specific to the
  // transition system. The transition state is allowed to be null, in which
  // case no additional work is performed. The parser state takes ownership of
  // the transition state. A label map is used for transforming between integer
  // and string representations of the labels.
  GeneratorState(Sentence *sentence,
              GeneratorTransitionState *transition_state,
              const TermFrequencyMap *label_map,
              const TermFrequencyMap *tag_map,
              const TermFrequencyMap *word_map);

  // Deletes the parser state.
  ~GeneratorState();

  // Clones the parser state.
  GeneratorState *Clone() const;

  // Returns the root label.
  int RootLabel() const;

  // Returns the number of possible labels.
  int NumLabels() const;

  // Returns the number of tokens in the sentence.
  int NumTokens() const { return num_tokens_; }

  // Returns the token index relative to the next input token. If no such token
  // exists, returns -2.
  int Input(int offset) const;

  // Pushes an element to the stack.
  void Push(int index);

  // Pops the top element from stack and returns it.
  int Pop();

  // Returns the element from the top of the stack.
  int Top() const;

  // Returns the element at a certain position in the stack. Stack(0) is the top
  // stack element. If no such position exists, returns -2.
  int Stack(int position) const;

  // Returns the number of elements on the stack.
  int StackSize() const;

  // Returns true if the stack is empty.
  bool StackEmpty() const;

  // Returns the head index for a given token.
  int Head(int index) const;

  // Returns the label of the relation to head for a given token.
  int Label(int index) const;

  int Tag(int index) const;
  int Word(int index) const;

  // Returns the parent of a given token 'n' levels up in the tree.
  int Parent(int index, int n) const;

  // Returns the leftmost child of a given token 'n' levels down in the tree. If
  // no such child exists, returns -2.
  int LeftmostChild(int index, int n) const;

  // Returns the rightmost child of a given token 'n' levels down in the tree.
  // If no such child exists, returns -2.
  int RightmostChild(int index, int n) const;

  // Returns the n-th left sibling of a given token. If no such sibling exists,
  // returns -2.
  int LeftSibling(int index, int n) const;

  // Returns the n-th right sibling of a given token. If no such sibling exists,
  // returns -2.
  int RightSibling(int index, int n) const;

  bool MissingWord();

  // Adds an arc to the partial dependency tree of the state.
  void AddArc(int index, int head, int label);

  // Get a reference to the underlying token at index. Returns an empty default
  // Token if accessing the root.
  const Token &GetToken(int index) const {
    if (index == -1) return kRootToken;
    return sentence().token(index);
  }

  // Annotates a document with the dependency relations built during parsing for
  // one of its sentences. If rewrite_root_labels is true, then all tokens with
  // no heads will be assigned the default root label "ROOT".
  void CreateDocument(Sentence *document, bool rewrite_root_labels) const;

  // As above, but uses the default of rewrite_root_labels = true.
  void CreateDocument(Sentence *document) const {
    CreateDocument(document, true);
  }

  // Returns the string representation of a dependency label, or an empty string
  // if the label is invalid.
  string LabelAsString(int label) const;

  // Returns the string representation of a dependency label, or an empty string
  // if the label is invalid.
  string TagAsString(int tag) const;

  // Returns the string representation of a dependency label, or an empty string
  // if the label is invalid.
  string WordAsString(int word) const;

  // Returns a string representation of the parser state.
  string ToString() const;

  // Returns the underlying sentence instance.
  const Sentence &sentence() const { return *sentence_; }
  Sentence *mutable_sentence() const { return sentence_; }

  // Returns the transition system-specific state.
  const GeneratorTransitionState *transition_state() const {
    return transition_state_;
  }
  GeneratorTransitionState *mutable_transition_state() {
    return transition_state_;
  }

  // Gets/sets the flag for recording the history of transitions.
  bool keep_history() const { return keep_history_; }
  void set_keep_history(bool keep_history) { keep_history_ = keep_history; }

  // History accessors.
  const std::vector<GeneratorAction> &history() const { return history_; }
  std::vector<GeneratorAction> *mutable_history() { return &history_; }

 private:
  // Empty constructor used for the cloning operation.
  GeneratorState() {}

  // Default value for the root token.
  const Token kRootToken;

  // Sentence to parse. Not owned.
  Sentence *sentence_ = nullptr;

  // Which alternative token analysis is used for tag/category/head/label
  // information. -1 means use default.
  int alternative_ = -1;

  // Transition system-specific state. Owned.
  GeneratorTransitionState *transition_state_ = nullptr;

  // Label map used for conversions between integer and string representations
  // of the dependency labels. Not owned.
  const TermFrequencyMap *label_map_ = nullptr;

  const TermFrequencyMap *tag_map_ = nullptr;
  const TermFrequencyMap *word_map_ = nullptr;

  // Root label.
  int root_label_;

  // Index of the next input token.
  int next_;

  // Parse stack of partially processed tokens.
  std::vector<int> stack_;

  // List of head positions for the (partial) dependency tree.
  std::vector<int> head_;

  // List of dependency relation labels describing the (partial) dependency
  // tree.
  std::vector<int> label_;

  std::vector<int> tag_;
  std::vector<int> word_;

  // Score of the parser state.
  double score_ = 0.0;

  // Transition history.
  bool keep_history_ = false;
  std::vector<GeneratorAction> history_;

  TF_DISALLOW_COPY_AND_ASSIGN(GeneratorState);
};

}  // namespace syntaxnet

#endif  // SYNTAXNET_GENERATOR_STATE_H_
