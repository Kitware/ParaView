/*=========================================================================

   Program: ParaView
   Module:    vtkVRUIServerState.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef vtkVRUIServerState_h
#define vtkVRUIServerState_h

#include "vtkSmartPointer.h"
#include "vtkVRUITrackerState.h"
#include <vector>

class vtkVRUIServerState
{
public:
  // Description:
  // Default constructor. All arrays are of size 0.
  vtkVRUIServerState();

  // Description:
  // Return the state of all the trackers.
  std::vector<vtkSmartPointer<vtkVRUITrackerState> >* GetTrackerStates();

  // Description:
  // Return the state of all the buttons.
  std::vector<bool>* GetButtonStates();

  // Description:
  // Return the state of all the valuators (whatever it is).
  std::vector<float>* GetValuatorStates();

protected:
  std::vector<vtkSmartPointer<vtkVRUITrackerState> > TrackerStates;
  std::vector<bool> ButtonStates;
  std::vector<float> ValuatorStates;

private:
  vtkVRUIServerState(const vtkVRUIServerState&) = delete;
  void operator=(const vtkVRUIServerState&) = delete;
};

#endif // #ifndef vtkVRUIServerState_h
