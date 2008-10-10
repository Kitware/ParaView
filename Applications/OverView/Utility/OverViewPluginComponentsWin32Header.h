#ifndef __OverViewPluginComponentsWin32Header_h
#define __OverViewPluginComponentsWin32Header_h

#include <vtksnlConfigure.h>

#if defined(WIN32) && !defined(VTK_SNL_STATIC)

  #if defined(OverViewPluginComponents_EXPORTS)
    #define OVERVIEW_PLUGIN_COMPONENTS_EXPORT __declspec( dllexport ) 
  #else
    #define OVERVIEW_PLUGIN_COMPONENTS_EXPORT __declspec( dllimport ) 
  #endif

#else
  #define OVERVIEW_PLUGIN_COMPONENTS_EXPORT
#endif

#endif
