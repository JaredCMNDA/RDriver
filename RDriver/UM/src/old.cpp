#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#include "client.dll.hpp"
#include "buttons.hpp"
#include "offsets.hpp"
#include "memory.h"

using namespace cs2_dumper::offsets;
using namespace cs2_dumper::schemas;
using namespace cs2_dumper;

using namespace memory;

int main() {

	const DWORD pid = get_process_id(L"cs2.exe");// L in front of string means it's a wide string
	if (pid == 0) {
		std::cout << "[-] Failed to get process id." << std::endl;
		std::cin.get(); // Pause
		return 1;
	}

	const HANDLE driver = CreateFile(L"\\\\.\\RDriver", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (driver == INVALID_HANDLE_VALUE) {
		std::cout << "[-] Failed to get driver handle." << std::endl;
		std::cin.get(); // Pause
		return 1;
	}

	if (driver::attach_to_process(driver, pid) == true) {
		std::cout << "[+] Successfully attached to process." << std::endl;

		if (const std::uintptr_t client = get_module_base(pid, L"client.dll"); client != 0) {
			std::cout << "[+] Found client.dll base address." << std::endl;

			while (true) {
				if (GetAsyncKeyState(VK_END)) {
					break;
				}


				const auto local_player_pawn = driver::read_memory<std::uintptr_t>(driver, client + client_dlloffset::dwLocalPlayerPawn);

				if (local_player_pawn == 0) {
					continue; // Keep trying
				}

				const auto flags = driver::read_memory<std::uint32_t>(driver, local_player_pawn + client_dll::C_BaseEntity::m_fFlags);

				const bool in_air = flags & (1 << 0); // 1 << 0 is the first bit in the flags
				const bool space_pressed = GetAsyncKeyState(VK_SPACE);
				const auto force_jump = driver::read_memory<DWORD>(driver, client + 181806780);

				if (space_pressed && in_air) {
					Sleep(5);
					driver::write_memory(driver, client + 181806780, 65537);
				}
				else if (space_pressed && !in_air) {
					driver::write_memory(driver, client + 181806780, 256);
				}
				else if (!space_pressed && force_jump == 65537) {
					driver::write_memory(driver, client + 181806780, 256);
				}
			}
		}
	}


	CloseHandle(driver);

	std::cin.get();	// Pause
	return 0;
}