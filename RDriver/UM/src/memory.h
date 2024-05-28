#pragma once
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

namespace memory {
	DWORD get_process_id(const wchar_t* process_name);
	std::uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name);
}

namespace driver {
	namespace codes {
		inline constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); // Attach to process
		inline constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); // Read proc memory
		inline constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS); // Write proc memory
	} // namespace codes

	struct Request {
		HANDLE process_id;

		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T return_size;
	};

	bool attach_to_process(HANDLE driver_handle, const DWORD pid);

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