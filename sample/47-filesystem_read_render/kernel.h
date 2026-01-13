//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _kernel_h
#define _kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/2dgraphics.h>
#include <circle/types.h>
#include "graphicshape.h"
#include "./cube.h"
#include <circle/usb/usbhcidevice.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

#define MAX_VERTS        32768
#define MAX_FACES        32768
#define MAX_FACE_VERTS   8
#define OBJ_LINE_MAX     128

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

	void Clear (void);
	Point2D Screen (const Vector3D &p);
	Vector3D Project (const Vector3D &p);
	Vector3D TranslateZ (const Vector3D &v, float dz);
	Vector3D RotateXZ (const Vector3D &v, float angle);
	void DrawLine (int x1, int y1, int x2, int y2);

private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	C2DGraphics		m_2DGraphics;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;
	CUSBHCIDevice		m_USBHCI;
	CEMMCDevice		m_EMMC;
	FATFS			m_FileSystem;
};

#endif
