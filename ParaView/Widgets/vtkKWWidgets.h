#ifndef __vtkKWWidgets_h
#define __vtkKWWidgets_h

#if defined(WIN32) && !defined(VTKSTATIC)
 #if defined(vtkKWWidgetsTCL_EXPORTS)
  #define KW_WIDGETS_EXPORT __declspec( dllexport ) 
 #else
  #define KW_WIDGETS_EXPORT __declspec( dllimport ) 
 #endif
#else
  #define KW_WIDGETS_EXPORT
#endif

#endif
