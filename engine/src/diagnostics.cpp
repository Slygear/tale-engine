#include "tale_engine/diagnostics.h"

namespace tale_engine {

    void Diagnostics::error(SourcePos pos, std::string message) {
        diags_.push_back(Diagnostic{ Severity::Error, std::move(pos), std::move(message) });
    }

    void Diagnostics::warning(SourcePos pos, std::string message) {
        diags_.push_back(Diagnostic{ Severity::Warning, std::move(pos), std::move(message) });
    }

    bool Diagnostics::has_errors() const {
        for (const auto& d : diags_) {
            if (d.severity == Severity::Error) return true;
        }
        return false;
    }

    const std::vector<Diagnostic>& Diagnostics::all() const {
        return diags_;
    }

    std::string to_string(Severity s) {
        switch (s) {
        case Severity::Error: return "error";
        case Severity::Warning: return "warning";
        case Severity::Info: return "info";
        default: return "unknown";
        }
    }

} // namespace tale_engine
