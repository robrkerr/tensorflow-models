#include "dragnn/components/generator/generator_transition_state.h"

#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/logging.h"

namespace syntaxnet {
namespace dragnn {

GeneratorTransitionState::GeneratorTransitionState(
    std::unique_ptr<GeneratorState> generator_state, SyntaxNetSentence *sentence)
    : generator_state_(std::move(generator_state)), sentence_(sentence) {
  score_ = 0;
  current_beam_index_ = -1;
  parent_beam_index_ = 0;
  step_for_token_.resize(sentence->sentence()->token_size(), -1);
  parent_for_token_.resize(sentence->sentence()->token_size(), -1);
  parent_step_for_token_.resize(sentence->sentence()->token_size(), -1);
}

void GeneratorTransitionState::Init(const TransitionState &parent) {
  score_ = parent.GetScore();
  parent_beam_index_ = parent.GetBeamIndex();
}

std::unique_ptr<GeneratorTransitionState> GeneratorTransitionState::Clone()
    const {
  // Create a new state from a clone of the underlying parser state.
  std::unique_ptr<GeneratorState> cloned_state(generator_state_->Clone());
  std::unique_ptr<GeneratorTransitionState> new_state(
      new GeneratorTransitionState(std::move(cloned_state), sentence_));

  // Copy relevant data members and set non-copied ones to flag values.
  new_state->score_ = score_;
  new_state->current_beam_index_ = current_beam_index_;
  new_state->parent_beam_index_ = parent_beam_index_;
  new_state->step_for_token_ = step_for_token_;
  new_state->parent_step_for_token_ = parent_step_for_token_;
  new_state->parent_for_token_ = parent_for_token_;

  // Copy trace if it exists.
  if (trace_) {
    new_state->trace_.reset(new ComponentTrace(*trace_));
  }

  return new_state;
}

const int GeneratorTransitionState::ParentBeamIndex() const {
  return parent_beam_index_;
}

const int GeneratorTransitionState::GetBeamIndex() const {
  return current_beam_index_;
}

void GeneratorTransitionState::SetBeamIndex(const int index) {
  current_beam_index_ = index;
}

const float GeneratorTransitionState::GetScore() const { return score_; }

void GeneratorTransitionState::SetScore(const float score) { score_ = score; }

string GeneratorTransitionState::HTMLRepresentation() const {
  // Crude HTML string showing the stack and the word on the input.
  string html = "Stack: ";
  for (int i = generator_state_->StackSize() - 1; i >= 0; --i) {
    const int word_idx = generator_state_->Stack(i);
    if (word_idx >= 0) {
      tensorflow::strings::StrAppend(
          &html, generator_state_->GetToken(word_idx).word(), " ");
    }
  }
  // tensorflow::strings::StrAppend(&html, "| Input: ");
  // const int word_idx = generator_state_->Input(0);
  // if (word_idx >= 0) {
  //   tensorflow::strings::StrAppend(
  //       &html, generator_state_->GetToken(word_idx).word(), " ");
  // }

  return html;
}

}  // namespace dragnn
}  // namespace syntaxnet
