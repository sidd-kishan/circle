#include "kernel.h"
#include <circle/timer.h>
#include <math.h>

struct vec4 {
    float x, y, z, w;
    vec4(float x = 0, float y = 0, float z = 0, float w = 0):
        x(x), y(y), z(z), w(w)
    {}
};

struct vec2 {
    float x, y;
    vec2(float x = 0, float y = 0):
        x(x), y(y)
    {}
    vec2 yx() const { return vec2(y, x); }
    vec4 xyyx() const { return vec4(x, y, y, x); }
};


vec2 operator *(const vec2 &a, float s) { return vec2(a.x*s, a.y*s); }
vec2 operator +(const vec2 &a, float s) { return vec2(a.x+s, a.y+s); }
vec2 operator *(float s, const vec2 &a) { return a*s; }
vec2 operator -(const vec2 &a, const vec2 &b) { return vec2(a.x-b.x, a.y-b.y); }
vec2 operator +(const vec2 &a, const vec2 &b) { return vec2(a.x+b.x, a.y+b.y); }
vec2 operator *(const vec2 &a, const vec2 &b) { return vec2(a.x*b.x, a.y*b.y); }
vec2 operator /(const vec2 &a, float s) { return vec2(a.x/s, a.y/s); }
float dot(const vec2 &a, const vec2 &b) { return a.x*b.x + a.y*b.y; }
vec2 abs(const vec2 &a) { return vec2(fabsf(a.x), fabsf(a.y)); } 
vec2 &operator +=(vec2 &a, const vec2 &b) { a = a + b; return a; }
vec2 &operator +=(vec2 &a, float s) { a = a + s; return a; }
vec2 cos(const vec2 &a) { return vec2(cosf(a.x), cosf(a.y)); } 
vec4 sin(const vec4 &a) { return vec4(sinf(a.x), sinf(a.y), sinf(a.z), sinf(a.w)); } 
vec4 exp(const vec4 &a) { return vec4(expf(a.x), expf(a.y), expf(a.z), expf(a.w)); } 
vec4 tanh(const vec4 &a) { return vec4(tanhf(a.x), tanhf(a.y), tanhf(a.z), tanhf(a.w)); } 
vec4 operator +(const vec4 &a, float s) { return vec4(a.x+s, a.y+s, a.z+s, a.w+s); }
vec4 operator *(const vec4 &a, float s) { return vec4(a.x*s, a.y*s, a.z*s, a.w*s); }
vec4 operator *(float s, const vec4 &a) { return a*s; }
vec4 operator +(const vec4 &a, const vec4 &b) { return vec4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }
vec4 &operator +=(vec4 &a, const vec4 &b) { a = a + b; return a; }
vec4 operator -(float s, const vec4 &a) { return vec4(s-a.x, s-a.y, s-a.z, s-a.w); }
vec4 operator /(const vec4 &a, const vec4 &b) { return vec4(a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w); }
static inline vec4 operator -(const vec4 &a, const vec4 &b)
{
    return vec4(
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w
    );
}

inline vec4 operator *(const vec4 &a, const vec2 &b)
{
    return vec4(
        a.x * b.x,
        a.y * b.y,
        a.z * b.x,
        a.w * b.y
    );
}

inline vec4 operator *(const vec2 &b, const vec4 &a)
{
    return a * b;
}

inline float abs(float v)
{
    return fabsf(v);
}

inline vec4 abs(const vec4 &a)
{
    return vec4(
        fabsf(a.x),
        fabsf(a.y),
        fabsf(a.z),
        fabsf(a.w)
    );
}

inline vec2 sin(const vec2 &a)
{
    return vec2(sinf(a.x), sinf(a.y));
}

inline vec4 cos(const vec4 &a)
{
    return vec4(
        cosf(a.x),
        cosf(a.y),
        cosf(a.z),
        cosf(a.w)
    );
}

inline vec4 operator +(const vec4 &a, const vec2 &b)
{
    return vec4(
        a.x + b.x,
        a.y + b.y,
        a.z + b.x,
        a.w + b.y
    );
}

inline vec4 &operator +=(vec4 &a, float s)
{
    a = a + s;
    return a;
}


// ------------------------------------------------------------
// GLSL compatibility helpers (drop-in)
// ------------------------------------------------------------
static inline float clamp01(float v)
{return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static inline float fract(float v)
{return v - floorf(v);
}

static inline float mix(float a, float b, float t)
{return a + (b - a) * t;
}

static inline vec2 mix(const vec2& a, const vec2& b, float t)
{return a + (b - a) * t;
}

static inline vec4 mix(const vec4& a, const vec4& b, float t)
{return a + (b - a) * t;
}


// ------------------------------------------------------------
// Gamma correction helpers (sRGB-ish)
// ------------------------------------------------------------
static inline float gamma_correct(float v)
{
    // Linear → sRGB approx (2.2)
    return powf(clamp01(v), 1.0f / 2.2f);
}


// ------------------------------------------------------------
// vec4 → Circle color bridge
// ------------------------------------------------------------
static inline CDisplay::TColor vec4_to_color(const vec4& c)
{
    unsigned r = (unsigned)(gamma_correct(c.x) * 255.0f);
    unsigned g = (unsigned)(gamma_correct(c.y) * 255.0f);
    unsigned b = (unsigned)(gamma_correct(c.z) * 255.0f);

    return (CDisplay::TColor)((r << 16) | (g << 8) | b);
}




#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

TShutdownMode CKernel::Run(void)
{
    const unsigned W = m_2DGraphics.GetWidth();
    const unsigned H = m_2DGraphics.GetHeight();

    //const int cx = W / 2;
    //const int cy = H / 2;
    //const int R  = 90;

    //float time = 0.0f;

    while (1)
    {
        for (int i = 0; i < 240; ++i) {
			m_2DGraphics.ClearScreen(CDisplay::Black);
			int w = W;
			int h = H;
			vec2 r = {(float)w, (float)h};
			float t = ((float)i/240)*2*M_PI;

			for (int y = 0; y < h; ++y){
            for (int x = 0; x < w; ++x) {
                vec4 o;
                vec2 FC = {(float)x, (float)y};
                //////////////////////////////
                // https://x.com/XorDev/status/1894123951401378051
                // ---> put shader here  <------
                //////////////////////////////
				vec2 p=(FC*2.-r)/r.y,l,i,v=p*(l+=4.-4.*abs(.7-dot(p,p)));
                for(;i.y++<8.;o+=(sin(v.xyyx())+1.)*abs(v.x-v.y))v+=cos(v.yx()*i.y+i+t)/i.y+.7;
                o=tanh(5.*exp(l.x-4.-p.y*vec4(-1,1,2,0))/o);
                //update frame buffer bellow
                m_2DGraphics.DrawPixel(
						x,
						y,
						vec4_to_color(o)
					);
				}
			}
			//fclose(f);
			//printf("Generated %s (%3d/%3d)\n", output_path, i + 1, 240);

			m_2DGraphics.UpdateDisplay();
		}
    }

    return ShutdownHalt;
}
