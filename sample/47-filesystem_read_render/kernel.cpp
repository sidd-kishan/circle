//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/timer.h>
#include <circle/types.h>
#include <math.h>
#include <stdlib.h>
#include <circle/string.h>

//#define DRIVE		"SD:"
#define DRIVE		"USB:"
//#define DRIVE		"FD:"

#define FILENAME	"/circle.txt"

static const char FromKernel[] = "kernel";


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// Colors
#if DEPTH == 16
static const TScreenColor BACKGROUND_COLOR = COLOR16 (16 >> 3, 16 >> 3, 16 >> 3);  // #101010
static const TScreenColor FOREGROUND_COLOR = COLOR16 (80 >> 3, 255 >> 3, 80 >> 3); // #50FF50
#elif DEPTH == 32
static const TScreenColor BACKGROUND_COLOR = COLOR32 (16, 16, 16, 255);  // #101010
static const TScreenColor FOREGROUND_COLOR = COLOR32 (80, 255, 80, 255);  // #50FF50
#else
static const TScreenColor BACKGROUND_COLOR = BLACK_COLOR;
static const TScreenColor FOREGROUND_COLOR = BRIGHT_GREEN_COLOR;
#endif

unsigned screen_width = 1080;
unsigned screen_height = 960;



Vector3D dyn_vs[MAX_VERTS];
unsigned dyn_fs[MAX_FACES][MAX_FACE_VERTS];
unsigned dyn_fs_sizes[MAX_FACES];

unsigned dyn_num_vs    = 0;
unsigned dyn_num_faces = 0;

static char* skip_ws(char* p)
{
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static float parse_float(char** p)
{
    float sign = 1.0f;
    float val = 0.0f;
    float frac = 0.0f;
    float base = 0.1f;

    char* s = *p;
    if (*s == '-') { sign = -1.0f; s++; }

    while (*s >= '0' && *s <= '9') {
        val = val * 10.0f + (*s++ - '0');
    }

    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            frac += (*s++ - '0') * base;
            base *= 0.1f;
        }
    }

    *p = s;
    return sign * (val + frac);
}

static int parse_index(char** p)
{
    int v = 0;
    char* s = *p;

    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s++ - '0');
    }

    // Skip /vt/vn if present
    while (*s && *s != ' ' && *s != '\t')
        s++;

    *p = s;
    return v - 1;  // OBJ is 1-based
}

int LoadOBJ(const char* path)
{
    FIL file;
    FRESULT res;
    UINT br;
    char line[OBJ_LINE_MAX];

    dyn_num_vs    = 0;
    dyn_num_faces = 0;

    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
        return -1;

    while (f_gets(line, sizeof(line), &file))
    {
        char* p = skip_ws(line);

        /* Vertex */
        if (p[0] == 'v' && p[1] == ' ')
        {
            if (dyn_num_vs >= MAX_VERTS)
                continue;

            p += 2;
            dyn_vs[dyn_num_vs].x = parse_float(&p);
            p = skip_ws(p);
            dyn_vs[dyn_num_vs].y = parse_float(&p);
            p = skip_ws(p);
            dyn_vs[dyn_num_vs].z = parse_float(&p);

            dyn_num_vs++;
        }

        /* Face / polyline */
        else if (p[0] == 'f' && p[1] == ' ')
        {
            if (dyn_num_faces >= MAX_FACES)
                continue;

            p += 2;
            unsigned count = 0;

            while (*p && count < MAX_FACE_VERTS)
            {
                p = skip_ws(p);
                if (*p < '0' || *p > '9')
                    break;

                int idx = parse_index(&p);
                if (idx >= 0 && idx < (int)dyn_num_vs)
                {
                    dyn_fs[dyn_num_faces][count++] = idx;
                }
            }

            if (count >= 2)
            {
                dyn_fs_sizes[dyn_num_faces] = count;
                dyn_num_faces++;
            }
        }
    }

    f_close(&file);
    return dyn_num_faces;
}

CKernel::CKernel (void)
:
m_2DGraphics (screen_width, screen_height, TRUE),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED)
{
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_2DGraphics.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	return bOK;
}


// Convert normalized coordinates (-1..1) to screen coordinates
Point2D CKernel::Screen (const Vector3D &p)
{
	
	unsigned width = screen_width;
	unsigned height = screen_height;
	
	// -1..1 => 0..2 => 0..1 => 0..width
	int x = (int) ((p.x + 1.0f) / 2.0f * width);
	// Flip Y: -1..1 => 0..2 => 0..1 => 0..height, then flip
	int y = (int) ((1.0f - (p.y + 1.0f) / 2.0f) * height);
	
	return Point2D (x, y);
}

// Project 3D point to 2D (perspective projection)
Vector3D CKernel::Project (const Vector3D &p)
{
    if (p.z <= 0.0f)
        return Vector3D(0, 0, 0);

    const float fov = 1.0f;   // try 0.6 – 1.2

    return Vector3D(
        (p.x / p.z) * fov,
        (p.y / p.z) * fov,
        p.z
    );
}

void NormalizeModel(void)
{
    Vector3D min = dyn_vs[0];
    Vector3D max = dyn_vs[0];

    for (unsigned i = 1; i < dyn_num_vs; i++) {
        Vector3D v = dyn_vs[i];

        if (v.x < min.x) min.x = v.x;
        if (v.y < min.y) min.y = v.y;
        if (v.z < min.z) min.z = v.z;

        if (v.x > max.x) max.x = v.x;
        if (v.y > max.y) max.y = v.y;
        if (v.z > max.z) max.z = v.z;
    }

    Vector3D center = {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    };

    float extent_x = max.x - min.x;
    float extent_y = max.y - min.y;
    float extent = (extent_x > extent_y) ? extent_x : extent_y;

    float scale = 1.0f / extent;

    for (unsigned i = 0; i < dyn_num_vs; i++) {
        dyn_vs[i].x = (dyn_vs[i].x - center.x) * scale;
        dyn_vs[i].y = (dyn_vs[i].y - center.y) * scale;
        dyn_vs[i].z = (dyn_vs[i].z - center.z) * scale;
    }
}


// Translate along Z axis
Vector3D CKernel::TranslateZ (const Vector3D &v, float dz)
{
	return Vector3D (v.x, v.y, v.z + dz);
}

// Rotate around Y axis (XZ plane rotation)
Vector3D CKernel::RotateXZ (const Vector3D &v, float angle)
{
	float c = cosf (angle);
	float s = sinf (angle);
	
	return Vector3D (
		v.x * c - v.z * s,
		v.y,
		v.x * s + v.z * c
	);
}

void CKernel::DrawLine (int x1, int y1, int x2, int y2)
{
	int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
	int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;
	int err = dx - dy;
	
	unsigned width = screen_width; //m_Screen.GetWidth ();
	unsigned height = screen_height; //m_Screen.GetHeight ();
	
	int x = x1;
	int y = y1;
	
	while (1)
	{
		// Only draw if within screen bounds
		if (x >= 0 && (unsigned) x < width && y >= 0 && (unsigned) y < height)
		{
			m_2DGraphics.DrawPixel ((unsigned) x, (unsigned) y, CDisplay::Green);
		}
		
		if (x == x2 && y == y2)
			break;
			
		int e2 = 2 * err;
		
		if (e2 > -dy)
		{
			err -= dy;
			x += sx;
		}
		
		if (e2 < dx)
		{
			err += dx;
			y += sy;
		}
	}
}

TShutdownMode CKernel::Run (void)
{
	// Mount file system
	CString message;
	message.Append("msg");
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		message.Format ("Cannot mount drive: %s", DRIVE);
	}
	// Show contents of root directory

	const float FPS = 60.0f;
	const float dt = 1.0f / FPS;
	const unsigned frame_delay_ms = (unsigned) (1000.0f / FPS);
	
	float angle = 0.0f;
	float dz = 1.0f;
	//m_pShape = new CGraphicShape (&m_2DGraphics);
	CString FileName;
	int result_of_load = LoadOBJ("USB:/tree.obj");
	NormalizeModel();
	while (1)
	{
		angle += M_PI * dt;
		
		m_2DGraphics.ClearScreen(CDisplay::Black);
		/*
		m_2DGraphics.DrawText(400, 0, CDisplay::White,(const char *) message, C2DGraphics::AlignCenter);
		FileName.Format ("total faces loaded: %d", result_of_load);
		m_2DGraphics.DrawText(400, 12, CDisplay::White,(const char *) FileName, C2DGraphics::AlignCenter);

		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
		for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
		{
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
			{
				FileName.Format ("%-20s", FileInfo.fname);

				//m_Screen.Write ((const char *) FileName, FileName.GetLength ());
				m_2DGraphics.DrawText(400, (i+2)*12, CDisplay::White,(const char *) FileName, C2DGraphics::AlignCenter);

				if (i % 4 == 3)
				{
					//m_Screen.Write ("\n", 1);
				}
			}

			Result = f_findnext (&Directory, &FileInfo);
		}
		*/
		//m_Screen.Write ("\n", 1);
		// Draw each face
		for (unsigned f = 0; f < dyn_num_faces; f++)
		{
			unsigned face_size = dyn_fs_sizes[f];
			
			for (unsigned i = 0; i < face_size; i++)
			{
				unsigned idx_a = dyn_fs[f][i];
				unsigned idx_b = dyn_fs[f][(i + 1) % face_size];
				
				Vector3D a = dyn_vs[idx_a];
				Vector3D b = dyn_vs[idx_b];
				
				// Transform vertices
				a = RotateXZ (a, angle);
				b = RotateXZ (b, angle);
				a = TranslateZ (a, dz);
				b = TranslateZ (b, dz);
				
				// Project to 2D
				Vector3D proj_a = Project (a);
				Vector3D proj_b = Project (b);
				
				// Skip if behind camera
				if (proj_a.z > 0.0f && proj_b.z > 0.0f)
				{
					// Convert to screen coordinates and draw line
					Point2D screen_a = Screen (proj_a);
					Point2D screen_b = Screen (proj_b);
					m_2DGraphics.DrawLine(screen_a.x, screen_a.y, screen_b.x, screen_b.y, CDisplay::Green);
					//m_2DGraphics.DrawText(400, 0, CDisplay::White,(const char *) message, C2DGraphics::AlignCenter);
					//DrawLine(screen_a.x, screen_a.y, screen_b.x, screen_b.y);
				}
			}
		}
		m_2DGraphics.UpdateDisplay();
		CTimer::SimpleMsDelay (frame_delay_ms);
		//CTimer::SimpleMsDelay(1000);
	}

	return ShutdownHalt;
}
