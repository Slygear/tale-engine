#pragma once
#include <string>
#include <variant>

#include "tale_engine/diagnostics.h"

namespace tale_engine::runtime {

	struct Value {
		SourcePos pos;
		std::variant<std::string, int, bool> data;
	};

	inline std::string type_name(const Value& v) {
		if (std::holds_alternative<std::string>(v.data)) return "string";
		if (std::holds_alternative<int>(v.data)) return "int";
		if (std::holds_alternative<bool>(v.data)) return "bool";
		return "unknown";
	}

} // namespace tale_engine::runtime
