#include "tale_engine/runtime/interpreter.h"

#include <utility>

namespace tale_engine::runtime {

    Interpreter::Interpreter(const dsl::FileAst& ast, Diagnostics& diagnostics)
        : ast_(ast), diags_(diagnostics) {
    }

    const dsl::SceneAst* Interpreter::find_scene(const std::string& id) const {
        for (const auto& s : ast_.scenes) {
            if (s.id == id) return &s;
        }
        return nullptr;
    }

    bool Interpreter::start(State& state, const std::string& start_scene_id) {
        if (ast_.scenes.empty()) {
            diags_.error(SourcePos{ "<runtime>", 1, 1 }, "No scenes available to start.");
            return false;
        }

        if (!start_scene_id.empty()) {
            if (!find_scene(start_scene_id)) {
                diags_.error(SourcePos{ "<runtime>", 1, 1 }, "Start scene does not exist: " + start_scene_id);
                return false;
            }
            state.set_current_scene(start_scene_id);
            return true;
        }

        state.set_current_scene(ast_.scenes.front().id);
        return true;
    }

    static runtime::Value to_runtime_value(const dsl::ValueAst& v) {
        runtime::Value rv;
        rv.pos = v.pos;

        if (std::holds_alternative<std::string>(v.value)) {
            rv.data = std::get<std::string>(v.value);
        }
        else if (std::holds_alternative<int>(v.value)) {
            rv.data = std::get<int>(v.value);
        }
        else if (std::holds_alternative<bool>(v.value)) {
            rv.data = std::get<bool>(v.value);
        }
        return rv;
    }

    void Interpreter::apply_effect(State& state, const dsl::EffectStmtAst& eff) {
        const auto& call = eff.call;

        if (const auto* s = std::get_if<dsl::EffectSetFlagAst>(&call)) {
            state.set_flag(s->name, to_runtime_value(s->value));
            return;
        }

        if (const auto* g = std::get_if<dsl::EffectGiveItemAst>(&call)) {
            state.give_item(g->item_id, g->qty);
            return;
        }

        if (const auto* t = std::get_if<dsl::EffectTakeItemAst>(&call)) {
            const bool ok = state.take_item(t->item_id, t->qty);
            if (!ok) {
                diags_.warning(t->pos, "take_item failed due to insufficient quantity: " + t->item_id);
            }
            return;
        }
    }

    bool Interpreter::try_extract_goto(
        const std::vector<std::variant<dsl::GotoStmtAst, dsl::EffectStmtAst>>& body,
        std::string& out_target) const {

        for (const auto& s : body) {
            if (const auto* g = std::get_if<dsl::GotoStmtAst>(&s)) {
                out_target = g->target_scene_id;
                return true;
            }
        }
        return false;
    }

    StepResult Interpreter::step(State& state) {
        StepResult r;

        const auto* scene = find_scene(state.current_scene());
        if (!scene) {
            diags_.error(SourcePos{ "<runtime>", 1, 1 }, "Current scene does not exist: " + state.current_scene());
            return r;
        }

        // Execute statements in order until we reach a choice block.
        for (std::size_t i = 0; i < scene->body.size(); ++i) {
            const auto& stmt = scene->body[i];

            if (const auto* tb = std::get_if<dsl::TextBlockAst>(&stmt)) {
                for (const auto& line : tb->lines) r.text.push_back(line);
                continue;
            }

            if (const auto* eff = std::get_if<dsl::EffectStmtAst>(&stmt)) {
                apply_effect(state, *eff);
                continue;
            }

            if (const auto* g = std::get_if<dsl::GotoStmtAst>(&stmt)) {
                // Immediate transfer.
                r.next_scene_id = g->target_scene_id;
                return r;
            }

            if (const auto* ch = std::get_if<dsl::ChoiceAst>(&stmt)) {
                // Emit one choice option for this choice block.
                r.choices.push_back(ChoiceOption{ ch->label, i });
                // v1 behavior: collect consecutive choices too
                for (std::size_t j = i + 1; j < scene->body.size(); ++j) {
                    const auto& next = scene->body[j];
                    if (const auto* ch2 = std::get_if<dsl::ChoiceAst>(&next)) {
                        r.choices.push_back(ChoiceOption{ ch2->label, j });
                        continue;
                    }
                    break;
                }
                return r;
            }
        }

        // Terminal scene: no next scene, no choices.
        return r;
    }

    bool Interpreter::apply_choice(State& state, const StepResult& step, std::size_t choice_index) {
        if (choice_index >= step.choices.size()) {
            diags_.error(SourcePos{ "<runtime>", 1, 1 }, "Choice index out of range.");
            return false;
        }

        const auto* scene = find_scene(state.current_scene());
        if (!scene) return false;

        const auto choice_stmt_index = step.choices[choice_index].choice_stmt_index;
        if (choice_stmt_index >= scene->body.size()) return false;

        const auto* ch = std::get_if<dsl::ChoiceAst>(&scene->body[choice_stmt_index]);
        if (!ch) return false;

        // Apply effects in the choice body, then goto (first goto wins).
        std::string target;
        for (const auto& s : ch->body) {
            if (const auto* eff = std::get_if<dsl::EffectStmtAst>(&s)) {
                apply_effect(state, *eff);
            }
        }

        if (!try_extract_goto(ch->body, target)) {
            diags_.warning(ch->pos, "Choice has no goto; staying in current scene.");
            return true;
        }

        if (!find_scene(target)) {
            diags_.error(ch->pos, "Choice goto target does not exist: " + target);
            return false;
        }

        state.set_current_scene(target);
        return true;
    }

} // namespace tale_engine::runtime
