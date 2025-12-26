#pragma once
#include <string>
#include <vector>

#include "tale_engine/dsl/ast.h"
#include "tale_engine/runtime/state.h"
#include "tale_engine/diagnostics.h"

namespace tale_engine::runtime {

    struct ChoiceOption {
        std::string label;
        // The index of the choice statement inside the current scene body.
        // Used to resolve effects + goto for that specific choice.
        std::size_t choice_stmt_index = 0;
    };

    struct StepResult {
        // Text lines emitted by executing the scene up to the first choice (or end).
        std::vector<std::string> text;

        // Available choices (if any). If empty, the scene is terminal (in v1).
        std::vector<ChoiceOption> choices;

        // If a scene immediately transfers (e.g., via top-level goto), next_scene_id is set.
        // For v1 we keep this simple.
        std::string next_scene_id;
    };

    class Interpreter {
    public:
        Interpreter(const dsl::FileAst& ast, Diagnostics& diagnostics);

        // Sets start scene. If empty, starts at first scene in file.
        bool start(State& state, const std::string& start_scene_id = "");

        // Executes the current scene and returns text + choices.
        StepResult step(State& state);

        // Applies the selected choice (by index in StepResult.choices) and advances state.current_scene.
        bool apply_choice(State& state, const StepResult& step, std::size_t choice_index);

    private:
        const dsl::SceneAst* find_scene(const std::string& id) const;

        // Execute helpers
        void apply_effect(State& state, const dsl::EffectStmtAst& eff);
        bool try_extract_goto(const std::vector<std::variant<dsl::GotoStmtAst, dsl::EffectStmtAst>>& body,
            std::string& out_target) const;

    private:
        const dsl::FileAst& ast_;
        Diagnostics& diags_;
    };

} // namespace tale_engine::runtime
