#include "tale_engine/runtime/state.h"

#include <utility>

namespace tale_engine::runtime {

	void State::set_flag(std::string name, Value v) {
		flags_[std::move(name)] = std::move(v);
	}

	bool State::has_flag(const std::string& name) const {
		return flags_.find(name) != flags_.end();
	}

	const Value* State::get_flag(const std::string& name) const {
		auto it = flags_.find(name);
		if (it == flags_.end()) return nullptr;
		return &it->second;
	}

	void State::give_item(std::string item_id, int qty) {
		if (qty <= 0) return;
		inventory_[std::move(item_id)] += qty;
	}

	bool State::take_item(const std::string& item_id, int qty) {
		if (qty <= 0) return true;
		auto it = inventory_.find(item_id);
		if (it == inventory_.end()) return false;
		if (it->second < qty) return false;
		it->second -= qty;
		return true;
	}

	int State::get_item_qty(const std::string& item_id) const {
		auto it = inventory_.find(item_id);
		if (it == inventory_.end()) return 0;
		return it->second;
	}

	void State::set_current_scene(std::string id) {
		current_scene_ = std::move(id);
	}

	const std::string& State::current_scene() const {
		return current_scene_;
	}

} // namespace tale_engine::runtime
