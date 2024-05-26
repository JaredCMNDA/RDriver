#include <ntifs.h>

extern "C" { // Can call undocumented function (not included in headers)
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE DriverInit);
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
}

void debug_print(PCSTR text) {
#ifndef DEBUG // If not in debug mode, remove the debug print
	UNREFERENCED_PARAMETER(text); // Prevents "unreferenced parameter" warning
#endif
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, text));
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

	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status; // Return status of the function that called this function
	}
	
	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status; // Return status of the function that called this function
	}

	// Todo
	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		debug_print("[+] Device control called.\n");

		NTSTATUS status = STATUS_UNSUCCESSFUL;

		PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp); // Get the current stack location of the irp. Need to determine which ioctl code was sent

		// Access request object from user space
		auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer); // Get the system buffer from the irp and cast it to a request object

		if (stack_irp == nullptr || request == nullptr) {
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		// Target process we want access to
		static PEPROCESS target_process = nullptr;

		const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode; // Get the control code from the stack irp

		switch (control_code) {
			case codes::attach:
				status = PsLookupProcessByProcessId(request->process_id, &target_process); // Get the process object from the process id
				break;
			case codes::read:
				if (target_process != nullptr)
					status = MmCopyVirtualMemory(target_process, request->target, PsGetCurrentProcess(), request->buffer, request->size, KernelMode, &request->return_size);
				break;
			case codes::write:
				if (target_process != nullptr)
					status = MmCopyVirtualMemory(PsGetCurrentProcess(), request->buffer, target_process, request->target, request->size, KernelMode, &request->return_size);
				break;
			default:
				break;
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(request);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return status;
	}

} // namespace driver

NTSTATUS DriverMain(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L"\\Device\\RDriver");

	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object); 

	if (status != STATUS_SUCCESS) {
		debug_print("[-] Failed to create driver device.\n");
		return status;
	}

	debug_print("[+] Driver device successfully created.\n");

	UNICODE_STRING symbolic_link = {};
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\RDriver");

	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (status != STATUS_SUCCESS) {
		debug_print("[-] Failed to establish symbolic link.\n");
		return status;
	}

	debug_print("[+] Driver symbolic link successfully established.\n");

	// Send small amounts of data between user and kernel space
	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	// Set the driver handlers to our functions with our own logic
	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create; // Within driver obj there is a major function array. We must set the create function to the create function we defined
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close; // Within driver obj there is a major function array. We must set the close function to the close function we defined
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control; // Within driver obj there is a major function array. We must set the device_control function to the device_control function we defined

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING); // Clear the flag to indicate that the device is ready to be used

	debug_print("[+] Driver initialized.\n");


	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry() {
	// For IOCTL communication we need driver object, but we are manually mapping so we do not have this driver object. DriverMain is the real entry point

	debug_print("[+] Hello from kernel.\n");

	UNICODE_STRING driver_name = {}; // When func wants a string, it wants a unicode string so we must call RtlInitUnicodeString
	RtlInitUnicodeString(&driver_name, L"\\Driver\\RDriver"); 

	return IoCreateDriver(&driver_name, &DriverMain);

}