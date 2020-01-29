/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file       mi/base/miwindows.h
/// \brief      include a lean version of windows.h

#ifndef BASE_MI_WINDOWS_H
#define BASE_MI_WINDOWS_H

#include <mi/base/config.h>

#ifdef MI_PLATFORM_WINDOWS
// Leave out unnecessary subsystems to speedup compilation.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

// Undefine these two symbols which are defined in windows.h and clash with
// our definitions.
#undef min
#undef max
#undef IGNORE

#endif // MI_PLATFORM_WINDOWS

#endif // BASE_MI_WINDOWS_H
