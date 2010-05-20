/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamingOptions - read access for streaming paraview options
// .SECTION Description
// This class primarily provides read access to global streaming paraview
// options. The Set methods get called by vtkSMStreamingOptionsProxy
// when the user changes options via the dialog panel on the client.
// A singleton class instance stores the options, so that the static
// get methods can be called anywhere on the client or server to read
// the options.

#ifndef __vtkStreamingOptions_h
#define __vtkStreamingOptions_h

#include "vtkObject.h"

class VTK_EXPORT vtkStreamingOptions : public vtkObject
{
 public:
  static vtkStreamingOptions* New();
  vtkTypeMacro(vtkStreamingOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static int GetStreamedPasses();
  static void SetStreamedPasses(int);

  static bool GetEnableStreamMessages();
  static void SetEnableStreamMessages(bool);

  static bool GetUsePrioritization();
  static void SetUsePrioritization(bool);

  static bool GetUseViewOrdering();
  static void SetUseViewOrdering(bool);

  static int GetPieceCacheLimit();
  static void SetPieceCacheLimit(int);

  static int GetPieceRenderCutoff();
  static void SetPieceRenderCutoff(int);

protected:
  vtkStreamingOptions();
  ~vtkStreamingOptions();

private:
  vtkStreamingOptions(const vtkStreamingOptions&); // Not implemented
  void operator=(const vtkStreamingOptions&); // Not implemented
};

#endif
