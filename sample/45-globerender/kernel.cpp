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

// ===== Illumination control (DROP-IN) =====
static inline unsigned char ShadePixel(float nx, float ny, float nz)
{
    // Camera-facing light
    const float lx = 0.0f;
    const float ly = 0.0f;
    const float lz = 1.0f;

    float diffuse = nx*lx + ny*ly + nz*lz;
    if (diffuse < 0.0f) diffuse = 0.0f;

    // ↑ INCREASE ILLUMINATION HERE
    const float ambient = 0.45f;   // was likely 0 or too low
    float intensity = ambient + diffuse * 1.6f;

    // Optional specular pop (cheap)
    intensity += diffuse * diffuse * 0.25f;

    if (intensity > 1.0f) intensity = 1.0f;

    return (unsigned char)(intensity * 255.0f);
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
				/* -------- checker projection -------- */

				// scale controls checker size
				const float scale = 8.0f;   // bigger squares
				//const float scale = 8.0f;   // smaller squares


				// project in world XZ plane (stable, no UVs needed)
				int cx = (int)(p.x * scale);
				int cz = (int)(p.z * scale);

				// XOR pattern
				bool checker = (cx ^ cz) & 1;

				// red & black (current)
				unsigned base_r = checker ? 255 : 0;

				// red & white
				base_r = checker ? 255 : 255;
				unsigned base_g = checker ? 0   : 255;
				unsigned base_b = checker ? 0   : 255;

				// overwritten by black & white
				//base_r = checker ? 255 : 0;
				//base_g = base_r;
				//base_b = base_r;


				/* -------- illumination -------- */


                Vector3D L = Normalize(Light);

				float diffuse = Dot(n, L);
				if (diffuse < 0.0f) diffuse = 0.0f;

				const float ambient = 0.35f;
				float intensity = ambient + diffuse * 1.4f;
				if (intensity > 1.0f) intensity = 1.0f;

				// apply lighting
				unsigned r = (unsigned)(base_r * intensity);
				unsigned g = (unsigned)(base_g * intensity);
				unsigned b = (unsigned)(base_b * intensity);

				// pack RGB
				unsigned col = (r << 16) | (g << 8) | b;

				m_Screen.SetPixel(x, y, col);
            }
        }

        CTimer::Get()->MsDelay(16);
    }

    return ShutdownHalt;
}
