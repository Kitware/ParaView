/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTreeComposite.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTreeComposite - An interruptable version of vtkCompositeRenderManager
//
// .SECTION Description
// vtkPVTreeComposite is a subclass of composite render manager that has methods that 
// interrupt rendering. This functionality requires an MPI controller.
//
// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkCompositeRenderManager

#include "vtkToolkits.h" // Needed for VTK_USE_MPI
#ifndef __vtkPVTreeComposite_h
#define __vtkPVTreeComposite_h

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h" // Needed for vtkMPICommunicator::Request
class vtkMPIController;
#endif

#include "vtkCompositeRenderManager.h"

class vtkDataArray;
class vtkFloatArray;
class vtkRenderWindowInteractor;

class VTK_EXPORT vtkPVTreeComposite : public vtkCompositeRenderManager
{
public:
  static vtkPVTreeComposite *New();
  vtkTypeRevisionMacro(vtkPVTreeComposite,vtkCompositeRenderManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Used by call backs.  Not intended to be called by the user.
  // Empty methods that can be used by the subclass to interupt a parallel render.
  virtual void CheckForAbortRender();
  virtual int CheckForAbortComposite();
    
  // Description:
  // This flag is on by default.
  // If this flag is off, then the behavior of this class becomes
  // that of the superclass (does not check for abort flag).
  vtkSetMacro(EnableAbort, int);
  vtkGetMacro(EnableAbort, int);
  vtkBooleanMacro(EnableAbort, int);

  // Description:
  // Avoid having the cross hairs contribute to the bounds unless
  // it is the only actor.  This method considers Pickable flag.
  virtual void ComputeVisiblePropBounds(vtkRenderer *ren, 
                                        double bounds[6]);


  // Description:
  // Also initialize rmi to check for data.
  virtual void InitializeRMIs();

  // Description:
  // Public because it is an RMI,  It checks to see if data exists
  // on the remote processes before bothering to composite.
  // It uses the supperclasses "UseComposite" flag to disable compositing.
  virtual void StartRender();
  virtual void SatelliteStartRender();
  virtual void WriteFullImage();
  void WriteFullFloatImage();

  // Description:
  // Public because it is an RMI.  
  void CheckForDataRMI();
  virtual void ComputeVisiblePropBoundsRMI();
  
  virtual void SetRenderWindow(vtkRenderWindow *renWin);
  virtual void PostRenderProcessing();

  vtkGetMacro(CompositeTime, double);
  vtkGetMacro(GetBuffersTime, double);
  vtkGetMacro(SetBuffersTime, double);
  vtkGetMacro(MaxRenderTime, double);
  
  // Description:
  // Get the value of the z buffer at a position.
  float GetZ(int x, int y);

  void ExitInteractor();

  void SetUseChar(int useChar);
  vtkGetMacro(UseChar, int);
  vtkBooleanMacro(UseChar, int);
  
  void SetUseRGB(int useRGB);
  vtkGetMacro(UseRGB, int);
  vtkBooleanMacro(UseRGB, int);

  static void ResizeFloatArray(vtkFloatArray *fa, int numComp, vtkIdType size);
  static void ResizeUnsignedCharArray(vtkUnsignedCharArray *uca, int numComp,
                                      vtkIdType size);
  static void DeleteArray(vtkDataArray *da);

protected:
  vtkPVTreeComposite();
  ~vtkPVTreeComposite();

  int  CheckForData();
  int  ShouldIComposite();
  void InternalStartRender();
  void MagnifyFullFloatImage();
  virtual void PreRenderProcessing();

//BTX

  enum Tags {
    CHECK_FOR_DATA_TAG=442445
  };
//ETX

  int EnableAbort;

  int LocalProcessId;
  int RenderAborted;
  // Flag used to indicate the first call for a render.
  // There is no initialize method.
  int Initialized;

  double CompositeTime;
  double GetBuffersTime;
  double SetBuffersTime;
  double MaxRenderTime;
  
//BTX 
#ifdef VTK_USE_MPI 
  void SatelliteFinalAbortCheck();
  void SatelliteAbortCheck();
  void RootAbortCheck();
  void RootFinalAbortCheck();
  void RootWaitForSatelliteToFinish(int satelliteId);
  void RootSendFinalCompositeDecision();
  
  // For the asynchronous receives.
  vtkMPIController *MPIController;
  vtkMPICommunicator::Request ReceiveRequest;
  // Only used on the satellite processes.  When on, the root is 
  // waiting in a blocking receive.  It expects to be pinged so
  // it can check for abort requests.
  int RootWaiting;
  int ReceivePending;
  int ReceiveMessage;
#endif
//ETX  

  vtkRenderWindowInteractor *RenderWindowInteractor;
  void SetRenderWindowInteractor(vtkRenderWindowInteractor *iren);

  void ReallocDataArrays();
  virtual void ReadReducedImage();
  virtual void MagnifyReducedFloatImage();
  virtual void SetRenderWindowFloatPixelData(vtkFloatArray *pixels,
                                             const int pixelDimensions[2]);
  
  int ExitInteractorTag;

  int UseChar;
  int UseRGB;
  
  vtkFloatArray *ReducedFloatImage;
  vtkFloatArray *FullFloatImage;
  vtkFloatArray *TmpFloatPixelData;
  
private:
  vtkPVTreeComposite(const vtkPVTreeComposite&); // Not implemented
  void operator=(const vtkPVTreeComposite&); // Not implemented
};

#endif
