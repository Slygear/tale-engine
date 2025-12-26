#include <fstream>
#include <iostream>
#include <sstream>

#include "tale_engine/diagnostics.h"
#include "tale_engine/version.h"

static std::string read_all_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
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
        // Stub validation: require at least one scene declaration.
        if (text.find("scene ") == std::string::npos) {
            diags.error(SourcePos{ path, 1, 1 }, "No 'scene' found. Expected at least one scene declaration.");
        }
        else {
            diags.warning(SourcePos{ path, 1, 1 }, "Stub validator: DSL parsing is not implemented yet.");
        }
    }

    for (const auto& d : diags.all()) {
        std::cerr << d.pos.file << ":" << d.pos.line << ":" << d.pos.column
            << " " << to_string(d.severity) << ": " << d.message << "\n";
    }

    return diags.has_errors() ? 1 : 0;
}
