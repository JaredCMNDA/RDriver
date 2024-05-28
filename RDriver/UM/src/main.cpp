
#include <GL/glew.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "controlMenu.h" // Header with menu functions
#include <Windows.h>
#include <iostream>
#include "memory.h" // Interact w/ driver

#include "../offsets/client.dll.hpp"
#include "../offsets/offsets.hpp"
#include "../offsets/buttons.hpp"
#include "../math/worldToScreen.h"
#include "../math/dataTypes.h"

using namespace driver;

using namespace cs2_dumper::offsets::client_dlloffset; // dwLocalPlayerPawn .. .. . . . 
using namespace cs2_dumper::schemas::client_dll; // C_BaseEntity -> .. .. 
using namespace cs2_dumper; // buttons

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";


	// Get the primary monitor's resolution
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
		std::cout << "[-] Failed to get primary monitor" << std::endl;
        return 1;
    }

	int width = glfwGetVideoMode(monitor)->width;
	int height = glfwGetVideoMode(monitor)->height;

    glfwWindowHint(GLFW_FLOATING, true);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_MAXIMIZED, true);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(width, height, "IMGUI", nullptr, nullptr);
    if (window == nullptr) {
        return 1;
    }

	glfwSetWindowAttrib(window, GLFW_DECORATED, false);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool bMenuVisible = true;

	// Get process id
    const DWORD pid = memory::get_process_id(L"cs2.exe");// L in front of string means it's a wide string
    if (pid == 0) {
        std::cout << "[-] Failed to get process id." << std::endl;
        std::cin.get(); // Pause
        return 1;
    }

	// Get driver handle
    const HANDLE driver = CreateFile(L"\\\\.\\RDriver", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (driver == INVALID_HANDLE_VALUE) {
        std::cout << "[-] Failed to get driver handle." << std::endl;
        std::cin.get(); // Pause
        return 1;
    }

    if (driver::attach_to_process(driver, pid) == true) {
        std::cout << "[+] Successfully attached to process." << std::endl;
        if (const std::uintptr_t client = memory::get_module_base(pid, L"client.dll"); client != 0) {
            std::cout << "[+] Found client.dll base address." << std::endl;

			// If we have the client.dll base address and the driver handle, we can start our loop

            Matrix viewMatrix;
            uintptr_t localPlayerPawn;

            do {
                localPlayerPawn = driver::read_memory<std::uintptr_t>(driver, client + dwLocalPlayerPawn);
            } while (localPlayerPawn == 0);
            Vec2 screenCoords;
            Vec2 lineOrigin;
            lineOrigin.X = 0;
            lineOrigin.Y = -1.0f;

            while (!glfwWindowShouldClose(window))
            {




                glfwPollEvents();
                // Start the Dear ImGui frame
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                if (GetAsyncKeyState(VK_INSERT) & 1) {
                    bMenuVisible = !bMenuVisible;
                    if (bMenuVisible) {
                        ShowMenu(window);
                    }
                    else {
                        HideMenu(window);
                    }
                }


                viewMatrix = driver::read_memory<Matrix>(driver, client + dwViewMatrix);

                // Get entity list
                uintptr_t entityList = driver::read_memory<uintptr_t>(driver, client + dwEntityList);
                // Get first entry in list
                uintptr_t localListEntry = driver::read_memory<uintptr_t>(driver, entityList + 0x10);
                // Loop through 64
                for (int i = 0; i < 64; i++) {
                    uintptr_t listEntry = driver::read_memory<uintptr_t>(driver, entityList + (8 * (i & 0x7FFF) >> 9) + 16);
                    if (listEntry == 0) {
                        continue;
                    }
                    // Get current controller
                    uintptr_t playerEntity = driver::read_memory<uintptr_t>(driver, listEntry + 120 * (i & 0x1FF));
                    if (!playerEntity) {
                        continue;
                    }
                    // Get pawn
                    uintptr_t playerPawn = driver::read_memory<uintptr_t>(driver, playerEntity + CCSPlayerController::m_hPlayerPawn);

                    // listEntry 2
                    uintptr_t listEntry2 = driver::read_memory<uintptr_t>(driver, entityList + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);
                    if (!listEntry2) {
                        continue;
                    }

                    uintptr_t CSPlayerPawn = driver::read_memory<uintptr_t>(driver, listEntry2 + 120 * (playerPawn & 0x1FF));

                    // Origin
                    Vec3 origin = driver::read_memory<Vec3>(driver, CSPlayerPawn + C_BasePlayerPawn::m_vOldOrigin);

                    std::cout << "X: " << origin.X << " Y: " << origin.Y << " Z: " << origin.Z << std::endl;

                    if (!WorldToScreen(origin, screenCoords, viewMatrix.vMatrix)) {
                        continue;
                    }


                    glBegin(GL_LINES);
                    glVertex2f(lineOrigin.X, lineOrigin.Y);
                    glVertex2f(screenCoords.X, screenCoords.Y);
                    glEnd();
                }


                // draw here 
                if (bMenuVisible) {
                    ImGui::Button("Test button");
                }

                // Rendering
                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                glfwSwapBuffers(window);
            }
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
