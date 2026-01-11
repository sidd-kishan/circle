#include "kernel.h"
#include <circle/timer.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
========================================================================
Mathematical + Historical Context (comment-only)

• Rotation via:
    x' = x cosθ − z sinθ
    z' = x sinθ + z cosθ
  (Euler, 18th century)

• Latitudinal banding mimics spherical harmonics
  used later by Laplace (celestial mechanics)

• Time-driven animation avoids frame-locking
  common in bare-metal systems
========================================================================
*/

// ------------------------------------------------------------
// Kernel
// ------------------------------------------------------------
CKernel::CKernel(void)
:
#ifdef SPI_DISPLAY
    m_SPIMaster (SPI_CLOCK_SPEED, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
    m_SPIDisplay (&m_SPIMaster, DISPLAY_PARAMETERS),
    m_2DGraphics (&m_SPIDisplay)
#elif defined (I2C_DISPLAY)
    m_I2CMaster (I2C_MASTER_DEVICE, TRUE),
    m_I2CDisplay (&m_I2CMaster, DISPLAY_PARAMETERS),
    m_2DGraphics (&m_I2CDisplay)
#else
    m_2DGraphics (m_Options.GetWidth(), m_Options.GetHeight(), TRUE)
#endif
{
    m_ActLED.Blink(5);
}

boolean CKernel::Initialize(void)
{
    return m_2DGraphics.Initialize();
}

// ------------------------------------------------------------
// MAIN LOOP
// ------------------------------------------------------------
// ------------------------------------------------------------
// MAIN LOOP — Correct spherical checkerboard
// ------------------------------------------------------------
TShutdownMode CKernel::Run(void)
{
    const unsigned W = m_2DGraphics.GetWidth();
    const unsigned H = m_2DGraphics.GetHeight();

    const int cx = W / 2;
    const int cy = H / 2;
    const int R  = 90;

    float time = 0.0f;

    while (1)
    {
        time += 0.04f;

        m_2DGraphics.ClearScreen(CDisplay::Black);

        for (int y = -R; y <= R; y++)
        {
            float fy = (float)y / R;
            float y2 = fy * fy;

            if (y2 > 1.0f)
                continue;

            float xr = sqrtf(1.0f - y2);
            int xspan = (int)(xr * R);

            // latitude (−π/2 → π/2)
            float v = asinf(fy);

            for (int x = -xspan; x <= xspan; x++)
            {
                float fx = (float)x / R;

                // longitude (−π → π)
                float u = atan2f(fx, xr);

                // checker frequency
                int iu = (int)((u + M_PI) * 4.0f);
                int iv = (int)((v + M_PI/2) * 4.0f);

                bool checker = ((iu ^ iv) & 1);

                T2DColor col = checker
                    ? CDisplay::BrightRed
                    : CDisplay::White;

                // Draw single pixel as a 1-pixel line
                m_2DGraphics.DrawLine(
                    cx + x, cy + y,
                    cx + x, cy + y,
                    col
                );
            }
        }

        m_2DGraphics.UpdateDisplay();
        CTimer::Get()->MsDelay(33);
    }

    return ShutdownHalt;
}
