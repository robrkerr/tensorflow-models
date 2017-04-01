
// Sentence-based features for the transition generator.

#ifndef SYNTAXNET_GENERATOR_FEATURES_H_
#define SYNTAXNET_GENERATOR_FEATURES_H_

#include <string>

#include "syntaxnet/feature_extractor.h"
#include "syntaxnet/feature_types.h"
#include "syntaxnet/generator_state.h"
#include "syntaxnet/task_context.h"
#include "syntaxnet/workspace.h"

namespace syntaxnet {

typedef FeatureFunction<GeneratorState> GeneratorFeatureFunction;

// Feature function for the transition parser based on a parser state object and
// a token index. This typically extracts information from a given token.
typedef FeatureFunction<GeneratorState, int> GeneratorIndexFeatureFunction;

// Utilities to register the two types of parser features.
#define REGISTER_GENERATOR_FEATURE_FUNCTION(name, component) \
  REGISTER_SYNTAXNET_FEATURE_FUNCTION(GeneratorFeatureFunction, name, component)

#define REGISTER_GENERATOR_IDX_FEATURE_FUNCTION(name, component)           \
  REGISTER_SYNTAXNET_FEATURE_FUNCTION(GeneratorIndexFeatureFunction, name, \
                                      component)

// Alias for locator type that takes a parser state, and produces a focus
// integer that can be used on nested ParserIndexFeature objects.
template<class DER>
using GeneratorLocator = FeatureAddFocusLocator<DER, GeneratorState, int>;

// Alias for Locator type features that take (GeneratorState, int) signatures and
// call other ParserIndexFeatures.
template<class DER>
using GeneratorIndexLocator = FeatureLocator<DER, GeneratorState, int>;

// Feature extractor for the transition parser based on a parser state object.
typedef FeatureExtractor<GeneratorState> GeneratorFeatureExtractor;

// Simple feature function that wraps a Sentence based feature
// function. It adds a "<ROOT>" feature value that is triggered whenever the
// focus is the special root token. This class is sub-classed based on the
// extracted arguments of the nested function.
template<class F>
class GeneratorSentenceFeatureFunction : public GeneratorIndexFeatureFunction {
 public:
  // Instantiates and sets up the nested feature.
  void Setup(TaskContext *context) override {
    this->feature_.set_descriptor(this->descriptor());
    this->feature_.set_prefix(this->prefix());
    this->feature_.set_extractor(this->extractor());
    feature_.Setup(context);
  }

  // Initializes the nested feature and sets feature type.
  void Init(TaskContext *context) override {
    feature_.Init(context);
    num_base_values_ = feature_.GetFeatureType()->GetDomainSize();
    set_feature_type(new RootFeatureType(
        name(), *feature_.GetFeatureType(), RootValue()));
  }

  // Passes workspace requests and preprocessing to the nested feature.
  void RequestWorkspaces(WorkspaceRegistry *registry) override {
    feature_.RequestWorkspaces(registry);
  }

  void Preprocess(WorkspaceSet *workspaces, GeneratorState *state) const override {
    feature_.Preprocess(workspaces, state->mutable_sentence());
  }

 protected:
  // Returns the special value to represent a root token.
  FeatureValue RootValue() const { return num_base_values_; }

  // Store the number of base values from the wrapped function so compute the
  // root value.
  int num_base_values_;

  // The wrapped feature.
  F feature_;
};

// Specialization of ParserSentenceFeatureFunction that calls the nested feature
// with (Sentence, int) arguments based on the current integer focus.
template<class F>
class BasicGeneratorSentenceFeatureFunction :
      public GeneratorSentenceFeatureFunction<F> {
 public:
  FeatureValue Compute(const WorkspaceSet &workspaces, const GeneratorState &state,
                       int focus, const FeatureVector *result) const override {
    if (focus == -1) return this->RootValue();
    return this->feature_.Compute(workspaces, state.sentence(), focus, result);
  }
};

}  // namespace syntaxnet

#endif  // SYNTAXNET_GENERATOR_FEATURES_H_
