#include "overlay/graphics_ui_metadata.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <string_view>

using namespace std::literals;

int main()
{
	static_assert(GraphicsUi::Settings.size() == 23);
	static_assert(GraphicsUi::Categories == std::array{
		"DISPLAY"sv, "IMAGE QUALITY"sv, "EFFECTS"sv, "PRESENTATION"sv, "TEXTURE TOOLS"sv });

	for (const auto& setting : GraphicsUi::Settings)
	{
		assert(!setting.key.empty());
		assert(!setting.category.empty());
		assert(!setting.label.empty());
		assert(!setting.tooltip.empty());
		assert(std::find(GraphicsUi::Categories.begin(), GraphicsUi::Categories.end(), setting.category) !=
			GraphicsUi::Categories.end());
		assert(std::count_if(GraphicsUi::Settings.begin(), GraphicsUi::Settings.end(),
			[&setting](const auto& other) { return other.key == setting.key; }) == 1);
	}

	const auto find = [](std::string_view key) -> const GraphicsUi::Metadata&
	{
		const auto result = std::find_if(GraphicsUi::Settings.begin(), GraphicsUi::Settings.end(),
			[key](const auto& setting) { return setting.key == key; });
		assert(result != GraphicsUi::Settings.end());
		return *result;
	};

	assert(find("UITextureReplacement").label == "HD Interface");
	assert(find("UseHiDefCharacters").label == "Hi-Def Characters");
	assert(find("RestoreJPClarissa").label == "Japanese Clarissa");
	assert(find("UITextureReplacement").category == "PRESENTATION");
	assert(find("UseHiDefCharacters").category == "PRESENTATION");
	assert(find("RestoreJPClarissa").category == "PRESENTATION");

	assert(std::none_of(GraphicsUi::Settings.begin(), GraphicsUi::Settings.end(), [](const auto& setting)
	{
		return setting.key == "TextureBaseFolder" || setting.key == "DrawDistanceBehind";
	}));
}
