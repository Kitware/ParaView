/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiResolutionStreamer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiResolutionStreamer - streams pieces with increasing refinement
// .SECTION Description
// Besides rendering pieces in a most to least important order, this streamer
// also continuously increases the resolution of the pieces it draws. That is,
// it draws the entire domain at a low level of resolution, then clears the
// screen and redraws the domain with one or more of the original pieces
// split and refined at a higher level of resolution. Groups of unimportant
// pieces, for instance those offscreen are merged back together into a lower
// resolution whole.

#ifndef __vtkMultiResolutionStreamer_h
#define __vtkMultiResolutionStreamer_h

#include "vtkStreamingDriver.h"

class VTK_EXPORT vtkMultiResolutionStreamer : public vtkStreamingDriver
{
public:
  vtkTypeMacro(vtkMultiResolutionStreamer,vtkStreamingDriver);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMultiResolutionStreamer *New();

  //Description:
  //A command to restart streaming on next render.
  void RestartStreaming();

  //Description:
  //A command to halt streaming as soon as possible.
  void StopStreaming();

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

  // Description:
  // Whether the user drives progression manually (0) or wether the driver
  // does automatically (1). The default value is manual.
  enum {MANUAL, AUTOMATIC};
  vtkSetMacro(ProgressionMode, int);
  vtkGetMacro(ProgressionMode, int);

  // Description:
  // The maximum height of the refinement tree.
  // The default is 5.
  vtkSetMacro(RefinementDepth, int);
  vtkGetMacro(RefinementDepth, int);

  // Description:
  // The deepest level of the refinement tree that refinement
  // is allowed to go to. The default is 10.
  vtkSetMacro(DepthLimit, int);
  vtkGetMacro(DepthLimit, int);

  // Description:
  // The maximum number of pieces that are allowed to be split
  // on each render pass. The default is 8.
  vtkSetMacro(MaxSplits, int);
  vtkGetMacro(MaxSplits, int);

  // Description:
  // In manual mode, this tells the driver to increase refinement
  // of all the harness shown within.
  void Refine();

  // Description:
  // In manual mode, this tells the driver to decrease refinement
  // of all the harness shown within.
  void Coarsen();

  // Description:
  // Apps can call this during camera motion to prevent refinement
  // during user interaction.
  vtkSetMacro(Interacting, bool);
  vtkGetMacro(Interacting, bool);

  // Description:
  // Number of polygonal cells estimated from number of sources cells
  // as sourcecells^CellPixelFactor.
  vtkSetMacro(CellPixelFactor, double);
  vtkGetMacro(CellPixelFactor, double);

  // Description:
  // Back facing pieces determined as those whose normal dot gaze is less than this.
  void SetBackFaceFactor(double);
  double GetBackFaceFactor();

//BTX
protected:
  vtkMultiResolutionStreamer();
  ~vtkMultiResolutionStreamer();

  virtual void StartRenderEvent();
  virtual void EndRenderEvent();

  // Description:
  // Overridden to set up initial number of passes.
  virtual void AddHarnessInternal(vtkStreamingHarness *);

  bool IsFirstPass();
  bool IsWendDone();
  bool IsCompletelyDone();

  void PrepareFirstPass();
  void PrepareNextPass();
  int Refine(vtkStreamingHarness *);
  int Coarsen(vtkStreamingHarness *);
  void Reap(vtkStreamingHarness *);
  void PixelBackoff(vtkStreamingHarness *);
  bool AnyToRefine(vtkStreamingHarness *);

  enum {COARSEN, STAY, ADVANCE};

  int PipelinePrioritization;
  int ViewPrioritization;
  int ProgressionMode;
  int RefinementDepth;
  int DepthLimit;
  int MaxSplits;
  double CellPixelFactor;

  bool Interacting;
private:
  vtkMultiResolutionStreamer
    (const vtkMultiResolutionStreamer&);  // Not implemented.
  void operator=(const vtkMultiResolutionStreamer&);  // Not implemented.

  class Internals;
  Internals *Internal;

//ETX
};

#endif
