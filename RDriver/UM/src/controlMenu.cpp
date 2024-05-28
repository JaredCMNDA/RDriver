#include "controlMenu.h"

void ShowMenu(GLFWwindow* window) {
	std::cout << "[+] Showing menu" << std::endl;
	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, false);
}

void HideMenu(GLFWwindow* window) {
	std::cout << "[+] Hiding menu" << std::endl;
	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, true);
}

bool CloseProgram(HANDLE driver, GLFWwindow* window) {
	return true;
}

