#pragma once
#include <string>
#include <variant>
#include <vector>

#include "tale_engine/diagnostics.h"

namespace tale_engine::dsl {

	struct ValueAst {
		SourcePos pos;
		std::variant<std::string, int, bool> value;
	};

	struct EffectSetFlagAst {
		SourcePos pos;
		std::string name;
		ValueAst value;
	};

	struct EffectGiveItemAst {
		SourcePos pos;
		std::string item_id;
		int qty = 0;
	};

	struct EffectTakeItemAst {
		SourcePos pos;
		std::string item_id;
		int qty = 0;
	};

	using EffectCallAst = std::variant<EffectSetFlagAst, EffectGiveItemAst, EffectTakeItemAst>;

	struct EffectStmtAst {
		SourcePos pos;
		EffectCallAst call;
	};

	struct GotoStmtAst {
		SourcePos pos;
		std::string target_scene_id;
	};

	struct TextBlockAst {
		SourcePos pos;
		std::vector<std::string> lines;
	};

	struct ChoiceAst {
		SourcePos pos;
		std::string label;
		std::vector<std::variant<GotoStmtAst, EffectStmtAst>> body;
	};

	using StmtAst = std::variant<TextBlockAst, ChoiceAst, GotoStmtAst, EffectStmtAst>;

	struct SceneAst {
		SourcePos pos;
		std::string id;
		std::vector<StmtAst> body;
	};

	struct FileAst {
		std::vector<SceneAst> scenes;
	};

} // namespace tale_engine::dsl
