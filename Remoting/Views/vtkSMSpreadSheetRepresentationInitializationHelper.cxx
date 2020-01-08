/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationInitializationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSpreadSheetRepresentationInitializationHelper.h"

#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

#include <cassert>

vtkStandardNewMacro(vtkSMSpreadSheetRepresentationInitializationHelper);
//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationInitializationHelper::
  vtkSMSpreadSheetRepresentationInitializationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationInitializationHelper::
  ~vtkSMSpreadSheetRepresentationInitializationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement*, vtkMTimeType ts)
{
  assert(proxy != nullptr);

  vtkSMProperty* cindexProperty = proxy->GetProperty("CompositeDataSetIndex");
  if (cindexProperty == nullptr || cindexProperty->GetMTime() > ts)
  {
    // no CompositeDataSetIndex property, or it has been modified since
    // initialization.
    return;
  }

  vtkSMPropertyHelper inputHelper(proxy, "Input");
  auto inputProxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());
  if (inputProxy == nullptr)
  {
    return;
  }

  // this is a hack to deal with statistic filters that produce a multiblock of
  // tables.
  unsigned int port = inputHelper.GetOutputPort();
  if (vtkPVDataInformation* dataInfo = inputProxy->GetDataInformation(port))
  {
    vtkPVCompositeDataInformation* cdInfo = dataInfo->GetCompositeDataInformation();
    if (!cdInfo->GetDataIsComposite() && dataInfo->GetDataSetType() != VTK_TABLE)
    {
      // data is not a composite dataset of tables , all's good.
      return;
    }

    // see if there are only partial arrays. If so, we go down the tree till we
    // find an index with at least 1 non-partial array.
    vtkNew<vtkPVCompositeDataInformationIterator> iter;
    iter->SetDataInformation(dataInfo);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPVDataInformation* currentDataInfo = iter->GetCurrentDataInformation();
      if (!currentDataInfo)
      {
        continue;
      }

      auto attrInfo = currentDataInfo->GetAttributeInformation(vtkDataObject::ROW);
      for (int cc = 0, max = attrInfo->GetNumberOfArrays(); cc < max; ++cc)
      {
        if (attrInfo->GetArrayInformation(cc) && !attrInfo->GetArrayInformation(cc)->GetIsPartial())
        {
          vtkSMPropertyHelper(cindexProperty).Set(static_cast<int>(iter->GetCurrentFlatIndex()));
          return;
        }
      }
    }
  }
}
