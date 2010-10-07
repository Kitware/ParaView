/*=========================================================================

  Program:   ParaView
  Module:    vtkAdaptiveOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAdaptiveOptions - read access for streaming paraview options
// .SECTION Description
// This class primarily provides read access to global streaming paraview
// options. The Set methods get called by vtkSMAdaptiveOptionsProxy
// when the user changes options via the dialog panel on the client.
// A singleton class instance stores the options, so that the static
// get methods can be called anywhere on the client or server to read
// the options.

#ifndef __vtkAdaptiveOptions_h
#define __vtkAdaptiveOptions_h

#include "vtkObject.h"

class VTK_EXPORT vtkAdaptiveOptions : public vtkObject
{
 public:
  static vtkAdaptiveOptions* New();
  vtkTypeMacro(vtkAdaptiveOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static bool GetEnableStreamMessages();
  static void SetEnableStreamMessages(bool);

  static bool GetUsePrioritization();
  static void SetUsePrioritization(bool);

  static bool GetUseViewOrdering();
  static void SetUseViewOrdering(bool);

  static int GetPieceCacheLimit();
  static void SetPieceCacheLimit(int);

  static int GetHeight();
  static void SetHeight(int);

  static int GetDegree();
  static void SetDegree(int);

  static int GetRate();
  static void SetRate(int);

  static int GetMaxSplits();
  static void SetMaxSplits(int);

  static int GetShowOn();
  static void SetShowOn(int);
//BTX
  enum ShowTimes {PIECE, REFINE, FINISH};
//ETX
protected:
  vtkAdaptiveOptions();
  ~vtkAdaptiveOptions();

private:
  vtkAdaptiveOptions(const vtkAdaptiveOptions&); // Not implemented
  void operator=(const vtkAdaptiveOptions&); // Not implemented
};

#endif
