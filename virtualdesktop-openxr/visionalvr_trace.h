// VisionALVR: per-frame timing trace handed to the OVR runtime DLL (OVRShim) when it exports ovrshim_Trace.
// A real LibOVR does not export it, and the calls are then no-ops. Times are OVR seconds (ovr_GetTimeInSeconds), the
// same clock OVRShim and alvr_host use, so one frame can be followed from xrWaitFrame to the encoder.
#pragma once
#include <windows.h>

namespace visionalvr {
    enum TraceKind : int {
        TraceWaitFrameReturn = 1, // xrWaitFrame about to return; a = predicted display time
        TraceBeginFrame = 2,      // xrBeginFrame; a = 1 when the previous frame was discarded
        TraceLocateViews = 3,     // xrLocateViews; a = requested display time
        TraceEndFrame = 4,        // xrEndFrame entered its critical section; a = the layers' display time, b = layer count
        TraceEndFrameSubmit = 5,  // layers handed to ovr_EndFrame (sync) or to the async submission thread; b = layer count
        TraceAsyncWaitReturn = 6, // async submission thread: ovr_WaitToBeginFrame returned
        TraceAsyncEndFrame = 7,   // async submission thread: about to call ovr_EndFrame; b = layer count
    };
    typedef void(__cdecl* TraceFn)(int kind, long long frameId, double now, double a, double b);

    inline void Trace(int kind, long long frameId, double now, double a = 0.0, double b = 0.0) {
        static TraceFn fn = nullptr;
        static int attempts = 0;
        if (!fn && attempts < 64) { // the runtime DLL is loaded at ovr_Initialize: retry a little
            attempts++;
            if (HMODULE m = GetModuleHandleW(L"LibOVRRT64_1.dll")) {
                fn = (TraceFn)GetProcAddress(m, "ovrshim_Trace");
            }
        }
        if (fn) {
            fn(kind, frameId, now, a, b);
        }
    }
} // namespace visionalvr
