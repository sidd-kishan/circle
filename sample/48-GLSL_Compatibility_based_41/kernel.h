#ifndef _kernel_h
#define _kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/2dgraphics.h>
#include <circle/types.h>

enum TShutdownMode
{
    ShutdownNone,
    ShutdownHalt,
    ShutdownReboot
};

class CKernel
{
public:
    CKernel (void);
    ~CKernel (void);

    boolean Initialize (void);
    TShutdownMode Run (void);

private:
    // ORDER MATTERS IN CIRCLE
    CActLED         m_ActLED;
    CKernelOptions m_Options;
    C2DGraphics    m_2DGraphics;
};

#endif
