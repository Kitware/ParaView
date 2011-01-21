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

class VTK_EXPORT vtkPrioritizedStreamer : public vtkStreamingDriver
{
public:
  vtkTypeMacro(vtkPrioritizedStreamer,vtkStreamingDriver);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPrioritizedStreamer *New();

  //Description:
  //A command to restart streaming on next render.
  void RestartStreaming();

  //Description:
  //A command to halt streaming as soon as possible.
  void StopStreaming();

  //Description:
  //Controls the number of pieces all harness shown by this driver
  //break their data into. Default is 32.
  void SetNumberOfPasses(int);
  vtkGetMacro(NumberOfPasses, int);

  //Description:
  //Controls the number of pieces all harness shown by this driver
  //break their data into. Default is 32.
  vtkSetMacro(LastPass, int);
  vtkGetMacro(LastPass, int);

  //Description:
  //Enables or disables data centric prioritization and culling.
  //Default is 1, meaning enabled.
  vtkSetMacro(PipelinePrioritization, int);
  vtkGetMacro(PipelinePrioritization, int);

  //Description:
  //Enables or disables camera centric prioritization and culling.
  //Default is 1, meaning enabled.
  vtkSetMacro(ViewPrioritization, int);
  vtkGetMacro(ViewPrioritization, int);

//BTX
protected:
  vtkPrioritizedStreamer();
  ~vtkPrioritizedStreamer();

  virtual void StartRenderEvent();
  virtual void EndRenderEvent();

  // Description:
  // Overridden to set up initial number of passes.
  virtual void AddHarnessInternal(vtkStreamingHarness *);

  bool IsFirstPass();
  bool IsEveryoneDone();

  void PrepareFirstPass();
  void PrepareNextPass();

  int NumberOfPasses;
  int LastPass;
  int PipelinePrioritization;
  int ViewPrioritization;

private:
  vtkPrioritizedStreamer(const vtkPrioritizedStreamer&);  // Not implemented.
  void operator=(const vtkPrioritizedStreamer&);  // Not implemented.

  class Internals;
  Internals *Internal;

//ETX
};

#endif
