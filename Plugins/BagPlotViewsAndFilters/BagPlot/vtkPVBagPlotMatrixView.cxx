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

#include <vtkDataObject.h>
#include <vtkDataRepresentation.h>
#include <vtkObjectFactory.h>

#include <sstream>

#include "vtkPVBagPlotMatrixRepresentation.h"

vtkStandardNewMacro(vtkPVBagPlotMatrixView);

//----------------------------------------------------------------------------
void vtkPVBagPlotMatrixView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
std::string vtkPVBagPlotMatrixView::GetFormattedTitle()
{
  std::string formattedTitle = this->Superclass::GetFormattedTitle();
  if (this->GetNumberOfRepresentations() > 0)
  {
    // A representation is available, format the variance in the title
    vtkPVBagPlotMatrixRepresentation* repr =
      vtkPVBagPlotMatrixRepresentation::SafeDownCast(this->GetRepresentation());
    std::string key = "${VARIANCE}";
    size_t pos = formattedTitle.find(key);
    if (pos != std::string::npos)
    {
      std::ostringstream stream;
      stream << formattedTitle.substr(0, pos)
             << static_cast<int>(repr->GetExtractedExplainedVariance())
             << formattedTitle.substr(pos + key.length());
      formattedTitle = stream.str();
    }
  }
  return formattedTitle;
}
