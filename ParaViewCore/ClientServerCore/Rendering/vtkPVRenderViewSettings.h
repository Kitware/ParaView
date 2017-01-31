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
/**
 * @class   vtkPVRenderViewSettings
 * @brief   singleton used to keep track of options for
 * vtkPVRenderView.
 *
 * vtkPVRenderViewSettings is a singleton used to keep track of selections for
 * various configurable parameters used by vtkPVRenderView.
 * All class to vtkPVRenderViewSettings::New() returns a reference to the
 * singleton instance.
*/

#ifndef vtkPVRenderViewSettings_h
#define vtkPVRenderViewSettings_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSmartPointer.h"                      // needed for vtkSmartPointer

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVRenderViewSettings : public vtkObject
{
public:
  static vtkPVRenderViewSettings* New();
  vtkTypeMacro(vtkPVRenderViewSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Access the singleton.
   */
  static vtkPVRenderViewSettings* GetInstance();

  /**
   * Get/Set use display lists.
   */
  void SetUseDisplayLists(bool val);

  enum
  {
    DO_NOTHING = 0,
    OFFSET_FACES = 1,
    OFFSET_LINES_AND_VERTS = 2,
    ZSHIFT = 3
  };

  //@{
  /**
   * vtkMapper settings.
   */
  void SetResolveCoincidentTopology(int mode);
  void SetPolygonOffsetParameters(double factor, double units);
  void SetZShift(double a);
  //@}

  //@{
  /**
   * Set the number of cells (in millions) when the representations show try to
   * use outline by default.
   */
  vtkSetMacro(OutlineThreshold, vtkIdType);
  vtkGetMacro(OutlineThreshold, vtkIdType);
  //@}

  //@{
  /**
   * Set the radius in pixels to include for finding the closet point when
   * selecting a point on render view. This only after single point selections
   * i.e. when selecting a regions this radius is not respected.
   */
  vtkSetMacro(PointPickingRadius, int);
  vtkGetMacro(PointPickingRadius, int);
  //@}

  //@{
  /**
   * EXPERIMENTAL: Add ability to disable IceT.
   */
  vtkSetMacro(DisableIceT, bool);
  vtkGetMacro(DisableIceT, bool);
  //@}

protected:
  vtkPVRenderViewSettings();
  ~vtkPVRenderViewSettings();

  vtkIdType OutlineThreshold;
  int PointPickingRadius;
  bool DisableIceT;

private:
  vtkPVRenderViewSettings(const vtkPVRenderViewSettings&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVRenderViewSettings&) VTK_DELETE_FUNCTION;

  static vtkSmartPointer<vtkPVRenderViewSettings> Instance;
};

#endif
