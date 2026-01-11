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
// ------------------------------------------------------------
// MAIN LOOP — TRUE SPHERE + CHECKER + SPIN
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
        time += 0.04f;   // rotation speed

        m_2DGraphics.ClearScreen(CDisplay::Black);

        // ----------------------------------------------------
        // Render filled sphere via scanlines
        // ----------------------------------------------------
        for (int py = -R; py <= R; py++)
        {
            float y = (float)py / R;             // [-1,1]
            float y2 = y * y;
            if (y2 > 1.0f)
                continue;

            // half-width of sphere at this latitude
            float xspan = sqrtf(1.0f - y2);
            int pxmax = (int)(xspan * R);

            for (int px = -pxmax; px <= pxmax; px++)
            {
                float x = (float)px / R;
                float z = sqrtf(1.0f - x*x - y*y);

                // ------------------------------------------------
                // SPIN: rotate longitude over time
                // ------------------------------------------------
                float lon = atan2f(z, x) + time;   // <<< SPIN
                float lat = asinf(y);

                // Normalize to [0,1]
                float u = lon / (2.0f * 3.1415926f);
                float v = lat / 3.1415926f;

                u -= floorf(u);   // wrap
                v += 0.5f;

                // ------------------------------------------------
                // TRUE CHECKERBOARD ON SPHERE
                // ------------------------------------------------
                const int checkerU = 12;   // columns
                const int checkerV = 12;   // rows

                int cu = (int)(u * checkerU);
                int cv = (int)(v * checkerV);

                bool checker = ((cu ^ cv) & 1) != 0;

                T2DColor col = checker
                    ? CDisplay::BrightRed
                    : CDisplay::White;

                m_2DGraphics.DrawPixel(
                    cx + px,
                    cy + py,
                    col
                );
            }
        }

        m_2DGraphics.UpdateDisplay();
    }

    return ShutdownHalt;
}
