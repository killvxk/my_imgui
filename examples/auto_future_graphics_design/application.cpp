#include "application.h"

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#if !defined(IMGUI_WAYLAND)
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#else
#include "../../deps/glad/glad.h"
#endif
#include <GLFW/glfw3.h>
#include "SOIL.h"
#include "texture.h"
#include "res_output.h"
#include "ft_image.h"
#include <functional>

#include "Resource.h"
#include "afb_load.h"
#include "primitive_object.h"
//extern void instantiating_internal_shader();
static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

namespace auto_future
{
	application::application(int argc, char **argv)
	{
		if (argc > 1)
		{
			_cureent_project_file_path = argv[1];//afb
		}
		if (argc > 3)
		{
			_screen_width = atoi(argv[2]);
			_screen_height = atoi(argv[3]);
		}
	}


	application::~application()
	{
	}

	bool application::prepare()
	{
		glfwSetErrorCallback(error_callback);
		if (!glfwInit())
			return false;
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
		if (!_screen_width || !_screen_height)
		{
			GLFWmonitor*  pmornitor = glfwGetPrimaryMonitor();
			const GLFWvidmode * mode = glfwGetVideoMode(pmornitor);
			_screen_width = mode->width;
			_screen_height = mode->height;
		}
		//instantiating_internal_shader();
		
		//ImVec2 edit_window_size = ImVec2()
		

		return true;
	}

	bool application::create_run()
	{
		GLFWwindow* _window = { NULL };
#if !defined(IMGUI_WAYLAND)
		_window = glfwCreateWindow(_screen_width, _screen_height, "Graphics app", NULL, NULL);
#else
		_window = glfwCreateWindow(_screen_width, _screen_height, "Graphics app", glfwGetPrimaryMonitor(), NULL);
#endif
		glfwMakeContextCurrent(_window);
		glfwSwapInterval(1); // Enable vsync
		gl3wInit();

		// Setup ImGui binding
		ImGui::CreateContext();
		//ImGuiIO& io = ImGui::GetIO(); //(void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
		ImGui_ImplGlfwGL3_Init(_window, true);
		init_internal_primitive_list();
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		base_ui_component* _proot = NULL;
		if (!_cureent_project_file_path.empty())
		{
			//_proot = new ft_base;
			afb_load afl(_proot);
			afl.load_afb(_cureent_project_file_path.c_str());
			resLoaded();
		}
		// Setup style
		//ImGui::StyleColorsLight();
		ImGui::StyleColorsClassic();
		while (!glfwWindowShouldClose(_window))
		{
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			glfwPollEvents();
			ImGui_ImplGlfwGL3_NewFrame();
			ImGui::SetNextWindowSize(ImVec2(1920, 720), ImGuiCond_FirstUseEver);
			static bool show_app = true;
			ImGui::Begin("edit window", &show_app, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			//
			preRender();
			if (_proot)
			{
				_proot->draw();
			}
			ImGui::End();
			// Rendering
			int display_w, display_h;
			glfwGetFramebufferSize(_window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui::Render();
			ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(_window);
		}
		return true;
	}


	void application::destroy()
	{
		ImGui_ImplGlfwGL3_Shutdown();
		ImGui::DestroyContext();
		glfwTerminate();
	}
}