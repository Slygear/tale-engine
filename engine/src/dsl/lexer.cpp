#include "tale_engine/dsl/lexer.h"

#include <cctype>
#include <utility>

namespace tale_engine::dsl {

    Lexer::Lexer(std::string_view source, std::string filename, Diagnostics& diagnostics)
        : source_(source), filename_(std::move(filename)), diagnostics_(diagnostics) {
    }

    char Lexer::peek() const {
        if (pos_ >= source_.size()) return '\0';
        return source_[pos_];
    }

    char Lexer::advance() {
        if (pos_ >= source_.size()) return '\0';
        const char c = source_[pos_++];
        if (c == '\n') {
            line_++;
            column_ = 1;
        }
        else {
            column_++;
        }
        return c;
    }

    bool Lexer::match(char expected) {
        if (peek() != expected) return false;
        advance();
        return true;
    }

    void Lexer::emit(TokenType type, std::string lexeme) {
        // Token position refers to the beginning of the token.
        // We approximate by using current (line_, column_) minus lexeme length for same-line tokens.
        // For v1 this is sufficient; later we can carry start_pos explicitly for perfect accuracy.
        SourcePos pos;
        pos.file = filename_;
        pos.line = line_;
        pos.column = column_;

        tokens_.push_back(Token{ type, std::move(lexeme), std::move(pos) });
    }

    static bool is_ident_start(char c) {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    static bool is_ident_cont(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    void Lexer::lex_identifier() {
        const int start_line = line_;
        const int start_col = column_;

        std::string s;
        while (is_ident_cont(peek())) {
            s.push_back(advance());
        }

        SourcePos pos{ filename_, start_line, start_col };
        tokens_.push_back(Token{ TokenType::Identifier, std::move(s), std::move(pos) });
    }

    void Lexer::lex_number() {
        const int start_line = line_;
        const int start_col = column_;

        std::string s;
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            s.push_back(advance());
        }

        SourcePos pos{ filename_, start_line, start_col };
        tokens_.push_back(Token{ TokenType::Integer, std::move(s), std::move(pos) });
    }

    void Lexer::lex_string() {
        const int start_line = line_;
        const int start_col = column_;

        // Consume opening quote
        advance();

        std::string out;
        while (true) {
            char c = peek();
            if (c == '\0') {
                diagnostics_.error(SourcePos{ filename_, start_line, start_col }, "Unterminated string literal.");
                break;
            }
            if (c == '\n') {
                diagnostics_.error(SourcePos{ filename_, start_line, start_col }, "Unterminated string literal (newline).");
                break;
            }
            if (c == '"') {
                advance(); // closing quote
                break;
            }

            if (c == '\\') {
                advance(); // consume backslash
                char esc = peek();
                if (esc == '\0') {
                    diagnostics_.error(SourcePos{ filename_, line_, column_ }, "Invalid escape sequence at end of file.");
                    break;
                }

                switch (esc) {
                case '"':  out.push_back('"');  advance(); break;
                case '\\': out.push_back('\\'); advance(); break;
                case 'n':  out.push_back('\n'); advance(); break;
                case 't':  out.push_back('\t'); advance(); break;
                default:
                    diagnostics_.warning(SourcePos{ filename_, line_, column_ }, "Unknown escape sequence; treating literally.");
                    out.push_back(esc);
                    advance();
                    break;
                }
                continue;
            }

            out.push_back(advance());
        }

        SourcePos pos{ filename_, start_line, start_col };
        tokens_.push_back(Token{ TokenType::String, std::move(out), std::move(pos) });
    }

    void Lexer::handle_indentation() {
        // Called right after a newline has been consumed and tokenized.
        // We count leading spaces. Tabs are illegal in v1.
        int spaces = 0;

        while (true) {
            char c = peek();
            if (c == ' ') {
                spaces++;
                advance();
                continue;
            }
            if (c == '\t') {
                diagnostics_.error(SourcePos{ filename_, line_, column_ }, "Tabs are not allowed. Use spaces for indentation.");
                advance(); // consume to avoid infinite loop
                continue;
            }
            break;
        }

        // If the next char is newline or comment or EOF, indentation does not start a block.
        char next = peek();
        if (next == '\n' || next == '\0' || next == '#') {
            return;
        }

        // Enforce 2-space indentation unit.
        if (spaces % 2 != 0) {
            diagnostics_.error(SourcePos{ filename_, line_, column_ }, "Indentation must be a multiple of 2 spaces.");
        }

        const int current = indent_stack_.empty() ? 0 : indent_stack_.back();
        if (spaces > current) {
            indent_stack_.push_back(spaces);
            tokens_.push_back(Token{ TokenType::Indent, "", SourcePos{ filename_, line_, column_ } });
            return;
        }

        if (spaces < current) {
            while (!indent_stack_.empty() && spaces < indent_stack_.back()) {
                indent_stack_.pop_back();
                tokens_.push_back(Token{ TokenType::Dedent, "", SourcePos{ filename_, line_, column_ } });
            }
            const int after = indent_stack_.empty() ? 0 : indent_stack_.back();
            if (spaces != after) {
                diagnostics_.error(SourcePos{ filename_, line_, column_ }, "Indentation does not match any previous indentation level.");
            }
        }
    }

    void Lexer::lex_token() {
        char c = peek();

        // Windows CRLF support: ignore carriage return.
        if (c == '\r') {
            advance();
            return;
        }


        // Skip spaces inside a line (indentation handled separately after newline)
        if (c == ' ') {
            advance();
            return;
        }

        // Tabs are never allowed.
        if (c == '\t') {
            diagnostics_.error(SourcePos{ filename_, line_, column_ }, "Tabs are not allowed. Use spaces for indentation.");
            advance();
            return;
        }

        // Comments: consume until newline or EOF (but do not consume newline here)
        if (c == '#') {
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
            return;
        }

        if (c == '\n') {
            const int start_line = line_;
            const int start_col = column_;
            advance(); // consume newline
            tokens_.push_back(Token{ TokenType::Newline, "", SourcePos{ filename_, start_line, start_col } });

            // After newline, compute indentation for next non-empty line.
            handle_indentation();
            return;
        }

        if (c == '\0') {
            return;
        }

        if (is_ident_start(c)) {
            lex_identifier();
            return;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            lex_number();
            return;
        }

        if (c == '"') {
            lex_string();
            return;
        }

        // Single-character tokens
        {
            const int start_line = line_;
            const int start_col = column_;
            switch (c) {
            case ':':
                advance();
                tokens_.push_back(Token{ TokenType::Colon, ":", SourcePos{ filename_, start_line, start_col } });
                return;
            case ',':
                advance();
                tokens_.push_back(Token{ TokenType::Comma, ",", SourcePos{ filename_, start_line, start_col } });
                return;
            case '(':
                advance();
                tokens_.push_back(Token{ TokenType::LParen, "(", SourcePos{ filename_, start_line, start_col } });
                return;
            case ')':
                advance();
                tokens_.push_back(Token{ TokenType::RParen, ")", SourcePos{ filename_, start_line, start_col } });
                return;
            default:
                diagnostics_.error(SourcePos{ filename_, start_line, start_col }, "Unexpected character.");
                advance();
                return;
            }
        }
    }

    std::vector<Token> Lexer::lex() {
        tokens_.clear();

        // Handle indentation at the very beginning (top-of-file)
        // Allow leading blank lines/comments without indentation tokens.
        // If the file starts with spaces before a non-blank line, that's indentation error by rules.
        // We'll treat it as indentation logic after an implicit newline at start.
        // For v1, simplest: if leading spaces before first non-blank token => error.
        {
            std::size_t p = 0;
            int local_spaces = 0;
            while (p < source_.size() && (source_[p] == ' ' || source_[p] == '\t')) {
                if (source_[p] == '\t') {
                    diagnostics_.error(SourcePos{ filename_, 1, 1 }, "Tabs are not allowed. Use spaces for indentation.");
                    break;
                }
                local_spaces++;
                p++;
            }
            if (local_spaces > 0) {
                // Only complain if the file doesn't start with a newline/comment.
                // If it's indentation before actual content, it's suspicious.
                diagnostics_.warning(SourcePos{ filename_, 1, 1 }, "Leading spaces at top-level are ignored in v1.");
            }
        }

        while (peek() != '\0') {
            lex_token();
        }

        // Emit a final newline if the file doesn't end with one (helps parsing blocks).
        if (tokens_.empty() || tokens_.back().type != TokenType::Newline) {
            tokens_.push_back(Token{ TokenType::Newline, "", SourcePos{ filename_, line_, column_ } });
        }

        // Close any remaining indents.
        while (indent_stack_.size() > 1) {
            indent_stack_.pop_back();
            tokens_.push_back(Token{ TokenType::Dedent, "", SourcePos{ filename_, line_, column_ } });
        }

        tokens_.push_back(Token{ TokenType::EndOfFile, "", SourcePos{ filename_, line_, column_ } });
        return tokens_;
    }

} // namespace tale_engine::dsl
