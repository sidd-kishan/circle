#include "kernel.h"
#include <circle/timer.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

static inline Vector3D RotateY(Vector3D v, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    return {
        v.x * c - v.z * s,
        v.y,
        v.x * s + v.z * c
    };
}


// ============================================================================
// Checker_WorldSpacePlanar_XZ
//
// PURPOSE:
//   Implements a procedural checkerboard using WORLD-SPACE planar projection.
//   This maps an infinite checker pattern onto the XZ plane, independent of
//   object geometry or curvature.
//
// HISTORICAL / MATHEMATICAL CONTEXT:
//   • Rooted in Euclidean geometry (c. 300 BCE):
//       - Continuous space is subdivided into equal regions (tiling).
//   • Uses parity (even/odd classification) studied by the Pythagoreans.
//   • Mathematically equivalent to a 2D square lattice used in:
//       - crystal structures (NaCl lattice)
//       - diffraction gratings
//       - early solid-state physics models
//
// PHYSICAL INTERPRETATION:
//   This function evaluates a discrete lattice function:
//       f(x,z) = (-1)^(⌊sx⌋ + ⌊sz⌋)
//   which alternates sign across adjacent spatial cells.
//
// PARAMETERS:
//   p     : world-space hit position on the surface
//   scale : spatial frequency of the checker pattern
//           (higher = smaller squares)
//
// RETURN VALUE:
//   true  -> red square
//   false -> black square
// ============================================================================
static inline bool Checker_WorldSpacePlanar_XZ(Vector3D p, float scale)
{
    // Convert continuous world-space X coordinate into a discrete cell index.
    // This is a modern digital analogue of Euclidean spatial subdivision.
    int cx = (int)(p.x * scale);

    // Convert continuous world-space Z coordinate into a discrete cell index.
    // Choosing XZ defines a planar projection similar to a "ground plane".
    int cz = (int)(p.z * scale);

    // XOR evaluates parity:
    //   • same parity (even+even or odd+odd) → 0
    //   • different parity                  → 1
    //
    // This parity alternation is the mathematical essence of a checkerboard.
    return ((cx ^ cz) & 1) != 0;
}


// ============================================================================
// Checker_SphericalUV_LatLong
//
// PURPOSE:
//   Implements a procedural checkerboard mapped onto a SPHERE using
//   spherical (latitude–longitude) parameterization.
//
// HISTORICAL / MATHEMATICAL CONTEXT:
//   • Spherical coordinates formalized by:
//       - Hipparchus (2nd century BCE)
//       - Ptolemy (2nd century CE)
//   • Used historically for:
//       - star catalogs
//       - planetary navigation
//       - early cartography
//   • This exact mapping is known as an EQUIRECTANGULAR PROJECTION.
//
// GEOMETRIC INSIGHT:
//   On a perfect sphere, the normalized surface normal equals the radial
//   position vector — a fact formalized by Gauss (1827).
//   This allows UV coordinates to be derived directly from normals.
//
// PHYSICAL INTERPRETATION:
//   This discretizes angular space on a curved manifold, analogous to
//   latitude–longitude grids used in:
//       - climate models
//       - magnetic field sampling
//       - planetary surface meshing
//
// PARAMETERS:
//   n     : normalized surface normal at the hit point
//   scale : number of checker divisions per angular axis
//
// RETURN VALUE:
//   true  -> red square
//   false -> black square
// ============================================================================
static inline bool Checker_SphericalUV_LatLong(Vector3D n, float scale)
{
    // Compute azimuth (longitude) angle using atan2.
    // atan2 avoids quadrant ambiguity and spans [-π, +π].
    float u = atan2f(n.z, n.x);

    // Normalize longitude from angular range to [0, 1].
    // This mirrors historical map projections of the Earth.
    u = (u / (2.0f * M_PI)) + 0.5f;

    // Compute elevation (latitude) using arcsin.
    // Range is [-π/2, +π/2], corresponding to south ↔ north poles.
    float v = asinf(n.y);

    // Normalize latitude to [0, 1].
    // This introduces polar distortion, identical to cartographic maps.
    v = 0.5f - (v / M_PI);

    // Convert continuous UV space into discrete checker cells.
    // This is angular discretization of a curved surface.
    int cu = (int)(u * scale);
    int cv = (int)(v * scale);

    // XOR parity across UV grid to produce alternating pattern.
    return ((cu ^ cv) & 1) != 0;
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

    float angle = 0.0f;   // <<< SPIN STATE (PERSISTENT)

    while (1)
    {
        angle += 0.03f;   // <<< SPIN SPEED (radians per frame)

        Clear();

        float ca = cosf(angle);
        float sa = sinf(angle);

        for (unsigned y = 0; y < m_Screen.GetHeight(); y++)
        for (unsigned x = 0; x < m_Screen.GetWidth(); x++)
        {
            float u = (2.0f*x/m_Screen.GetWidth() - 1.0f) * 1.6f;
            float v = (1.0f - 2.0f*y/m_Screen.GetHeight());

            Vector3D rd = Normalize({u,v,1});
            float t = SphereIntersect(Camera, rd, Sphere, 1.5f);

            if (t > 0)
            {
                Vector3D p = Add(Camera, Mul(rd, t));
                Vector3D n = Normalize(Sub(p, Sphere));

                /* =====================================================
                   SPHERE SPIN (OBJECT-SPACE ROTATION)
                   -----------------------------------------------------
                   Rotates the hit-point around Y-axis BEFORE texturing.
                   This makes the checker pattern rotate while geometry
                   remains fixed.
                   ===================================================== */

                Vector3D ps = p;
                ps.x =  p.x * ca - p.z * sa;
                ps.z =  p.x * sa + p.z * ca;

                /* -------- checker projection -------- */

                const float scale = 8.0f;

                //int cx = (int)(ps.x * scale);
                //int cz = (int)(ps.z * scale);

                //bool checker = (cx ^ cz) & 1;
				Vector3D p_obj = RotateY(p, angle);
				bool checker = Checker_SphericalUV_LatLong(p_obj, scale);

                // red & white checker
                unsigned base_r = 255;
                unsigned base_g = checker ? 0   : 255;
                unsigned base_b = checker ? 0   : 255;

                /* -------- illumination -------- */

                float diffuse = Dot(n, Light);
                if (diffuse < 0.0f) diffuse = 0.0f;

                const float ambient = 0.35f;
                float intensity = ambient + diffuse * 1.4f;
                if (intensity > 1.0f) intensity = 1.0f;

                unsigned r = (unsigned)(base_r * intensity);
                unsigned g = (unsigned)(base_g * intensity);
                unsigned b = (unsigned)(base_b * intensity);

                unsigned col = (r << 16) | (g << 8) | b;
                m_Screen.SetPixel(x, y, col);
            }
        }

        //CTimer::Get()->MsDelay(16);   // <<< REQUIRED FOR MOTION
    }

    return ShutdownHalt;
}
