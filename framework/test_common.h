#pragma once
// On Windows, doctest.h pulls in <windows.h> for its timer. windows.h's GDI
// header (wingdi.h) declares functions named Rectangle, Arc, Polygon,
// Ellipse, etc. as plain WinAPI symbols — which silently collide with any
// C++ class you write using one of those names (e.g. `class Rectangle`).
// Defining these before windows.h is ever included excludes that section
// entirely, so problem classes are free to use those names.
//
// All problem tests.cpp files should include THIS header instead of
// including framework/doctest.h directly, so this protection is never
// accidentally skipped for a new problem.
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOGDI
        #define NOGDI
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#include "doctest.h"
