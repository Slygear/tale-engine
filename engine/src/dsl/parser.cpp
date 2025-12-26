#include "tale_engine/dsl/parser.h"

#include <string>
#include <utility>

namespace tale_engine::dsl {

    Parser::Parser(std::vector<Token> tokens, Diagnostics& diagnostics)
        : tokens_(std::move(tokens)), diagnostics_(diagnostics) {
    }

    const Token& Parser::peek() const { return tokens_[current_]; }
    const Token& Parser::previous() const { return tokens_[current_ - 1]; }
    bool Parser::is_at_end() const { return peek().type == TokenType::EndOfFile; }

    bool Parser::check(TokenType type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    bool Parser::match(TokenType type) {
        if (check(type)) {
            current_++;
            return true;
        }
        return false;
    }

    const Token& Parser::consume(TokenType type, const char* message) {
        if (check(type)) return tokens_[current_++];

        diagnostics_.error(peek().pos, message);
        return tokens_[current_]; // error recovery: return current token
    }

    bool Parser::check_ident(std::string_view text) const {
        if (peek().type != TokenType::Identifier) return false;
        return peek().lexeme == text;
    }

    bool Parser::match_ident(std::string_view text) {
        if (check_ident(text)) {
            current_++;
            return true;
        }
        return false;
    }

    const Token& Parser::consume_ident(const char* message) {
        if (peek().type == TokenType::Identifier) return tokens_[current_++];
        diagnostics_.error(peek().pos, message);
        return tokens_[current_];
    }

    void Parser::skip_newlines() {
        while (match(TokenType::Newline)) {}
    }

    FileAst Parser::parse_file() {
        FileAst file;
        skip_newlines();

        while (!is_at_end()) {
            if (match_ident("scene")) {
                file.scenes.push_back(parse_scene());
            }
            else {
                diagnostics_.error(peek().pos, "Expected 'scene' at top level.");
                current_++; // recovery
            }
            skip_newlines();
        }
        return file;
    }

    SceneAst Parser::parse_scene() {
        const Token& scene_kw = previous(); // 'scene'
        const Token& id = consume(TokenType::Identifier, "Expected scene id after 'scene'.");
        consume(TokenType::Colon, "Expected ':' after scene id.");
        consume(TokenType::Newline, "Expected newline after scene header.");
        consume(TokenType::Indent, "Expected an indented scene body.");

        SceneAst scene;
        scene.pos = scene_kw.pos;
        scene.id = id.lexeme;

        skip_newlines();
        while (!check(TokenType::Dedent) && !is_at_end()) {
            scene.body.push_back(parse_scene_stmt());
            skip_newlines();
        }

        consume(TokenType::Dedent, "Expected dedent after scene body.");
        return scene;
    }

    StmtAst Parser::parse_scene_stmt() {
        if (match_ident("text")) {
            return parse_text_block(previous());
        }
        if (match_ident("choice")) {
            return parse_choice_block(previous());
        }
        if (match_ident("goto")) {
            return parse_goto_stmt(previous());
        }

        // Effects are identifiers too (set_flag/give_item/take_item)
        if (peek().type == TokenType::Identifier) {
            return parse_effect_stmt();
        }

        diagnostics_.error(peek().pos, "Unexpected token in scene body.");
        current_++; // recovery
        // Return an empty text block placeholder to keep AST valid.
        return TextBlockAst{ peek().pos, {} };
    }

    TextBlockAst Parser::parse_text_block(const Token& kw) {
        consume(TokenType::Colon, "Expected ':' after 'text'.");
        consume(TokenType::Newline, "Expected newline after 'text:'.");
        consume(TokenType::Indent, "Expected an indented text block.");

        TextBlockAst tb;
        tb.pos = kw.pos;

        skip_newlines();
        while (!check(TokenType::Dedent) && !is_at_end()) {
            const Token& line = consume(TokenType::String, "Expected string line inside text block.");
            tb.lines.push_back(line.lexeme);
            consume(TokenType::Newline, "Expected newline after text line.");
            skip_newlines();
        }

        consume(TokenType::Dedent, "Expected dedent after text block.");
        return tb;
    }

    ChoiceAst Parser::parse_choice_block(const Token& kw) {
        const Token& label = consume(TokenType::String, "Expected choice label string.");
        consume(TokenType::Colon, "Expected ':' after choice label.");
        consume(TokenType::Newline, "Expected newline after choice header.");
        consume(TokenType::Indent, "Expected an indented choice body.");

        ChoiceAst ch;
        ch.pos = kw.pos;
        ch.label = label.lexeme;

        skip_newlines();
        while (!check(TokenType::Dedent) && !is_at_end()) {
            if (match_ident("goto")) {
                ch.body.push_back(parse_goto_stmt(previous()));
            }
            else if (peek().type == TokenType::Identifier) {
                ch.body.push_back(parse_effect_stmt());
            }
            else {
                diagnostics_.error(peek().pos, "Unexpected token in choice body.");
                current_++; // recovery
            }
            skip_newlines();
        }

        consume(TokenType::Dedent, "Expected dedent after choice body.");
        return ch;
    }

    GotoStmtAst Parser::parse_goto_stmt(const Token& kw) {
        const Token& target = consume(TokenType::Identifier, "Expected target scene id after 'goto'.");
        // newline consumed by outer loops in most cases, but make it strict:
        // Allow either newline or dedent/end depending on context. We'll accept optional newline here.
        if (check(TokenType::Newline)) {
            consume(TokenType::Newline, "Expected newline after goto.");
        }

        GotoStmtAst g;
        g.pos = kw.pos;
        g.target_scene_id = target.lexeme;
        return g;
    }

    EffectStmtAst Parser::parse_effect_stmt() {
        const Token& nameTok = consume_ident("Expected effect name.");
        consume(TokenType::LParen, "Expected '(' after effect name.");

        EffectStmtAst s;
        s.pos = nameTok.pos;
        s.call = parse_effect_call(nameTok);

        consume(TokenType::RParen, "Expected ')' after effect arguments.");

        if (check(TokenType::Newline)) {
            consume(TokenType::Newline, "Expected newline after effect.");
        }

        return s;
    }

    EffectCallAst Parser::parse_effect_call(const Token& nameTok) {
        if (nameTok.lexeme == "set_flag") {
            const Token& flag = consume(TokenType::Identifier, "Expected flag name (identifier).");
            consume(TokenType::Comma, "Expected ',' after flag name.");
            ValueAst v = parse_value();

            EffectSetFlagAst e;
            e.pos = nameTok.pos;
            e.name = flag.lexeme;
            e.value = std::move(v);
            return e;
        }

        if (nameTok.lexeme == "give_item") {
            const Token& item = consume(TokenType::Identifier, "Expected item id (identifier).");
            consume(TokenType::Comma, "Expected ',' after item id.");
            const Token& qtyTok = consume(TokenType::Integer, "Expected quantity (integer).");

            EffectGiveItemAst e;
            e.pos = nameTok.pos;
            e.item_id = item.lexeme;
            e.qty = parse_int(qtyTok);
            return e;
        }

        if (nameTok.lexeme == "take_item") {
            const Token& item = consume(TokenType::Identifier, "Expected item id (identifier).");
            consume(TokenType::Comma, "Expected ',' after item id.");
            const Token& qtyTok = consume(TokenType::Integer, "Expected quantity (integer).");

            EffectTakeItemAst e;
            e.pos = nameTok.pos;
            e.item_id = item.lexeme;
            e.qty = parse_int(qtyTok);
            return e;
        }

        diagnostics_.error(nameTok.pos, "Unknown effect function.");
        // Best-effort recovery: parse a value and ignore until ')'
        return EffectGiveItemAst{ nameTok.pos, "", 0 };
    }

    ValueAst Parser::parse_value() {
        ValueAst v;
        v.pos = peek().pos;

        if (match(TokenType::String)) {
            v.value = previous().lexeme;
            return v;
        }

        if (match(TokenType::Integer)) {
            v.value = parse_int(previous());
            return v;
        }

        if (match_ident("true")) {
            v.value = true;
            return v;
        }

        if (match_ident("false")) {
            v.value = false;
            return v;
        }

        diagnostics_.error(peek().pos, "Expected value (string, integer, true, false).");
        // recovery: consume one token
        current_++;
        v.value = false;
        return v;
    }

    int Parser::parse_int(const Token& tok) {
        try {
            return std::stoi(tok.lexeme);
        }
        catch (...) {
            diagnostics_.error(tok.pos, "Invalid integer literal.");
            return 0;
        }
    }

} // namespace tale_engine::dsl
