#pragma once
#include <vector>
#include <string_view>

#include "tale_engine/dsl/ast.h"
#include "tale_engine/dsl/token.h"
#include "tale_engine/diagnostics.h"

namespace tale_engine::dsl {

	class Parser {
	public:
		Parser(std::vector<Token> tokens, Diagnostics& diagnostics);

		FileAst parse_file();

	private:
		const Token& peek() const;
		const Token& previous() const;
		bool is_at_end() const;

		bool check(TokenType type) const;
		bool match(TokenType type);
		const Token& consume(TokenType type, const char* message);

		bool check_ident(std::string_view text) const;
		bool match_ident(std::string_view text);
		const Token& consume_ident(const char* message);

		void skip_newlines();

		SceneAst parse_scene();
		StmtAst parse_scene_stmt();

		TextBlockAst parse_text_block(const Token& kw);
		ChoiceAst parse_choice_block(const Token& kw);
		GotoStmtAst parse_goto_stmt(const Token& kw);
		EffectStmtAst parse_effect_stmt();

		// Effect calls
		EffectCallAst parse_effect_call(const Token& nameTok);
		ValueAst parse_value();

		int parse_int(const Token& tok);

	private:
		std::vector<Token> tokens_;
		Diagnostics& diagnostics_;
		std::size_t current_ = 0;
	};

} // namespace tale_engine::dsl
