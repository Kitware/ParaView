/*=========================================================================

  Program:   ParaView
  Module:    vtkMatplotlibUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMatplotlibUtilities
// .SECTION Description
// vtkMatplotlibUtilities is a utility class for generating vtkImageData objects from
// a matplotlib script. It requires that Python and matplotlib be
// available on your system.
#ifndef __vtkMatplotlibUtilities_h
#define __vtkMatplotlibUtilities_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

#include "vtkObject.h"

struct _object;
typedef struct _object PyObject;
class vtkImageData;
class vtkPythonInterpreter;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkMatplotlibUtilities : public vtkObject
{
public:
  static vtkMatplotlibUtilities* New();
  vtkTypeMacro(vtkMatplotlibUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Renders a matplotlib plot to a VTK image. The Python script
  // defines a canvas (must be a matplotlib.backends.backend_agg.FigureCanvasAgg)
  // to which a plot is rendered. The resulting vtkImageData is returned.
  vtkImageData* ImageFromScript(const char* script, const char* canvasName,
                                int width, int height);

 protected:
  vtkMatplotlibUtilities();
  virtual ~vtkMatplotlibUtilities();

  vtkPythonInterpreter* Interpreter;

  // Used for runtime checking of matplotlib's mathtext availability.
  enum Availability
    {
    NOT_TESTED = 0,
    AVAILABLE,
    UNAVAILABLE
    };

  // Function used to check matplotlib availability and update MatplotlibAvailable.
  // This will do tests only the first time this method is called.
  static void CheckMatplotlibAvailability();

  // Initialize matplotlib. This loads the matplotlib module into the python interpreter.
  bool InitializeMatplotlib();

  // Sets up a matplotlib canvas and figure object
  void InitializeCanvas(const char* canvasName);

  // Destroys matplotlib canvas and figure object
  void DestroyCanvas(const char* canvasName);

  // Determine the DPI (matplotlib lies about DPI, so we need to figure it out).
  bool DetermineDPI();
  
  // Cleanup and destroy any python objects. This is called during destructor as
  // well as when the Python interpreter is finalized. Thus this class must
  // handle the case where the internal python objects disappear between calls.
  void CleanupPythonObjects();

  // Check for errors in Python
  bool CheckForPythonError(PyObject* object);
  bool CheckForPythonError();

  // Get a vtkImageData object from a named matplotlib canvas. The
  // caller must call Delete() on the returned value when finished with it.
  vtkImageData* ImageFromCanvas(const char* canvasName);

 private:
  vtkMatplotlibUtilities(const vtkMatplotlibUtilities&); // Not implemented.
  void operator=(const vtkMatplotlibUtilities&); // Not implemented.

  static Availability MatplotlibAvailable;

  int DPI;
};

#endif // __vtkMatplotlibUtilities_h
