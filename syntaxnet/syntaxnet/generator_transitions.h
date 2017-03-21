/*

==============================================================================*/

// Transition system for the transition-based generator.

#ifndef SYNTAXNET_GENERATOR_TRANSITIONS_H_
#define SYNTAXNET_GENERATOR_TRANSITIONS_H_

#include <string>
#include <vector>

#include "syntaxnet/registry.h"
#include "syntaxnet/utils.h"

namespace tensorflow {
namespace io {
class RecordReader;
class RecordWriter;
}
}

namespace syntaxnet {

class Sentence;
class GeneratorState;
class TaskContext;

// Generator actions for the transition system are encoded as integers.
typedef int GeneratorAction;

// Transition system-specific state. Transition systems can subclass this to
// preprocess the generator state and/or to keep additional information during
// generating.
class GeneratorTransitionState {
 public:
  ~GeneratorTransitionState();

  // Clones the transition state.
  GeneratorTransitionState *Clone() const;

  // Initializes a generator state for the transition system.
  void Init(GeneratorState *state);

  void CreateDocument(const GeneratorState &state,
                                  bool rewrite_root_labels,
                                  Sentence *sentence) const;

  // Returns a human readable string representation of this state.
  string ToString(const GeneratorState &state) const;
};

// A transition system is used for handling the parser state transitions. During
// training the transition system is used for extracting a canonical sequence of
// transitions for an annotated sentence. During parsing the transition system
// is used for applying the predicted transitions to the parse state and
// therefore build the parse tree for the sentence. Transition systems can be
// implemented by subclassing this abstract class and registered using the
// REGISTER_TRANSITION_SYSTEM macro.
class GeneratorTransitionSystem
    : public RegisterableClass<GeneratorTransitionSystem> {
 public:
  // Construction and cleanup.
  ~GeneratorTransitionSystem();

  // Sets up the transition system. If inputs are needed, this is the place to
  // specify them.
  void Setup(TaskContext *context);

  // Initializes the transition system.
  void Init(TaskContext *context);

  // Reads the transition system from disk.
  void Read(tensorflow::io::RecordReader *reader);

  // Writes the transition system to disk.
  void Write(tensorflow::io::RecordWriter *writer) const;

  // Returns the number of action types.
  int NumActionTypes() const;

  // Returns the number of actions.
  int NumActions(int num_labels) const;

  // Internally creates the set of outcomes (when transition systems support a
  // variable number of actions).
  void CreateOutcomeSet(int num_labels);

  // Returns the default action for a given state.
  GeneratorAction GetDefaultAction(const GeneratorState &state) const;

  // Returns the number of atomic actions within the specified ParserAction.
  int ActionLength(GeneratorAction action) const;

  // Returns true if the action is allowed in the given parser state.
  bool IsAllowedAction(GeneratorAction action, const GeneratorState &state) const;

  // Performs the specified action on a given parser state. The action is not
  // saved in the state's history.
  void PerformActionWithoutHistory(GeneratorAction action, GeneratorState *state) const;

  // Performs the specified action on a given parser state. The action is saved
  // in the state's history.
  void PerformAction(GeneratorAction action, GeneratorState *state) const;

  // Returns true if a given state is deterministic.
  bool IsDeterministicState(const GeneratorState &state) const;

  // Returns true if no more actions can be applied to a given parser state.
  bool IsFinalState(const GeneratorState &state) const;

  // Returns a string representation of a parser action.
  string ActionAsString(GeneratorAction action, const GeneratorState &state) const;

  // Returns a new transition state that can be used to put additional
  // information in a parser state. By specifying if we are in training_mode
  // (true) or not (false), we can construct a different transition state
  // depending on whether we are training a model or parsing new documents. A
  // null return value means we don't need to add anything to the parser state.
  GeneratorTransitionState *NewTransitionState(bool training_mode) const;

  // Whether to back off to the best allowable transition rather than the
  // default action when the highest scoring action is not allowed.  Some
  // transition systems do not degrade gracefully to the default action and so
  // should return true for this function.
  bool BackOffToBestAllowableTransition() const;

  // Whether the system allows non-projective trees.
  bool AllowsNonProjective() const;

  // Action meta data: get pointers to token indices based on meta-info about
  // (state, action) pairs. NOTE: the following interface is somewhat
  // experimental and may be subject to change. Use with caution and ask
  // googleuser@ for details.

  // Whether or not the system supports computing meta-data about actions.
  bool SupportsActionMetaData() const;

  // Get the index of the child that would be created by this action. -1 for
  // no child created.
  int ChildIndex(const GeneratorState &state, const GeneratorAction &action) const;

  // Get the index of the parent that would gain a new child by this action. -1
  // for no parent modified.
  int ParentIndex(const GeneratorState &state, const GeneratorAction &action) const;

 private:
  TF_DISALLOW_COPY_AND_ASSIGN(GeneratorTransitionSystem);
  
  int num_labels;
  int num_tags;
  int num_words;
};

#define REGISTER_TRANSITION_SYSTEM(type, component) \
  REGISTER_SYNTAXNET_CLASS_COMPONENT(GeneratorTransitionSystem, type, component)

}  // namespace syntaxnet

#endif  // SYNTAXNET_GENERATOR_TRANSITIONS_H_
