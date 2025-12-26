#pragma once
#include <string>
#include <unordered_map>

#include "tale_engine/runtime/value.h"

namespace tale_engine::runtime {

	class State {
	public:
		void set_flag(std::string name, Value v);
		bool has_flag(const std::string& name) const;
		const Value* get_flag(const std::string& name) const;

		void give_item(std::string item_id, int qty);
		bool take_item(const std::string& item_id, int qty); // returns false if insufficient
		int get_item_qty(const std::string& item_id) const;

		void set_current_scene(std::string id);
		const std::string& current_scene() const;

	private:
		std::unordered_map<std::string, Value> flags_;
		std::unordered_map<std::string, int> inventory_;
		std::string current_scene_;
	};

} // namespace tale_engine::runtime
