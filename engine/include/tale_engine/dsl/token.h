#pragma once
#include <string>
#include <cstdint>

#include "tale_engine/diagnostics.h"

namespace tale_engine::dsl {

	enum class TokenType : std::uint8_t {
		Identifier,
		String,
		Integer,

		Colon,
		Comma,
		LParen,
		RParen,

		Newline,
		Indent,
		Dedent,

		EndOfFile
	};

	struct Token {
		TokenType type;
		std::string lexeme;
		SourcePos pos;
	};

} // namespace tale_engine::dsl
