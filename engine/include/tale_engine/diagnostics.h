#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tale_engine {

	enum class Severity : std::uint8_t {
		Error,
		Warning,
		Info
	};

	struct SourcePos {
		std::string file;
		int line = 1;   // 1-based
		int column = 1; // 1-based
	};

	struct Diagnostic {
		Severity severity = Severity::Error;
		SourcePos pos{};
		std::string message;
	};

	class Diagnostics {
	public:
		void error(SourcePos pos, std::string message);
		void warning(SourcePos pos, std::string message);

		bool has_errors() const;
		const std::vector<Diagnostic>& all() const;

	private:
		std::vector<Diagnostic> diags_;
	};

	std::string to_string(Severity s);

} // namespace tale_engine
