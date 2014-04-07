/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewSettings.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRenderViewSettings - singleton used to keep track of options for
// vtkPVRenderView.
// .SECTION Description
// vtkPVRenderViewSettings is a singleton used to keep track of selections for
// various configurable parameters used by vtkPVRenderView.
// All class to vtkPVRenderViewSettings::New() returns a reference to the
// singleton instance.

#ifndef __vtkPVRenderViewSettings_h
#define __vtkPVRenderViewSettings_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVRenderViewSettings : public vtkObject
{
public:
  static vtkPVRenderViewSettings* New();
  vtkTypeMacro(vtkPVRenderViewSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Access the singleton.
  static vtkPVRenderViewSettings* GetInstance();

  // Description:
  // Get/Set use display lists.
  void SetUseDisplayLists(bool val);

  enum
    {
    DO_NOTHING =0,
    OFFSET_FACES=1,
    OFFSET_LINES_AND_VERTS=2,
    ZSHIFT=3
    };

  // Description:
  // vtkMapper settings.
  void SetResolveCoincidentTopology(int mode);
  void SetPolygonOffsetParameters(double factor, double units);
  void SetZShift(double a);

  // Description:
  // Set the number of cells (in millions) when the representations show try to
  // use outline by default.
  vtkSetMacro(OutlineThreshold, vtkIdType);
  vtkGetMacro(OutlineThreshold, vtkIdType);

//BTX
protected:
  vtkPVRenderViewSettings();
  ~vtkPVRenderViewSettings();

  vtkIdType OutlineThreshold;
private:
  vtkPVRenderViewSettings(const vtkPVRenderViewSettings&); // Not implemented
  void operator=(const vtkPVRenderViewSettings&); // Not implemented

  static vtkSmartPointer<vtkPVRenderViewSettings> Instance;
//ETX
};

#endif
