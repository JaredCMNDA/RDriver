#pragma once
#include <GL/glew.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <Windows.h>
#include <iostream>


void ShowMenu(GLFWwindow* window);
void HideMenu(GLFWwindow* window);
bool CloseProgram(HANDLE driver, GLFWwindow* window);