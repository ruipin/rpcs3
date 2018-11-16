﻿#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "sys_usbd.h"
#include "sys_ppu_thread.h"


LOG_CHANNEL(sys_usbd);

std::vector<usbDevice> devices = {
	// System devices
	usbDevice{ DeviceListUnknownDataType{0x1, 0x2, 0x2, 0x44}, 50, deviceDescriptor{0x12, 0x1, 0x0200, 0, 0, 0,			0x40, 0x054C, 0x0250, 0x0009, 3, 4, 5, 1}},
	usbDevice{ DeviceListUnknownDataType{0x1, 0x3, 0x2, 0xAD}, 326, deviceDescriptor{0x12, 0x1, 0x0200, 0xE0, 0x1, 0x1,	0x40, 0x054C, 0x0267, 0x0001, 1, 2, 0, 1}},
	// USB Drive
	usbDevice{ DeviceListUnknownDataType{0x0, 0x4, 0x2, 0x0}, 50, deviceDescriptor{0x12, 0x1, 0x0200, 0, 0, 0,			0x40, 0x1516, 0x1226, 0x0100, 1, 2, 3, 1}},
	// Skylanders Portal
	usbDevice{ DeviceListUnknownDataType{0x0, 0x5, 0x2, 0xF8}, 59, deviceDescriptor{0x12, 0x1, 0x0200, 0, 0, 0,			0x20, 0x1430, 0x0150, 0x0100, 1, 2, 0, 1}},
};

bool has_register_extra_ldd;
int receive_event_called_count;

/*
 * sys_usbd_initialize changes handle to a semi-unique identifier.
 * identifier generation is speculated to be:
 * f(n+1) = (f(n) + 2<<9) %  14847 + f_min (probably just f(0)-14847 but it could be different)
 * Thanks to @flash-fire for thinking up the identifier generation code.
 * TODO: try to get hardware to return not CELL_OK (perhaps set handle to NULL).
 */
s32 sys_usbd_initialize(vm::ptr<u32> handle)
{
	sys_usbd.warning("sys_usbd_initialize(0x%x)", handle.get_ptr());
	*handle = 805322496;
	has_register_extra_ldd = false;
	receive_event_called_count = 0;
	return CELL_OK;
}

s32 sys_usbd_finalize()
{
	sys_usbd.todo("sys_usbd_finalize()");
	return CELL_OK;
}

s32 sys_usbd_get_device_list(u32 handle, vm::ptr<DeviceListUnknownDataType> device_list, char unknown)
{
	sys_usbd.warning("sys_usbd_get_device_list(handle=%d, device_list=0x%x, unknown=%c)", handle, device_list, unknown);
	// Unknown is just 0x7F
	for (int i = 0; i < devices.size(); i++)
	{
		device_list[i] = devices[i].basicDevice;
	}
	return devices.size();
}

s32 sys_usbd_register_extra_ldd(u32 handle, vm::ptr<void> lddOps, u16 strLen, u16 vendorID, u16 productID, u16 unk1)
{
	sys_usbd.warning("sys_usbd_register_extra_ldd(handle=%u, lddOps=0x%x, unk1=%u, vendorID=%u, productID=%u, unk2=%u)", handle, lddOps, strLen, vendorID, productID, unk1);
	has_register_extra_ldd = true;

	// No idea what 9 means.
	return 9;
}

s32 sys_usbd_get_descriptor_size(u32 handle, u8 deviceNumber)
{
	sys_usbd.warning("sys_usbd_get_descriptor_size(handle=%u, deviceNumber=%u)", handle, deviceNumber);
	return devices[deviceNumber-2].descSize;
}

s32 sys_usbd_get_descriptor(u32 handle, u8 deviceNumber, vm::ptr<deviceDescriptor> descriptor, s64 descSize)
{
	sys_usbd.warning("sys_usbd_get_descriptor(handle=%u, deviceNumber=%u, descriptor=0x%x, descSize=%u)", handle, deviceNumber, descriptor, descSize);

	// Just gonna have to hack it for now

	u8 device = deviceNumber-2;
	unsigned char* desc = (unsigned char*)descriptor.get_ptr();
	if (device == 0)
	{
		unsigned char bytes[] = {0x12, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x40, 0x4c, 0x5, 0x50, 0x2, 0x9, 0x0, 0x3, 0x4, 0x5, 0x1,
								 0x9, 0x2, 0x20, 0x0, 0x1, 0x1, 0x0, 0x80, 0xfa,
								 0x9, 0x4, 0x0, 0x0, 0x2, 0x8, 0x5, 0x50, 0x0,
								 0x7, 0x5, 0x81, 0x2, 0x0, 0x2, 0x0,
								 0x7, 0x5, 0x2, 0x2, 0x0, 0x2, 0x0};
		memcpy(desc, bytes, descSize);
	}
	else if (device == 1)
	{
		unsigned char bytes[] = {0x12, 0x1, 0x0, 0x2, 0xe0, 0x1, 0x1, 0x40, 0x4c, 0x5, 0x67, 0x2, 0x0, 0x1, 0x1, 0x2, 0x0, 0x1,
								 0x9, 0x2, 0x34, 0x1, 0x4, 0x1, 0x0, 0xc0, 0x0,
								 0x9, 0x4, 0x0, 0x0, 0x3, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x81, 0x3, 0x40, 0x0, 0x1,
								 0x7, 0x5, 0x2, 0x2, 0x0, 0x2, 0x0,
								 0x7, 0x5, 0x82, 0x2, 0x0, 0x2, 0x0,
								 0x9, 0x4, 0x1, 0x0, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0x0, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0x0, 0x0, 0x4,
								 0x9, 0x4, 0x1, 0x1, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0xb, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0xb, 0x0, 0x4,
								 0x9, 0x4, 0x1, 0x2, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0x13, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0x13, 0x0, 0x4,
								 0x9, 0x4, 0x1, 0x3, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0x1b, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0x1b, 0x0, 0x4,
								 0x9, 0x4, 0x1, 0x4, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0x23, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0x23, 0x0, 0x4,
								 0x9, 0x4, 0x1, 0x5, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0x33, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0x33, 0x0, 0x4,
								 0x9, 0x4, 0x1, 0x6, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x3, 0x1, 0x40, 0x0, 0x4,
								 0x7, 0x5, 0x83, 0x1, 0x40, 0x0, 0x4,
								 0x9, 0x4, 0x2, 0x0, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x4, 0x3, 0x40, 0x0, 0x1,
								 0x7, 0x5, 0x85, 0x3, 0x40, 0x0, 0x1,
								 0x9, 0x4, 0x2, 0x1, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x4, 0x3, 0x80, 0x0, 0x1,
								 0x7, 0x5, 0x85, 0x3, 0x80, 0x0, 0x1,
								 0x9, 0x4, 0x2, 0x2, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x4, 0x3, 0x0, 0x1, 0x1,
								 0x7, 0x5, 0x85, 0x3, 0x0, 0x1, 0x1,
								 0x9, 0x4, 0x2, 0x3, 0x2, 0xe0, 0x1, 0x1, 0x0,
								 0x7, 0x5, 0x4, 0x3, 0x0, 0x4, 0x1,
								 0x7, 0x5, 0x85, 0x3, 0x0, 0x4, 0x1,
								 0x9, 0x4, 0x3, 0x0, 0x0, 0xfe, 0x1, 0x0, 0x0,
								 0x7, 0x21, 0x7, 0x88, 0x13, 0xff, 0x3};

		memcpy(desc, bytes, descSize);
	}
	else if (device == 2)
	{
		// USB Drive
		unsigned char bytes[] = {0x12, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x40, 0x16, 0x15, 0x26, 0x12, 0x0, 0x1, 0x1, 0x2, 0x3, 0x1,
								 0x9, 0x2, 0x20, 0x0, 0x1, 0x1, 0x0, 0x80, 0xfa,
								 0x9, 0x4, 0x0, 0x0, 0x2, 0x8, 0x6, 0x50, 0x0,
								 0x7, 0x5, 0x81, 0x2, 0x0, 0x2, 0x0,
								 0x7, 0x5, 0x2, 0x2, 0x0, 0x2, 0x0};
		memcpy(desc, bytes, descSize);
	}
	else if (device == 3 || device == 4)
	{
		// Skylanders Portal
		unsigned char bytes[] = {0x12, 0x1, 0x0, 0x2, 0x0, 0x0, 0x0, 0x20, 0x30, 0x14, 0x50, 0x1, 0x0, 0x1, 0x1, 0x2, 0x0, 0x1,
								 0x9, 0x2, 0x29, 0x0, 0x1, 0x1, 0x0, 0x80, 0x96,
								 0x9, 0x4, 0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0,
								 0x9, 0x21, 0x11, 0x1, 0x0, 0x1, 0x22, 0x1d, 0x0,
								 0x7, 0x5, 0x81, 0x3, 0x20, 0x0, 0x1,
								 0x7, 0x5, 0x1, 0x3, 0x20, 0x0, 0x1};
		memcpy(desc, bytes, descSize);
	}
	else
		return -1;
	return CELL_OK;
}

s32 sys_usbd_register_ldd()
{
	sys_usbd.todo("sys_usbd_register_ldd()");
	return CELL_OK;
}

s32 sys_usbd_unregister_ldd()
{
	sys_usbd.todo("sys_usbd_unregister_ldd()");
	return CELL_OK;
}

s32 sys_usbd_open_pipe(u32 handle, vm::ptr<void> endDisc)
{
	sys_usbd.todo("sys_usbd_open_pipe(handle=%d, endDisc=0x%x)", handle, endDisc);
	return CELL_OK;
}

s32 sys_usbd_open_default_pipe()
{
	sys_usbd.todo("sys_usbd_open_default_pipe()");
	return CELL_OK;
}

s32 sys_usbd_close_pipe()
{
	sys_usbd.todo("sys_usbd_close_pipe()");
	return CELL_OK;
}

s32 sys_usbd_receive_event(ppu_thread& ppu, u32 handle, vm::ptr<u64> arg1, vm::ptr<u64> arg2, vm::ptr<u64> arg3)
{
	sys_usbd.todo("sys_usbd_receive_event(handle=%u, arg1=0x%x, arg2=0x%x, arg3=0x%x)", handle, arg1, arg2, arg3);
	lv2_obj::sleep(ppu);
	thread_ctrl::wait();
	//_sys_ppu_thread_exit(ppu, CELL_OK);

	// TODO
	/*if (receive_event_called_count == 0)
	{
		*arg1 = 2;
		*arg2 = 5;
		*arg3 = 0;
		receive_event_called_count++;
	}
	else if (receive_event_called_count == 1)
	{
		*arg1 = 1;
		*arg2 = 6;
		*arg3 = 0;
		receive_event_called_count++;
	}
	else
	{
		_sys_ppu_thread_exit(ppu, CELL_OK);
	}*/
	return CELL_OK;
}

s32 sys_usbd_detect_event()
{
	sys_usbd.todo("sys_usbd_detect_event()");
	return CELL_OK;
}

s32 sys_usbd_attach()
{
	sys_usbd.todo("sys_usbd_attach()");
	return CELL_OK;
}

s32 sys_usbd_transfer_data()
{
	sys_usbd.todo("sys_usbd_transfer_data()");
	return CELL_OK;
}

s32 sys_usbd_isochronous_transfer_data()
{
	sys_usbd.todo("sys_usbd_isochronous_transfer_data()");
	return CELL_OK;
}

s32 sys_usbd_get_transfer_status()
{
	sys_usbd.todo("sys_usbd_get_transfer_status()");
	return CELL_OK;
}

s32 sys_usbd_get_isochronous_transfer_status()
{
	sys_usbd.todo("sys_usbd_get_isochronous_transfer_status()");
	return CELL_OK;
}

s32 sys_usbd_get_device_location()
{
	sys_usbd.todo("sys_usbd_get_device_location()");
	return CELL_OK;
}

s32 sys_usbd_send_event()
{
	sys_usbd.todo("sys_usbd_send_event()");
	return CELL_OK;
}

s32 sys_usbd_event_port_send()
{
	sys_usbd.todo("sys_usbd_event_port_send()");
	return CELL_OK;
}

s32 sys_usbd_allocate_memory()
{
	sys_usbd.todo("sys_usbd_allocate_memory()");
	return CELL_OK;
}

s32 sys_usbd_free_memory()
{
	sys_usbd.todo("sys_usbd_free_memory()");
	return CELL_OK;
}

s32 sys_usbd_get_device_speed()
{
	sys_usbd.todo("sys_usbd_get_device_speed()");
	return CELL_OK;
}
