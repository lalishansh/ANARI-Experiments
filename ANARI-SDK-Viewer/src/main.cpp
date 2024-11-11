#include <iostream>

bool           g_verbose          = false;
bool           g_useDefaultLayout = true;
bool           g_enableDebug      = false;
std::string    g_libraryName      = "environment";
const char*    g_traceDir         = nullptr;

int entry_point();
int ENTRY_POINT();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void printUsage()
{
	std::cout << "./anariViewer [{--help|-h}]\n"
			  << "   [{--verbose|-v}] [{--debug|-g}]\n"
			  << "   [{--library|-l} <ANARI library>]\n"
			  << "   [{--trace|-t} <directory>]\n";
}

static void parseCommandLine(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-v" || arg == "--verbose")
			g_verbose = true;
		if (arg == "--help" || arg == "-h") {
			printUsage();
			std::exit(0);
		} else if (arg == "--noDefaultLayout")
			g_useDefaultLayout = false;
		else if (arg == "-l" || arg == "--library")
			g_libraryName = argv[++i];
		else if (arg == "--debug" || arg == "-g")
			g_enableDebug = true;
		else if (arg == "--trace" || arg == "-t")
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
#if 1
	return ENTRY_POINT();
#else
	return entry_point();
#endif
}
