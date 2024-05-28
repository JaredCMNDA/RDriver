#include <iostream>
#include <Windows.h>
#include "memory.h"

namespace memory {
	DWORD get_process_id(const wchar_t* process_name) {
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
	std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name) {
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
}


bool driver::attach_to_process(HANDLE driver_handle, const DWORD pid) {
	driver::Request req = {};
	// PsLookupProcessByProcessId is a kernel function that retrieves the process object from the process id but requires a handle to the process so we must reinterpret
	req.process_id = reinterpret_cast<HANDLE>(pid);

	return DeviceIoControl(driver_handle, driver::codes::attach, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
}