#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "tale_engine/diagnostics.h"
#include "tale_engine/dsl/lexer.h"
#include "tale_engine/dsl/parser.h"
#include "tale_engine/runtime/interpreter.h"
#include "tale_engine/runtime/state.h"
#include "tale_engine/version.h"
#include <unordered_set>

static std::string read_all_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void print_diags(const tale_engine::Diagnostics& diags) {
    for (const auto& d : diags.all()) {
        std::cerr << d.pos.file << ":" << d.pos.line << ":" << d.pos.column
            << " " << tale_engine::to_string(d.severity) << ": " << d.message << "\n";
    }
}

static bool validate_basic(const tale_engine::dsl::FileAst& ast, tale_engine::Diagnostics& diags) {
    // Reuse minimal checks (this mirrors validate tool; later we move to engine module).
    std::unordered_set<std::string> ids;
    for (const auto& s : ast.scenes) {
        if (!ids.insert(s.id).second) {
            diags.error(s.pos, "Duplicate scene id: " + s.id);
        }
    }
    if (ast.scenes.empty()) {
        diags.error(tale_engine::SourcePos{ "<input>", 1, 1 }, "No scenes found.");
    }
    return !diags.has_errors();
}

int main(int argc, char** argv) {
    using namespace tale_engine;

    if (argc < 2) {
        std::cerr << kProductName << " run\n";
        std::cerr << "Usage: tale_run <path-to-.tale> [start_scene_id]\n";
        return 2;
    }

    const std::string path = argv[1];
    const std::string start_scene = (argc >= 3) ? argv[2] : "";

    const auto text = read_all_text(path);
    Diagnostics diags;

    if (text.empty()) {
        diags.error(SourcePos{ path, 1, 1 }, "File is empty or cannot be read.");
        print_diags(diags);
        return 1;
    }

    dsl::Lexer lexer(text, path, diags);
    auto tokens = lexer.lex();

    dsl::Parser parser(std::move(tokens), diags);
    auto ast = parser.parse_file();

    if (!validate_basic(ast, diags) || diags.has_errors()) {
        print_diags(diags);
        return 1;
    }

    runtime::State state;
    runtime::Interpreter interp(ast, diags);

    if (!interp.start(state, start_scene)) {
        print_diags(diags);
        return 1;
    }

    while (true) {
        auto step = interp.step(state);

        if (diags.has_errors()) {
            print_diags(diags);
            return 1;
        }

        // Immediate transfer (top-level goto)
        if (!step.next_scene_id.empty()) {
            state.set_current_scene(step.next_scene_id);
            continue;
        }

        // Print text
        for (const auto& line : step.text) {
            std::cout << line << "\n";
        }

        // Terminal scene
        if (step.choices.empty()) {
            std::cout << "\n[End of scene: " << state.current_scene() << "]\n";
            return 0;
        }

        // Print choices
        std::cout << "\n";
        for (std::size_t i = 0; i < step.choices.size(); ++i) {
            std::cout << (i + 1) << ") " << step.choices[i].label << "\n";
        }

        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);

        int idx = 0;
        try {
            idx = std::stoi(input);
        }
        catch (...) {
            std::cout << "Invalid input.\n\n";
            continue;
        }

        if (idx < 1 || static_cast<std::size_t>(idx) > step.choices.size()) {
            std::cout << "Choice out of range.\n\n";
            continue;
        }

        if (!interp.apply_choice(state, step, static_cast<std::size_t>(idx - 1))) {
            print_diags(diags);
            return 1;
        }

        std::cout << "\n";
    }
}
