#pragma once
#include <string_view>
#include <vector>

#include "tale_engine/dsl/token.h"
#include "tale_engine/diagnostics.h"

namespace tale_engine::dsl {

    class Lexer {
    public:
        Lexer(std::string_view source,
            std::string filename,
            Diagnostics& diagnostics);

        std::vector<Token> lex();

    private:
        char peek() const;
        char advance();
        bool match(char expected);

        void lex_line();
        void lex_token();

        void lex_identifier();
        void lex_number();
        void lex_string();

        void emit(TokenType type, std::string lexeme = "");

        void handle_indentation();

    private:
        std::string_view source_;
        std::string filename_;
        Diagnostics& diagnostics_;

        std::size_t pos_ = 0;
        int line_ = 1;
        int column_ = 1;

        std::vector<int> indent_stack_{ 0 };
        std::vector<Token> tokens_;
    };

} // namespace tale_engine::dsl
