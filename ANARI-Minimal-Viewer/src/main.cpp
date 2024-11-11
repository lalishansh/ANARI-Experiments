#include <filesystem>
#include <iostream>
#include <thread>

#include "anari_test_scenes.h"

bool           g_verbose          = false;
std::string    g_libraryName      = "environment";
bool           g_enableDebug      = false;
const char*    g_traceDir         = nullptr;

int ENTRY_POINT();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void printUsage()
{
	std::cout << "./anariViewer [{--help|-h}]\n"
			  << "   [{--verbose|-v}] [{--debug|-d}]\n"
			  << "   [{--library|-l} <ANARI library>]\n"
			  << "   [{--trace|-t} <directory>]\n\n"
			  << "Available libraries:\n";
	for (auto const& dir_entry : std::filesystem::directory_iterator{std::filesystem::current_path()}) {
		auto extension = dir_entry.path().extension().string();
		auto filename = dir_entry.path().filename().string();
		auto basename = filename.substr(0, filename.size() - extension.size());

		const char lib_prefix[] = "anari_library_";
		if (basename.starts_with(lib_prefix)) {
			auto lib_name = basename.substr(std::size(lib_prefix) - 1);
			if (ANARILibrary lib = anariLoadLibrary(lib_name.c_str())) {
				std::cout << "   " << lib_name << "\n";
				anariUnloadLibrary(lib);
			}
		}
	}
}

static void parseCommandLine(int argc, char *argv[]) {
	if (argc <= 1) printUsage(), std::exit(0);
	else for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			printUsage();
			std::cout << std::flush, std::this_thread::sleep_for(std::chrono::seconds(1));
			std::exit(0);
		}
		else if (arg == "-v" || arg == "--verbose")
			g_verbose = true;
		else if (arg == "-l" || arg == "--library")
			g_libraryName = argv[++i];
		else if (arg == "-d" || arg == "--debug")
			g_enableDebug = true;
		else if (arg == "-t" || arg == "--trace")
			g_traceDir = argv[++i];
	}
}

#if _WIN32
#include <Windows.h>
#undef min
#undef max
#endif
int
#if _WIN32
	WINAPI
	WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
	main(int __argc, const char *__argv[])
#endif
{
	(void)__argc, (void)__argv;

	parseCommandLine(__argc, __argv);
	return ENTRY_POINT(), std::this_thread::sleep_for(std::chrono::seconds(1)), 0;
}
