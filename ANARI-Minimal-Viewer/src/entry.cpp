
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <anari/anari_cpp.hpp>
#include <chrono>
#include <array>

using uvec2 = unsigned int[2];
using uvec3 = unsigned int[3];
using ivec2 = int[2];
using ivec3 = int[3];
using fvec3 = float[3];
using fvec4 = float[4];

struct {
	anari::Device device {nullptr};

	anari::Camera perspectiveCamera {nullptr};

	anari::Frame frame {nullptr};
} g_ANARIState; // ANARI Device and Scene handle

struct
{
	GLFWwindow* nativeWindow = nullptr;

	struct
	{
		int width = 0, height = 0;
		bool resized = true;
	} size;

	std::chrono::duration<float> lastFrameTime;
	std::chrono::time_point<std::chrono::system_clock> currentFrameStartTime;

	GLuint outputTexture = 0;

	// screen quad
	GLuint quadVAO = 0, quadVBO = 0;
	GLuint screenShader = 0;
} g_AppState;

extern bool        g_verbose;
extern std::string g_libraryName;
extern std::string g_categoryName;
extern std::string g_sceneName;
extern bool        g_enableDebug;
extern const char* g_traceDir;

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

static void initialize_ALL() {
	g_AppState.size = {1280, 720};

	{ // ANARI Device
		auto library =
			anariLoadLibrary(g_libraryName.c_str(), status_callback, &g_verbose);
		if (library == nullptr) {
			throw std::runtime_error("Failed to load ANARI library");
		}

		// anari::Extensions extensions = anari::extension::getDeviceExtensionStruct(library, "default");
		// if (!extensions.ANARI_KHR_GEOMETRY_TRIANGLE) {
		// 	throw std::runtime_error("device doesn't support ANARI_KHR_GEOMETRY_TRIANGLE");
		// }
		// if (!extensions.ANARI_KHR_CAMERA_PERSPECTIVE) {
		// 	throw std::runtime_error("device doesn't support ANARI_KHR_CAMERA_PERSPECTIVE");
		// }

		auto dev = g_ANARIState.device = anariNewDevice(library, "default");

		anariUnloadLibrary(library);

		if (g_traceDir != nullptr) {
			anari::setParameter(dev, dev, "traceDir", g_traceDir);
			anari::setParameter(dev, dev, "traceMode", "code");
		}

		anari::commitParameters(dev, dev);
	}

	{
		auto& d = g_ANARIState.device;

		// Camera
		auto camera = g_ANARIState.perspectiveCamera = anari::newObject<anari::Camera>(d, "perspective");
		const fvec3 cam_pos = {0.f, 0.f, 0.f};
		const fvec3 cam_up = {0.f, 1.f, 0.f};
		const fvec3 cam_view = {0.1f, 0.f, 1.f};
		anari::setParameter(d, camera, "aspect", float(g_AppState.size.width)/float(g_AppState.size.height));
		anari::setParameter(d, camera, "position", cam_pos);
		anari::setParameter(d, camera, "direction", cam_view);
		anari::setParameter(d, camera, "up", cam_up);
		anari::commitParameters(d, camera); // commit objects to indicate setting parameters is done

		// Scene
		auto scene = anari::newObject<anari::World>(d);
		std::vector<anari::Surface> renderableObjects;
		// 1. triangle
		{
			fvec3 vertex[] = {{-1.0f, -1.0f, 3.0f},
					  {-1.0f, 1.0f, 3.0f},
					  {1.0f, -1.0f, 3.0f},
					  {0.1f, 0.1f, 0.3f}};
			fvec4 color[] = {{0.9f, 0.5f, 0.5f, 1.0f},
					{0.8f, 0.8f, 0.8f, 1.0f},
					{0.8f, 0.8f, 0.8f, 1.0f},
					{0.5f, 0.9f, 0.5f, 1.0f}};
			uvec3 index[] = {{0, 1, 2}, {1, 2, 3}};

			auto mesh = anari::newObject<anari::Geometry>(d, "triangle");
			anari::setParameterArray1D(d, mesh, "vertex.position", vertex, 4);
			anari::setParameterArray1D(d, mesh, "vertex.color", color, 4);
			anari::setParameterArray1D(d, mesh, "primitive.index", index, 2);
			anari::commitParameters(d, mesh);

			auto mat = anari::newObject<anari::Material>(d, "matte");
			anari::setParameter(d, mat, "color", "color");
			anari::commitParameters(d, mat);

			auto surface = anari::newObject<anari::Surface>(d);
			anari::setAndReleaseParameter(d, surface, "geometry", mesh);
			anari::setAndReleaseParameter(d, surface, "material", mat);
			anari::setParameter(d, surface, "id", 2u);
			anari::commitParameters(d, surface);

			renderableObjects.push_back(surface);
		}

		// END, add all instances to the scene
		anari::setParameterArray1D(d, scene, "surface", renderableObjects.data(), renderableObjects.size());
		anari::setParameter(d, scene, "id", 3u);

		for (auto& obj: renderableObjects)
			anari::release(d, obj);
		renderableObjects.clear();

		anari::commitParameters(d, scene);

		// Rendering setup
		auto renderer = g_ANARIState.renderer = anari::newObject<anari::Renderer>(d, "default");
		// objects can be named for easier identification in debug output etc.
		anari::setParameter(d, renderer, "name", "MainRenderer");
		anari::setParameter(d, renderer, "ambientRadiance", 1.f);
		anari::commitParameters(d, renderer);

		// create and setup frame
		auto frame = g_ANARIState.frame = anari::newObject<anari::Frame>(d);
		const uvec2 fb_size = {(unsigned)g_AppState.size.width, (unsigned)g_AppState.size.height};
		anari::setParameter(d, frame, "size", fb_size);
		anari::setParameter(d, frame, "channel.color", ANARI_UFIXED8_RGBA_SRGB);
		anari::setParameter(d, frame, "channel.primitiveId", ANARI_UINT32);
		anari::setParameter(d, frame, "channel.objectId", ANARI_UINT32);
		anari::setParameter(d, frame, "channel.instanceId", ANARI_UINT32);

		anari::setParameter(d, frame, "camera", g_ANARIState.perspectiveCamera);
		anari::setAndReleaseParameter(d, frame, "renderer", renderer);
		anari::setAndReleaseParameter(d, frame, "world", scene);

		anari::commitParameters(d, frame);
	}

	{ // GLFW & GLAD
		if (glfwInit() == 0) {
			throw std::runtime_error("failed to initialize GLFW");
		}

		auto wnd = g_AppState.nativeWindow = glfwCreateWindow(g_AppState.size.width, g_AppState.size.height, "ANARI-glTF-Minimal-Viewer", nullptr, nullptr);
		if (wnd == nullptr) {
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(wnd);
		glfwSwapInterval(1);

		if (gladLoadGLLoader (reinterpret_cast<GLADloadproc> (glfwGetProcAddress)) == 0) {
			glfwTerminate();
			throw std::runtime_error("Failed to load GL");
		}

		glfwSetFramebufferSizeCallback(
			wnd, [](GLFWwindow *w, int newWidth, int newHeight) {
				g_AppState.size = { newWidth, newHeight, true };
			});

		glGenTextures(1, &g_AppState.outputTexture);
		glBindTexture(GL_TEXTURE_2D, g_AppState.outputTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			g_AppState.size.width,
			g_AppState.size.height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			0);

		// For screen quad
		const float quadVertices[] = {
			// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};
		glGenVertexArrays(1, &g_AppState.quadVAO);
		glGenBuffers(1, &g_AppState.quadVBO);
		glBindVertexArray(g_AppState.quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, g_AppState.quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		const char* vShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoords = aTexCoords;
}
)";
		const char* pShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoords);
}
)";
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vShader, nullptr);
		glCompileShader(vertexShader);
		GLuint pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(pixelShader, 1, &pShader, nullptr);
		glCompileShader(pixelShader);
		g_AppState.screenShader = glCreateProgram();
		glAttachShader(g_AppState.screenShader, vertexShader);
		glAttachShader(g_AppState.screenShader, pixelShader);
		glLinkProgram(g_AppState.screenShader);
		glDeleteShader(vertexShader);
		glDeleteShader(pixelShader);
	}
}

static bool window_condition() {
	const bool should_close = glfwWindowShouldClose(g_AppState.nativeWindow) == 0;

	auto now = std::chrono::system_clock::now();
	g_AppState.lastFrameTime = g_AppState.currentFrameStartTime - now;
	g_AppState.currentFrameStartTime = now;
	g_AppState.size.resized = false;

	glfwPollEvents();
	return should_close;
}

static void update() {
	if (g_AppState.size.resized) {
		// ANARI
		const uvec2 fb_size = {unsigned(g_AppState.size.width), unsigned(g_AppState.size.height)};

		anari::setParameter(g_ANARIState.device, g_ANARIState.perspectiveCamera, "aspect", float(g_AppState.size.width)/float(g_AppState.size.height));
		anari::commitParameters(g_ANARIState.device, g_ANARIState.perspectiveCamera);

		anari::setParameter(g_ANARIState.device, g_ANARIState.frame, "size", fb_size);
		anari::commitParameters(g_ANARIState.device, g_ANARIState.frame);

		// GLAD
		glViewport(0, 0, fb_size[0], fb_size[1]);

		glBindTexture(GL_TEXTURE_2D, g_AppState.outputTexture);
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			fb_size[0],
			fb_size[1],
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			0);
	}



	anari::render(g_ANARIState.device, g_ANARIState.frame);
	anari::wait(g_ANARIState.device, g_ANARIState.frame);

	auto fb = anari::map<uint32_t>(g_ANARIState.device, g_ANARIState.frame, "channel.color");

	glClearColor(0.1f, 0.1f, 0.1f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, g_AppState.outputTexture);
	glTexSubImage2D(GL_TEXTURE_2D,
		0,
		0,
		0,
		fb.width,
		fb.height,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		fb.data);

	anari::unmap(g_ANARIState.device, g_ANARIState.frame, "channel.color");

	// draw to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(g_AppState.screenShader);
	glBindVertexArray(g_AppState.quadVAO);
	glBindTexture(GL_TEXTURE_2D, g_AppState.outputTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glfwSwapBuffers(g_AppState.nativeWindow);
}

static void terminate_ALL() {
	{ // GLFW
		glfwDestroyWindow(g_AppState.nativeWindow);
		glfwTerminate();
	}
}

int ENTRY_POINT() {
	initialize_ALL();

	while (window_condition()) {
		update();
	}

	terminate_ALL();
	return 0;
}