#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#include "client.dll.hpp"
#include "buttons.hpp"
#include "offsets.hpp"

using namespace cs2_dumper::offsets;
using namespace cs2_dumper::schemas;
using namespace cs2_dumper;

static DWORD get_process_id(const wchar_t* process_name) {
	DWORD process_id = 0;

	HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snap_shot == INVALID_HANDLE_VALUE) {
		return process_id;
	}

	PROCESSENTRY32 entry = {};
	entry.dwSize = sizeof(decltype(entry));

	if (Process32FirstW(snap_shot, &entry) == TRUE) {
		// Iterate through all processes, check if first process name matches the process name we want
		if (_wcsicmp(process_name, entry.szExeFile) == 0) {
			process_id = entry.th32ProcessID;
		}
		else {
			while (Process32NextW(snap_shot, &entry) == TRUE) {
				if (_wcsicmp(process_name, entry.szExeFile) == 0) {
					process_id = entry.th32ProcessID;
					break;
				}
			}
		}
	}
	CloseHandle(snap_shot);
	return process_id;
}

static std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name) {
	std::uintptr_t module_base = 0;

	// Create a snapshot of all modules in the process (dlls)
	HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap_shot == INVALID_HANDLE_VALUE) {
		return module_base;
	}

	MODULEENTRY32 entry = {};
	entry.dwSize = sizeof(decltype(entry));

	if (Module32FirstW(snap_shot, &entry) == TRUE) {
		// Check if the first module name matches the module name we want
		if (wcsstr(module_name, entry.szModule) != nullptr) {
			module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
		}
		else {
			while (Module32NextW(snap_shot, &entry) == TRUE) {
				if (wcsstr(module_name, entry.szModule) != nullptr) {
					module_base = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
					break;
				}
			}
		}
	}
	
	CloseHandle(snap_shot);

	return module_base;
}

namespace driver {
	namespace codes {
		constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); // Attach to process
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); // Read proc memory
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); // Write proc memory
	} // namespace codes

	struct Request {
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	bool attach_to_process(HANDLE driver_handle, const DWORD pid) {
		Request req = {};
		// PsLookupProcessByProcessId is a kernel function that retrieves the process object from the process id but requires a handle to the process so we must reinterpret
		req.process_id = reinterpret_cast<HANDLE>(pid); 

		return DeviceIoControl(driver_handle, codes::attach, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
	}

	template <class T>
	T read_memory(HANDLE driver_handle, const std::uintptr_t addr) {
		T temp = {};

		Request req;
		req.target = reinterpret_cast<PVOID>(addr); // Address we want to read from
		req.buffer = &temp; // Buffer to store the read data
		req.size = sizeof(T);

		DeviceIoControl(driver_handle, codes::read, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);

		return temp;
	}

	template <class T>
	void write_memory(HANDLE driver_handle, const std::uintptr_t addr, const T& data) {
		Request req;
		req.target = reinterpret_cast<PVOID>(addr); // Address we want to write to
		req.buffer = (PVOID)&data; // Data we want to write
		req.size = sizeof(T);

		DeviceIoControl(driver_handle, codes::write, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
	}

} // namespace driver

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