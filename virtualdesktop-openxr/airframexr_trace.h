// AirFrameXR: per-frame facts handed to the OVR runtime DLL (OVRShim) when it exports ovrshim_Trace. The libOVR API cannot
// carry them: when the app's frame really started, and whether a frame holds a newly released picture or resubmits the
// last one. A real LibOVR does not export ovrshim_Trace and the calls are then no-ops. Times are OVR seconds
// (ovr_GetTimeInSeconds), OVRShim's clock. Kinds 3, 5, 6 and 7 were used by the 2026-10-10 experiment (branch visionalvr:
// xrLocateViews, submit, async submission thread) and are free again.
#pragma once
#include <windows.h>

#include <atomic>

namespace airframexr {
    enum TraceKind : int {
        TraceWaitFrameReturn = 1, // xrWaitFrame about to return; a = predicted display time
        TraceBeginFrame = 2,      // xrBeginFrame; a = 1 when the previous frame was discarded
        TraceEndFrame = 4,        // xrEndFrame past its call-order checks; a = the layers' display time, b = layer count
        TraceFrameContent = 8,    // layers resolved; a = swapchain slices with an image released since the previous frame
                                  // (0: the app resubmitted its last picture), b = swapchain slices in the frame
        TraceFrameCopies = 9,     // a = slices copied or resolved into a second OVR swapchain (texture array slice > 0, MSAA),
                                  // b = slices on that path not copied because nothing was released
    };
    typedef void(__cdecl* TraceFn)(int kind, long long frameId, double now, double a, double b);

    inline void Trace(int kind, long long frameId, double now, double a = 0.0, double b = 0.0) {
        static std::atomic<TraceFn> fn{nullptr};
        static std::atomic<int> attempts{0};
        TraceFn f = fn.load(std::memory_order_relaxed);
        if (!f && attempts.load(std::memory_order_relaxed) < 64) { // the runtime DLL is loaded at ovr_Initialize: retry a little
            attempts.fetch_add(1, std::memory_order_relaxed);
            if (HMODULE m = GetModuleHandleW(L"LibOVRRT64_1.dll")) {
                f = (TraceFn)GetProcAddress(m, "ovrshim_Trace");
                fn.store(f, std::memory_order_relaxed);
            }
        }
        if (f) {
            f(kind, frameId, now, a, b);
        }
    }
} // namespace airframexr
