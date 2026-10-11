#include "input_backend_override.hpp"

#include <cassert>
#include <string_view>

using namespace HYP36RInputBackend;

int main()
{
	assert(parse("AUTOMATIC") == Override::Automatic);
	assert(parse("windows.gaming.input") == Override::Wgi);
	assert(parse("Direct_Input") == Override::DirectInput);
	assert(parse("rawinput") == Override::RawInput);
	assert(parse("X_INPUT") == Override::XInput);
	assert(parse("unsupported") == Override::Invalid);

	// Automatic is the unchanged production policy.
	assert(resolve(0, false, Override::Automatic) == Backend::Wgi);
	assert(resolve(0, true, Override::Automatic) == Backend::DirectInput);
	assert(resolve(1, true, Override::Automatic) == Backend::RawInput);
	assert(resolve(2, false, Override::Automatic) == Backend::DirectInput);
	assert(resolve(3, false, Override::Automatic) == Backend::XInput);

	assert(resolve(0, true, Override::Wgi) == Backend::Wgi);
	assert(resolve(0, false, Override::DirectInput) == Backend::DirectInput);
	assert(resolve(0, true, Override::RawInput) == Backend::RawInput);
	assert(resolve(0, true, Override::XInput) == Backend::XInput);
	assert(resolve(0, true, Override::Invalid) == Backend::DirectInput);

	assert(std::string_view(value(Override::Automatic)) == "AUTOMATIC");
	assert(std::string_view(value(Override::Wgi)) == "WGI");
	assert(std::string_view(value(Override::DirectInput)) == "DIRECTINPUT");
	assert(std::string_view(value(Override::RawInput)) == "RAWINPUT");
	assert(std::string_view(value(Override::XInput)) == "XINPUT");
	assert(!restart_required(Override::Automatic, Override::Automatic));
	assert(restart_required(Override::Automatic, Override::Wgi));
	assert(!restart_required(Override::Automatic, Override::Invalid));
	return 0;
}
