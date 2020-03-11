/*=========================================================================

  Program:   ParaView
  Module:    vtkOrderedCompositingHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkOrderedCompositingHelper
 * @brief helper to assist in determine process order when rendering
 *
 * vtkOrderedCompositingHelper is used to help determine compositing order for
 * ranks when ordered-compositing is being used.
 */

#ifndef vtkOrderedCompositingHelper_h
#define vtkOrderedCompositingHelper_h

#include "vtkBoundingBox.h" // needed for ivar
#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" //needed for exports

#include <vector>

class vtkBoundingBox;
class vtkCamera;

class VTKREMOTINGVIEWS_EXPORT vtkOrderedCompositingHelper : public vtkObject
{
public:
  static vtkOrderedCompositingHelper* New();
  vtkTypeMacro(vtkOrderedCompositingHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetBoundingBoxes(const std::vector<vtkBoundingBox>& boxes);
  const std::vector<vtkBoundingBox>& GetBoundingBoxes() const { return this->Boxes; }
  const vtkBoundingBox& GetBoundingBox(int index) const;

  std::vector<int> ComputeSortOrder(vtkCamera* camera);
  std::vector<int> ComputeSortOrderInViewDirection(const double directionOfProjection[3]);
  std::vector<int> ComputeSortOrderFromPosition(const double position[3]);

protected:
  vtkOrderedCompositingHelper();
  ~vtkOrderedCompositingHelper();

  std::vector<vtkBoundingBox> Boxes;

private:
  vtkOrderedCompositingHelper(const vtkOrderedCompositingHelper&) = delete;
  void operator=(const vtkOrderedCompositingHelper&) = delete;

  const vtkBoundingBox InvalidBox;
};

#endif
