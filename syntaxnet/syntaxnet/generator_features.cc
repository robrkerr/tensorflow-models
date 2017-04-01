
#include "syntaxnet/generator_features.h"

#include <string>

#include "syntaxnet/registry.h"
#include "syntaxnet/sentence_features.h"
#include "syntaxnet/workspace.h"

namespace syntaxnet {

// Registry for the parser feature functions.
REGISTER_SYNTAXNET_CLASS_REGISTRY("generator feature function",
                                  GeneratorFeatureFunction);

// Registry for the parser state + token index feature functions.
REGISTER_SYNTAXNET_CLASS_REGISTRY("generator+index feature function",
                                  GeneratorIndexFeatureFunction);

RootFeatureType::RootFeatureType(const string &name,
                                 const FeatureType &wrapped_type,
                                 int root_value)
    : FeatureType(name), wrapped_type_(wrapped_type), root_value_(root_value) {}

string RootFeatureType::GetFeatureValueName(FeatureValue value) const {
  if (value == root_value_) return "<ROOT>";
  return wrapped_type_.GetFeatureValueName(value);
}

FeatureValue RootFeatureType::GetDomainSize() const {
  return wrapped_type_.GetDomainSize() + 1;
}

// Parser feature locator for accessing the stack in the parser state. The
// argument represents the position on the stack, 0 being the top of the stack.
class StackGeneratorLocator : public GeneratorLocator<StackGeneratorLocator> {
 public:
  // Gets the new focus.
  int GetFocus(const WorkspaceSet &workspaces, const GeneratorState &state) const {
    const int position = argument();
    return state.Stack(position);
  }
};

REGISTER_GENERATOR_FEATURE_FUNCTION("stack", StackGeneratorLocator);

// Parser feature locator for locating the head of the focus token. The argument
// specifies the number of times the head function is applied. Please note that
// this operates on partially built dependency trees.
class HeadFeatureLocator : public GeneratorIndexLocator<HeadFeatureLocator> {
 public:
  // Updates the current focus to a new location. If the initial focus is
  // outside the range of the sentence, returns -2.
  void UpdateArgs(const WorkspaceSet &workspaces, const GeneratorState &state,
                  int *focus) const {
    if (*focus < -1 || *focus >= state.sentence().token_size()) {
      *focus = -2;
      return;
    }
    const int levels = argument();
    *focus = state.Parent(*focus, levels);
  }
};

REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("head", HeadFeatureLocator);

// Parser feature locator for locating children of the focus token. The argument
// specifies the number of times the leftmost (when the argument is < 0) or
// rightmost (when the argument > 0) child function is applied. Please note that
// this operates on partially built dependency trees.
class ChildFeatureLocator : public GeneratorIndexLocator<ChildFeatureLocator> {
 public:
  // Updates the current focus to a new location. If the initial focus is
  // outside the range of the sentence, returns -2.
  void UpdateArgs(const WorkspaceSet &workspaces, const GeneratorState &state,
                  int *focus) const {
    if (*focus < -1 || *focus >= state.sentence().token_size()) {
      *focus = -2;
      return;
    }
    const int levels = argument();
    if (levels < 0) {
      *focus = state.LeftmostChild(*focus, -levels);
    } else {
      *focus = state.RightmostChild(*focus, levels);
    }
  }
};

REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("child", ChildFeatureLocator);

// Parser feature locator for locating siblings of the focus token. The argument
// specifies the sibling position relative to the focus token: a negative value
// triggers a search to the left, while a positive value one to the right.
// Please note that this operates on partially built dependency trees.
class SiblingFeatureLocator
    : public GeneratorIndexLocator<SiblingFeatureLocator> {
 public:
  // Updates the current focus to a new location. If the initial focus is
  // outside the range of the sentence, returns -2.
  void UpdateArgs(const WorkspaceSet &workspaces, const GeneratorState &state,
                  int *focus) const {
    if (*focus < -1 || *focus >= state.sentence().token_size()) {
      *focus = -2;
      return;
    }
    const int position = argument();
    if (position < 0) {
      *focus = state.LeftSibling(*focus, -position);
    } else {
      *focus = state.RightSibling(*focus, position);
    }
  }
};

REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("sibling", SiblingFeatureLocator);

// Feature function for computing the label from focus token. Note that this
// does not use the precomputed values, since we get the labels from the stack;
// the reason it utilizes sentence_features::Label is to obtain the label map.
class LabelFeatureFunction : public BasicGeneratorSentenceFeatureFunction<Label> {
 public:
  // Computes the label of the relation between the focus token and its parent.
  // Valid focus values range from -1 to sentence->size() - 1, inclusively.
  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       int focus, const FeatureVector *result) const override {
    if (focus == -1) return RootValue();
    if (focus < -1 || focus >= state.sentence().token_size()) {
      return feature_.NumValues();
    }
    const int label = state.Label(focus);
    return label == -1 ? RootValue() : label;
  }
};

REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("label", LabelFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Word> WordFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("word", WordFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<KnownWord> KnownWordFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("known-word", KnownWordFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Char> CharFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("char", CharFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Tag> TagFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("tag", TagFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Digit> DigitFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("digit", DigitFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Hyphen> HyphenFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("hyphen", HyphenFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Capitalization>
    CapitalizationFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("capitalization",
                                     CapitalizationFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<PunctuationAmount>
    PunctuationAmountFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("punctuation-amount",
                                     PunctuationAmountFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<Quote>
    QuoteFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("quote",
                                     QuoteFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<PrefixFeature> PrefixFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("prefix", PrefixFeatureFunction);

typedef BasicGeneratorSentenceFeatureFunction<SuffixFeature> SuffixFeatureFunction;
REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("suffix", SuffixFeatureFunction);

// Parser feature function that can use nested sentence feature functions for
// feature extraction.
class GeneratorTokenFeatureFunction
    : public NestedFeatureFunction<FeatureFunction<Sentence, int>, GeneratorState,
                                   int> {
 public:
  void Preprocess(WorkspaceSet *workspaces, GeneratorState *state) const override {
    for (auto *function : nested_) {
      function->Preprocess(workspaces, state->mutable_sentence());
    }
  }

  void Evaluate(const WorkspaceSet &workspaces, const GeneratorState &state,
                int focus, FeatureVector *result) const override {
    for (auto *function : nested_) {
      function->Evaluate(workspaces, state.sentence(), focus, result);
    }
  }

  // Returns the first nested feature's computed value.
  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       int focus, const FeatureVector *result) const override {
    if (nested_.empty()) return kNone;
    return nested_[0]->Compute(workspaces, state.sentence(), focus, result);
  }
};

REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("token", GeneratorTokenFeatureFunction);

class GeneratorWholeSentenceFeatureFunction
    : public NestedFeatureFunction<FeatureFunction<Sentence>, GeneratorState> {
 public:
  void Preprocess(WorkspaceSet *workspaces, GeneratorState *state) const override {
    for (auto *function : nested_) {
      function->Preprocess(workspaces, state->mutable_sentence());
    }
  }

  void Evaluate(const WorkspaceSet &workspaces, const GeneratorState &state,
                FeatureVector *result) const override {
    for (auto *function : nested_) {
      function->Evaluate(workspaces, state.sentence(), result);
    }
  }

  // Returns the first nested feature's computed value.
  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       const FeatureVector *result) const override {
    if (nested_.empty()) return kNone;
    return nested_[0]->Compute(workspaces, state.sentence(), result);
  }
};

REGISTER_GENERATOR_FEATURE_FUNCTION("sentence",
                                 GeneratorWholeSentenceFeatureFunction);

// Parser feature that always fetches the focus (position) of the token.
class FocusFeatureFunction : public GeneratorIndexFeatureFunction {
 public:
  // Initializes the feature function.
  void Init(TaskContext *context) override {
    // Note: this feature can return up to N values, where N is the length of
    // the input sentence. Here, we give the arbitrary number 100 since it
    // is not used.
    set_feature_type(new NumericFeatureType(name(), 100));
  }

  void Evaluate(const WorkspaceSet &workspaces, const GeneratorState &object,
                int focus, FeatureVector *result) const override {
    FeatureValue value = focus;
    result->add(feature_type(), value);
  }

  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       int focus, const FeatureVector *result) const override {
    return focus;
  }
};

REGISTER_GENERATOR_IDX_FEATURE_FUNCTION("focus", FocusFeatureFunction);

// Parser feature returning a previous predicted action.
class LastActionFeatureFunction : public GeneratorFeatureFunction {
 public:
  void Init(TaskContext *context) override {
    // NB: The "100" here is totally bogus, but it doesn't matter if predicate
    // maps will be used.
    set_feature_type(new NumericFeatureType(name(), 100));
  }

  // Turn on history tracking for the parser state to get the history of
  // features.
  void Preprocess(WorkspaceSet *workspaces, GeneratorState *state) const override {
    state->set_keep_history(true);
  }

  // Returns '0' for no prior action, otherwise returns the action.
  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       const FeatureVector *result) const override {
    const int history_size = state.history().size();
    const int offset = history_size - argument() - 1;
    if (offset < 0 || offset >= history_size) return 0;
    return state.history().at(offset) + 1;
  }
};

REGISTER_GENERATOR_FEATURE_FUNCTION("last-action", LastActionFeatureFunction);

class Constant : public GeneratorFeatureFunction {
 public:
  void Init(TaskContext *context) override {
    value_ = this->GetIntParameter("value", 0);
    this->set_feature_type(new NumericFeatureType(this->name(), value_ + 1));
  }

  // Returns the constant's value.
  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       const FeatureVector *result) const override {
    return value_;
  }
 private:
  int value_ = 0;
};

REGISTER_GENERATOR_FEATURE_FUNCTION("constant", Constant);

}  // namespace syntaxnet
