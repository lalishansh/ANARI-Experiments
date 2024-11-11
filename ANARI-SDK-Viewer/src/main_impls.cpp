//
// Created by ilal on 24-10-2024.
//

#include "anari_viewer/Orbit.h"
#include "anari_viewer/ui_anari.h"
#include "anari_viewer/windows/Viewport.h"

// anari
#include <anari_test_scenes.h>
// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
// glfw
#include <GLFW/glfw3.h>
// glad
#include <glad/glad.h>
// std
#include <chrono>

#include "anari_viewer/windows/LightsEditor.h"
#include "anari_viewer/windows/SceneSelector.h"

extern bool        g_verbose;
extern bool        g_useDefaultLayout;
extern bool        g_enableDebug;
extern std::string g_libraryName;
extern const char* g_traceDir;

struct AppState {
	// anari
	anari::Library debug = nullptr;
	anari::Device  device{ nullptr };
	// camera
	my_viewer::manipulators::Orbit manipulator;
	// window
	GLFWwindow* nativeWindow = nullptr;
	struct
	{
		int width = 0, height = 0;
	} size;
	std::string appName;

	std::chrono::time_point<std::chrono::steady_clock> lastFrameEndTime;
	std::chrono::time_point<std::chrono::steady_clock> lastFrameStartTime;
	bool windowResized = true;

	// remove
	using WindowArray = std::vector<my_viewer::windows::Window*>;
	WindowArray windows;
} static g_AppState;

extern const char* getDefaultUILayout ();

static void status_callback (const void* userData, ANARIDevice device, ANARIObject source,
							 ANARIDataType sourceType, ANARIStatusSeverity severity,
							 ANARIStatusCode code, const char* message) {
	const bool verbose = userData ? *(const bool*)userData : false;
	if (severity == ANARI_SEVERITY_FATAL_ERROR) {
		fprintf (stderr, "[FATAL][%p] %s\n", source, message);
		std::exit (1);
	} else if (severity == ANARI_SEVERITY_ERROR) {
		fprintf (stderr, "[ERROR][%p] %s\n", source, message);
	} else if (severity == ANARI_SEVERITY_WARNING) {
		fprintf (stderr, "[WARN ][%p] %s\n", source, message);
	} else if (verbose && severity == ANARI_SEVERITY_PERFORMANCE_WARNING) {
		fprintf (stderr, "[PERF ][%p] %s\n", source, message);
	} else if (verbose && severity == ANARI_SEVERITY_INFO) {
		fprintf (stderr, "[INFO ][%p] %s\n", source, message);
	} else if (verbose && severity == ANARI_SEVERITY_DEBUG) {
		fprintf (stderr, "[DEBUG][%p] %s\n", source, message);
	}
}

void init_ANARI() {
	auto library =
		anariLoadLibrary(g_libraryName.c_str(), status_callback, &g_verbose);
	if (library == nullptr) {
		throw std::runtime_error("Failed to load ANARI library");
	}
	anari::Device dev = anariNewDevice(library, "default");

	anariUnloadLibrary(library);

	//   if (g_verbose)
	//     anari::setParameter(dev, dev, "glDebug", true);
	//
	// #ifdef USE_GLES2
	//   anari::setParameter(dev, dev, "glAPI", "OpenGL_ES");
	// #else
	//   anari::setParameter(dev, dev, "glAPI", "OpenGL");
	// #endif

	if (g_traceDir != nullptr) {
		anari::setParameter(dev, dev, "traceDir", g_traceDir);
		anari::setParameter(dev, dev, "traceMode", "code");
	}

	anari::commitParameters(dev, dev);

	g_AppState.device = dev;
}

void init_GLFW() {
	if (glfwInit() == 0) {
		throw std::runtime_error("failed to initialize GLFW");
	}

	g_AppState.size = { 1280, 720 };

	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	auto* window = glfwCreateWindow (g_AppState.size.width, g_AppState.size.height, g_AppState.appName.c_str(), nullptr, nullptr);
	if (window == nullptr) {
		throw std::runtime_error("failed to create GLFW window");
	}

	glfwSetWindowUserPointer(window, &g_AppState);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	g_AppState.nativeWindow = window;

	if (gladLoadGLLoader (reinterpret_cast<GLADloadproc> (glfwGetProcAddress)) == 0) {
		glfwTerminate();
		throw std::runtime_error("Failed to load GL");
	}

	glfwSetFramebufferSizeCallback(
		window, [](GLFWwindow *w, int newWidth, int newHeight) {
		  auto *app = static_cast<AppState*> (glfwGetWindowUserPointer (w));
		  app->size.width = newWidth;
		  app->size.height = newHeight;
		  app->windowResized = true;
		});
}

void init_ImGUI_GLFW_OpenGL() {
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForOpenGL(g_AppState.nativeWindow, true);
	ImGui_ImplOpenGL2_Init();
}

void init_ImGUI_and_UI() {
	// imgui-styling
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.FontGlobalScale = 1.5f;
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();

	ImGuiStyle &style = ImGui::GetStyle();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.f;
	style.FrameRounding = 0.f;
	style.PopupRounding = 0.f;
	style.ScrollbarRounding = 0.f;
	style.GrabRounding = 0.f;
	style.TabRounding = 0.f;

	my_viewer::ui::init(); // native-file-dialog init

	if (g_useDefaultLayout) {
		ImGui::LoadIniSettingsFromMemory(getDefaultUILayout());
	}

	// build ui
	auto *viewport = new my_viewer::windows::Viewport(g_AppState.device, "Viewport");
	viewport->setManipulator(&g_AppState.manipulator);

	auto *leditor = new my_viewer::windows::LightsEditor(g_AppState.device);

	auto *sselector = new my_viewer::windows::SceneSelector();
	sselector->setCallback([=](const char *category, const char *scene) {
	  try {
		auto s = anari::scenes::createScene(g_AppState.device, category, scene);
		anari::scenes::commit(s);
		auto w = anari::scenes::getWorld(s);
		viewport->setWorld(w, true);
		leditor->setWorlds({w});
		sselector->setScene(s);
	  } catch (const std::runtime_error &e) {
		printf("%s\n", e.what());
	  }
	});

	AppState::WindowArray windows;
	windows.emplace_back(viewport);
	windows.emplace_back(leditor);
	windows.emplace_back(sselector);

	g_AppState.windows = std::move(windows);
}

bool window_condition() {
	const bool should_close = glfwWindowShouldClose(g_AppState.nativeWindow) == 0;

	g_AppState.lastFrameStartTime = g_AppState.lastFrameEndTime;
	g_AppState.lastFrameEndTime = std::chrono::steady_clock::now();
	glfwPollEvents();

	return should_close;
}

void draw_ui() {
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGuiIO &io = ImGui::GetIO();

	if (io.KeyCtrl && (io.KeysDown[GLFW_KEY_Q] && io.KeysDown[GLFW_KEY_W])) {
		glfwSetWindowShouldClose(g_AppState.nativeWindow, 1);
	}

	// menu bar
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("print ImGui ini")) {
				const char *info = ImGui::SaveIniSettingsToMemory();
				printf("%s\n", info);
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	// windows
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("MainDockSpace", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("MainDockSpaceID");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	// draw scene
}

void draw_scene() {
	for (auto *window : g_AppState.windows) {
		window->renderUI();
	}
}

void present() {
	ImGui::End();

	ImGui::Render();

	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(g_AppState.nativeWindow);
	g_AppState.windowResized = false;
}

void cleanup() {
	g_AppState.windows.clear();

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGui::DestroyContext();

	glfwDestroyWindow(g_AppState.nativeWindow);
	glfwTerminate();

	g_AppState.nativeWindow = nullptr;
}

int ENTRY_POINT() {
	init_ANARI();
	init_GLFW();
	init_ImGUI_GLFW_OpenGL();
	init_ImGUI_and_UI();

	while (window_condition()) {
		draw_ui();
		draw_scene();
		present();
	}

	cleanup();

	return 0;
}