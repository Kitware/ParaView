/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationBatchHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationBatchHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringListDomain.h"

vtkStandardNewMacro(vtkPVAnimationBatchHelper);
vtkCxxRevisionMacro(vtkPVAnimationBatchHelper, "1.2");

//---------------------------------------------------------------------------
void vtkPVAnimationBatchHelper::SetAnimationValueInBatch(
  ofstream* file, vtkSMDomain *domain, vtkSMProperty *property,
  vtkClientServerID sourceID, int idx, double value)
{
  if (!file || !property || !sourceID.ID)
    {
    return;
    }

  if (!strcmp(domain->GetClassName(), "vtkSMDoubleRangeDomain"))
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << property->GetXMLName() << "] SetElement " << idx << " " << value
          << endl;
    }
  else if (!strcmp(domain->GetClassName(), "vtkSMExtentDomain"))
    {
    vtkSMIntVectorProperty *ivp =
      vtkSMIntVectorProperty::SafeDownCast(property);
    if (!ivp)
      {
      return;
      }
 
    int animValue = (int)floor(value + 0.5);
    int compare;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << property->GetXMLName() << "] SetElement " << idx << " "
          << animValue << endl;

    switch (idx)
      {
      case 0:
      case 2:
      case 4:
        compare = ivp->GetElement(idx+1);
        if (animValue > compare)
          {
          *file << "  [$pvTemp" << sourceID << " GetProperty "
                << property->GetXMLName() << "] SetElement " << idx+1 << " "
                << animValue << endl;
          }
        break;
      case 1:
      case 3:
      case 5:
        compare = ivp->GetElement(idx-1);
        if (animValue < compare)
          {
          *file << "  [$pvTemp" << sourceID << " GetProperty "
                << property->GetXMLName() << "] SetElement " << idx-1 << " "
                << animValue << endl;
          }
        break;
      }
    }
  else if (!strcmp(domain->GetClassName(), "vtkSMIntRangeDomain"))
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << property->GetXMLName() << "] SetElement " << idx << " "
          << (int)(floor(value + 0.5)) << endl;
    }
  else if (!strcmp(domain->GetClassName(), "vtkSMStringListDomain"))
    {
    vtkSMStringListDomain *sld = vtkSMStringListDomain::SafeDownCast(domain);
    if (!sld)
      {
      return;
      }
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << property->GetXMLName() << "] SetElement " << idx << " {"
          << sld->GetString((int)(floor(value + 0.5))) << "}" << endl;
    }
  else if (!strcmp(domain->GetClassName(), "vtkSMStringListRangeDomain"))
    {
    char val[128];
    sprintf(val, "%d", static_cast<int>(floor(value + 0.5)));
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << property->GetXMLName() << "] SetElement " << 2*idx+1 << " " << val
          << endl;
    }

  *file << "  $pvTemp" << sourceID << " UpdateVTKObjects" << endl;
}

//---------------------------------------------------------------------------
void vtkPVAnimationBatchHelper::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

