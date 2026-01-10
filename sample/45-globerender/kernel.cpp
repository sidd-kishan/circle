#include "kernel.h"
#include <circle/timer.h>
#include <math.h>

/* =========================================================
   Local math helpers
   ========================================================= */

static inline Vector3D Add(Vector3D a, Vector3D b)
{
    return {a.x+b.x, a.y+b.y, a.z+b.z};
}

static inline Vector3D Sub(Vector3D a, Vector3D b)
{
    return {a.x-b.x, a.y-b.y, a.z-b.z};
}

static inline Vector3D Mul(Vector3D a, float s)
{
    return {a.x*s, a.y*s, a.z*s};
}

static inline float Dot(Vector3D a, Vector3D b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline Vector3D Normalize(Vector3D v)
{
    float l = sqrtf(Dot(v,v));
    return Mul(v, 1.0f/l);
}

static float SphereIntersect(Vector3D ro, Vector3D rd,
                             Vector3D c, float r)
{
    Vector3D oc = Sub(ro, c);
    float b = Dot(oc, rd);
    float cc = Dot(oc, oc) - r*r;
    float h = b*b - cc;
    if (h < 0.0f) return -1.0f;
    return -b - sqrtf(h);
}

/* =========================================================
   Kernel
   ========================================================= */

CKernel::CKernel(void)
: m_ActLED(false),
  m_Options(),
  m_DeviceNameService(),
  m_Screen(m_Options.GetWidth(), m_Options.GetHeight())
{
    m_ActLED.Blink(3);
}

CKernel::~CKernel(void)
{
}

boolean CKernel::Initialize(void)
{
    return m_Screen.Initialize();
}

void CKernel::Clear(void)
{
    for (unsigned y = 0; y < m_Screen.GetHeight(); y++)
    for (unsigned x = 0; x < m_Screen.GetWidth(); x++)
        m_Screen.SetPixel(x, y, 0x000000);
}

TShutdownMode CKernel::Run(void)
{
    Vector3D Camera = {0,0,-3};
    Vector3D Sphere = {0,0,0};
    Vector3D Light  = Normalize({1,1,1});

    while (1)
    {
        Clear();

        for (unsigned y = 0; y < m_Screen.GetHeight(); y++)
        for (unsigned x = 0; x < m_Screen.GetWidth(); x++)
        {
            float u = (2.0f*x/m_Screen.GetWidth() - 1.0f) * 1.6f;
            float v = (1.0f - 2.0f*y/m_Screen.GetHeight());

            Vector3D rd = Normalize({u,v,1});
            float t = SphereIntersect(Camera, rd, Sphere, 1.0f);

            if (t > 0)
            {
                Vector3D p = Add(Camera, Mul(rd, t));
                Vector3D n = Normalize(Sub(p, Sphere));

                float l = Dot(n, Light);
                if (l < 0) l = 0;

                unsigned c = (unsigned)(l * 255);
                unsigned col = (c<<16)|(c<<8)|c;

                m_Screen.SetPixel(x, y, col);
            }
        }

        CTimer::Get()->MsDelay(16);
    }

    return ShutdownHalt;
}
