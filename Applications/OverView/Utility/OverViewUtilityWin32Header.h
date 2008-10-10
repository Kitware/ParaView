#ifndef __vtkOverViewUtilityWin32Header_h
#define __vtkOverViewUtilityWin32Header_h

#if defined(WIN32) && !defined(VTK_SNL_STATIC)

  #if defined(vtksnlOverViewUtility_EXPORTS)
    #define OVERVIEW_UTILITY_EXPORT __declspec( dllexport ) 
  #else
    #define OVERVIEW_UTILITY_EXPORT __declspec( dllimport ) 
  #endif

#else
  #define OVERVIEW_UTILITY_EXPORT
#endif

#endif
