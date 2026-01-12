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

unsigned screen_width = 800;
unsigned screen_height = 800;


CKernel::CKernel (void)
:
m_2DGraphics (800, 800, TRUE)
{
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	return m_2DGraphics.Initialize ();
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
	{
		// Behind camera, return invalid point
		return Vector3D (0.0f, 0.0f, 0.0f);
	}
	
	return Vector3D (p.x / p.z, p.y / p.z, p.z);
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
	const float FPS = 60.0f;
	const float dt = 1.0f / FPS;
	const unsigned frame_delay_ms = (unsigned) (1000.0f / FPS);
	
	float angle = 0.0f;
	float dz = 1.0f;
	//m_pShape = new CGraphicShape (&m_2DGraphics);
	
	while (1)
	{
		angle += M_PI * dt;
		
		m_2DGraphics.ClearScreen(CDisplay::Black);
		
		// Draw each face
		for (unsigned f = 0; f < num_faces; f++)
		{
			unsigned face_size = fs_sizes[f];
			
			for (unsigned i = 0; i < face_size; i++)
			{
				unsigned idx_a = fs[f][i];
				unsigned idx_b = fs[f][(i + 1) % face_size];
				
				Vector3D a = vs[idx_a];
				Vector3D b = vs[idx_b];
				
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
					//m_2DGraphics.DrawText(400, 0, CDisplay::White, "Hello Circle! %d", C2DGraphics::AlignCenter);
					//DrawLine(screen_a.x, screen_a.y, screen_b.x, screen_b.y);
				}
			}
		}
		m_2DGraphics.UpdateDisplay();
		CTimer::SimpleMsDelay (frame_delay_ms);
	}

	return ShutdownHalt;
}
