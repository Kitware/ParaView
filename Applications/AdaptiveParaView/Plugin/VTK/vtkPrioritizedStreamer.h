/*=========================================================================

  Program:   ParaView
  Module:    vtkPrioritizedStreamer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPrioritizedStreamer - streams pieces in priority order
// .SECTION Description
// This streamer uses piece meta information to determine how important
// each piece is. It then renders the pieces in a most important to least
// important order, entirely skipping those which can be proven to not matter.
// For example, pieces near the camera are important, pieces off screen
// are unimportant, pieces that filters will reject are also unimportant.

#ifndef __vtkPrioritizedStreamer_h
#define __vtkPrioritizedStreamer_h

#include "vtkStreamingDriver.h"

class Internals;

class VTK_EXPORT vtkPrioritizedStreamer : public vtkStreamingDriver
{
public:
  vtkTypeMacro(vtkPrioritizedStreamer,vtkStreamingDriver);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPrioritizedStreamer *New();

//BTX
protected:
  vtkPrioritizedStreamer();
  ~vtkPrioritizedStreamer();

  virtual void StartRenderEvent();
  virtual void EndRenderEvent();

  virtual bool IsFirstPass();
  virtual bool IsEveryoneDone();

  virtual void ResetEveryone();
  virtual void AdvanceEveryone();
  virtual void FinalizeEveryone();

  Internals *Internal;

private:
  vtkPrioritizedStreamer(const vtkPrioritizedStreamer&);  // Not implemented.
  void operator=(const vtkPrioritizedStreamer&);  // Not implemented.

//ETX
};

#endif
