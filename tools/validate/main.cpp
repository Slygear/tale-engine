#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include "tale_engine/diagnostics.h"
#include "tale_engine/version.h"
#include "tale_engine/dsl/lexer.h"
#include "tale_engine/dsl/parser.h"
#include "tale_engine/dsl/ast.h"

static std::string read_all_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void validate_ast(const tale_engine::dsl::FileAst& ast,
    tale_engine::Diagnostics& diags) {
    using namespace tale_engine;

    // 1) Unique scene ids
    std::unordered_set<std::string> scene_ids;
    for (const auto& s : ast.scenes) {
        if (!scene_ids.insert(s.id).second) {
            diags.error(s.pos, "Duplicate scene id: " + s.id);
        }
    }

    auto scene_exists = [&](const std::string& id) {
        return scene_ids.count(id) != 0;
        };

    // 2) Goto targets exist
    for (const auto& s : ast.scenes) {
        for (const auto& stmt : s.body) {
            if (auto g = std::get_if<tale_engine::dsl::GotoStmtAst>(&stmt)) {
                if (!scene_exists(g->target_scene_id)) {
                    diags.error(g->pos, "Goto target scene does not exist: " + g->target_scene_id);
                }
            }
            else if (auto ch = std::get_if<tale_engine::dsl::ChoiceAst>(&stmt)) {
                for (const auto& cstmt : ch->body) {
                    if (auto cg = std::get_if<tale_engine::dsl::GotoStmtAst>(&cstmt)) {
                        if (!scene_exists(cg->target_scene_id)) {
                            diags.error(cg->pos, "Goto target scene does not exist: " + cg->target_scene_id);
                        }
                    }
                }
            }
        }
    }

    // 3) Minimal sanity: require at least one scene
    if (ast.scenes.empty()) {
        diags.error(SourcePos{ "<input>", 1, 1 }, "No scenes found. Expected at least one 'scene' block.");
    }
}

int main(int argc, char** argv) {
    using namespace tale_engine;

    if (argc < 2) {
        std::cerr << kProductName << " validate\n";
        std::cerr << "Usage: tale_validate <path-to-.tale>\n";
        return 2;
    }

    const std::string path = argv[1];
    const auto text = read_all_text(path);

    Diagnostics diags;

    if (text.empty()) {
        diags.error(SourcePos{ path, 1, 1 }, "File is empty or cannot be read.");
    }
    else {
        // Lex -> Parse -> Validate
        tale_engine::dsl::Lexer lexer(text, path, diags);
        auto tokens = lexer.lex();

        tale_engine::dsl::Parser parser(std::move(tokens), diags);
        auto ast = parser.parse_file();

        validate_ast(ast, diags);
    }

    for (const auto& d : diags.all()) {
        std::cerr << d.pos.file << ":" << d.pos.line << ":" << d.pos.column
            << " " << to_string(d.severity) << ": " << d.message << "\n";
    }

    return diags.has_errors() ? 1 : 0;
}
