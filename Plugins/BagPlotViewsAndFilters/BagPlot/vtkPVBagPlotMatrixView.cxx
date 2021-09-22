/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBagPlotMatrixView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVBagPlotMatrixView.h"

#include "vtkDataObject.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkPVBagPlotMatrixRepresentation.h"
#include "vtkPVStringFormatter.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVBagPlotMatrixView);

//----------------------------------------------------------------------------
void vtkPVBagPlotMatrixView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVBagPlotMatrixView::Render(bool interactive)
{
  std::string formattedTitle = this->GetTitle();

  // push scope with variance
  if (this->GetNumberOfRepresentations() > 0)
  {
    // A representation is available, format the variance in the title
    vtkPVBagPlotMatrixRepresentation* repr =
      vtkPVBagPlotMatrixRepresentation::SafeDownCast(this->GetRepresentation());
    auto variance = static_cast<int>(repr->GetExtractedExplainedVariance());
    vtkPVStringFormatter::PushScope("VIEW", fmt::arg("variance", variance));
  }
  else
  {
    vtkPVStringFormatter::PushScope("VIEW", fmt::arg("variance", std::ref("")));
  }

  // check for old format
  std::string possibleOldFormatString = formattedTitle;
  vtksys::SystemTools::ReplaceString(formattedTitle, "${VARIANCE}", "{variance}");
  if (possibleOldFormatString != formattedTitle)
  {
    vtkLogF(WARNING, "Legacy formatting pattern detected. Please replace '%s' with '%s'.",
      possibleOldFormatString.c_str(), formattedTitle.c_str());
  }
  this->SetTitle(formattedTitle.c_str());

  this->Superclass::Render(interactive);

  // pop scope with variance
  vtkPVStringFormatter::PopScope();
}
