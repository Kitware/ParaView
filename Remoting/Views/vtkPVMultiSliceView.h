/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiSliceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMultiSliceView
 *
 * vtkPVMultiSliceView extends vtkPVRenderView but add meta-data information
 * used by SliceRepresentation as a data model.
*/

#ifndef vtkPVMultiSliceView_h
#define vtkPVMultiSliceView_h

#include "vtkNew.h" // needed for vtkNew
#include "vtkPVRenderView.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include <vector>                   // needed for std::vector

class vtkClientServerStream;
class vtkMatrix4x4;

class VTKREMOTINGVIEWS_EXPORT vtkPVMultiSliceView : public vtkPVRenderView
{
public:
  static vtkPVMultiSliceView* New();
  vtkTypeMacro(vtkPVMultiSliceView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Update() override;

  void SetNumberOfXSlices(unsigned int count) { this->SetNumberOfSlices(0, count); }
  void SetXSlices(const double* values) { this->SetSlices(0, values); }
  void SetNumberOfYSlices(unsigned int count) { this->SetNumberOfSlices(1, count); }
  void SetYSlices(const double* values) { this->SetSlices(1, values); }
  void SetNumberOfZSlices(unsigned int count) { this->SetNumberOfSlices(2, count); }
  void SetZSlices(const double* values) { this->SetSlices(2, values); }

  const std::vector<double>& GetSlices(int axis) const;
  const std::vector<double>& GetXSlices() const { return this->GetSlices(0); }
  const std::vector<double>& GetYSlices() const { return this->GetSlices(1); }
  const std::vector<double>& GetZSlices() const { return this->GetSlices(2); }

  void GetDataBounds(double bounds[6]) const;

  // These return nullptr when no overrides were specified.
  const char* GetXAxisLabel() const { return this->GetAxisLabel(0); }
  const char* GetYAxisLabel() const { return this->GetAxisLabel(1); }
  const char* GetZAxisLabel() const { return this->GetAxisLabel(2); }

  const vtkClientServerStream& GetAxisLabels() const;

  // Convenience methods used by representations to pass information to the view
  // in vtkPVView::REQUEST_UPDATE() pass. SetAxisTitle can be used to tell the
  // view if the representation's data has information about titles to use for
  // each of the axis. SetDataBounds can be used to tell the view the raw data
  // bounds which are to be use when showing the slice-sliders.
  static void SetAxisTitle(vtkInformation* info, int axis, const char* title);
  static void SetDataBounds(vtkInformation* info, const double bounds[6]);

  void SetModelTransformationMatrix(vtkMatrix4x4*);

protected:
  vtkPVMultiSliceView();
  ~vtkPVMultiSliceView() override;

  void AboutToRenderOnLocalProcess(bool interactive) override;

  void SetNumberOfSlices(int type, unsigned int count);
  void SetSlices(int type, const double* values);
  const char* GetAxisLabel(int axis) const;
  vtkNew<vtkMatrix4x4> ModelTransformationMatrix;

  vtkTimeStamp ModelTransformationMatrixUpdateTime;

private:
  vtkPVMultiSliceView(const vtkPVMultiSliceView&) = delete;
  void operator=(const vtkPVMultiSliceView&) = delete;

  class vtkSliceInternal;
  vtkSliceInternal* Internal;
};

#endif
