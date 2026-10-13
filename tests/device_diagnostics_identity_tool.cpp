#include "device_diagnostics_identity.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char** argv)
{
	if (argc != 2) return 2;
	const std::string_view option = argv[1];
	if (option == "--version") std::cout << DeviceDiagnosticsIdentity::Version;
	else if (option == "--commit") std::cout << DeviceDiagnosticsIdentity::BuildCommit;
	else if (option == "--short-commit") std::cout << DeviceDiagnosticsIdentity::ShortCommit;
	else if (option == "--schema") std::cout << DeviceDiagnosticsIdentity::ReportSchema;
	else return 2;
	return 0;
}
