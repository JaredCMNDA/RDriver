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