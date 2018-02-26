/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentedArrayListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentedArrayListDomain.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMRepresentationProxy.h"

#include <cassert>
#include <vtksys/RegularExpression.hxx>

namespace
{
// Given composite data set information, check whether the arrays
// associated with the field data in the leaf blocks have a single
// tuple. We do this to limit which field arrays are available to
// the domain.
bool vtkFieldArrayHasOneTuplePerCompositeDataSetLeaf(
  vtkPVCompositeDataInformation* info, const char* arrayName)
{
  for (unsigned int i = 0; i < info->GetNumberOfChildren(); ++i)
  {
    vtkPVDataInformation* childInfo = info->GetDataInformation(i);
    if (childInfo)
    {
      vtkPVCompositeDataInformation* compositeChildInfo = childInfo->GetCompositeDataInformation();
      if (compositeChildInfo->GetNumberOfChildren() == 0)
      {
        // We have found a leaf in the dataset. Check whether the field
        // array with the given name has just one tuple.
        vtkPVArrayInformation* childArrayInfo =
          childInfo->GetArrayInformation(arrayName, vtkDataObject::FIELD_ASSOCIATION_NONE);
        if (childArrayInfo && childArrayInfo->GetNumberOfTuples() != 1)
        {
          return false;
        }
      }
      else
      {
        // Recurse on the composite data information in the child
        if (!vtkFieldArrayHasOneTuplePerCompositeDataSetLeaf(compositeChildInfo, arrayName))
        {
          return false;
        }
      }
    }
  }

  // If we got here, everything checks out
  return true;
}
}

// Callback to update the RepresentedArrayListDomain
class vtkSMRepresentedArrayListDomainUpdateCommand : public vtkCommand
{
public:
  vtkWeakPointer<vtkSMRepresentedArrayListDomain> Domain;
  typedef vtkCommand Superclass;
  vtkSMRepresentedArrayListDomainUpdateCommand() { this->Domain = NULL; }
  const char* GetClassNameInternal() const override
  {
    return "vtkSMRepresentedArrayListDomainUpdateCommand";
  }
  static vtkSMRepresentedArrayListDomainUpdateCommand* New()
  {
    return new vtkSMRepresentedArrayListDomainUpdateCommand();
  }
  void Execute(vtkObject*, unsigned long, void*) override
  {
    if (this->Domain)
    {
      this->Domain->Update(NULL);
    }
  }
};

vtkStandardNewMacro(vtkSMRepresentedArrayListDomain);
//----------------------------------------------------------------------------
vtkSMRepresentedArrayListDomain::vtkSMRepresentedArrayListDomain()
{
  this->ObserverId = 0;

  // See description in vtkSMRepresentedArrayListDomain::Update().
  this->UseTrueParentForRepresentatedDataInformation = true;

  // The question is whether to just pick the first available array when the
  // "chosen" attribute is not available or not. Opting for not. This keeps us
  // from ending up scalar coloring random data arrays by default. This logic
  // may need to be reconsidered.
  this->PickFirstAvailableArrayByDefault = false;

  // Set up observer on vtkPVRepresentedArrayListSettings so that the domain
  // updates whenever the settings are changed.
  vtkSMRepresentedArrayListDomainUpdateCommand* observer =
    vtkSMRepresentedArrayListDomainUpdateCommand::New();
  observer->Domain = this;

  vtkPVRepresentedArrayListSettings* arrayListSettings =
    vtkPVRepresentedArrayListSettings::GetInstance();
  arrayListSettings->AddObserver(vtkCommand::ModifiedEvent, observer);
  observer->FastDelete();
}

//----------------------------------------------------------------------------
vtkSMRepresentedArrayListDomain::~vtkSMRepresentedArrayListDomain()
{
  this->SetRepresentationProxy(NULL);
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::Update(vtkSMProperty* property)
{
  if (this->RepresentationProxy == NULL)
  {
    // When the update happens the first time, save a reference to the
    // representation proxy and add observers so that we can monitor the
    // representation updates.
    vtkSMRepresentationProxy* selfProxy = (this->GetProperty()
        ? vtkSMRepresentationProxy::SafeDownCast(this->GetProperty()->GetParent())
        : NULL);

    // BUG #15586. This is a tricky issue. The problem is that the
    // PVRepresentationBase (which is a composite representation comprising of
    // other representations) exposes the "ColorArrayName" property from one of
    // its subproxies. That ensures proper grouping and placement of this
    // property's widget in the UI. However, the problem is that the domain for
    // this property needs to be updated based on the representated-data
    // information of the composite-representation rather than the internal
    // representation (which is what was causing the reported bug, since we always
    // ended up getting represented-data information from the "Surface
    // Representation". Adding a mechanism to tell the domain to use the
    // represented-data information from the outer most representation, overcomes
    // this issue with less XML tricks and better backwards compatibility.
    vtkSMRepresentationProxy* outerMostRepresentation =
      (selfProxy && this->UseTrueParentForRepresentatedDataInformation)
      ? vtkSMRepresentationProxy::SafeDownCast(selfProxy->GetTrueParentProxy())
      : NULL;

    this->SetRepresentationProxy(outerMostRepresentation ? outerMostRepresentation : selfProxy);
  }
  this->Superclass::Update(property);
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::SetRepresentationProxy(vtkSMRepresentationProxy* repr)
{
  if (this->RepresentationProxy != repr)
  {
    if (this->RepresentationProxy && this->ObserverId)
    {
      this->RepresentationProxy->RemoveObserver(this->ObserverId);
    }
    this->RepresentationProxy = repr;
    if (repr)
    {
      this->ObserverId = this->RepresentationProxy->AddObserver(vtkCommand::UpdateDataEvent, this,
        &vtkSMRepresentedArrayListDomain::OnRepresentationDataUpdated);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::OnRepresentationDataUpdated()
{
  this->Update(NULL);
}

//----------------------------------------------------------------------------
bool vtkSMRepresentedArrayListDomain::IsFilteredArray(
  vtkPVDataInformation* info, int association, const char* name)
{
  // NOTE: Return `true` to reject.
  vtkPVRepresentedArrayListSettings* colorArraySettings =
    vtkPVRepresentedArrayListSettings::GetInstance();
  for (int idx = 0; idx < colorArraySettings->GetNumberOfFilterExpressions(); ++idx)
  {
    std::string filterExpression = colorArraySettings->GetFilterExpression(idx);
    vtksys::RegularExpression re(filterExpression);
    bool matches = re.find(name);
    if (matches)
    {
      // filter i.e. remove the array.
      return true;
    }
  }

  // If the array is a field data array and the data is a composite datasets,
  // we need to ensure it has exactly as many tuples as the blocks in dataset
  // for coloring.
  if (association == vtkDataObject::FIELD_ASSOCIATION_NONE && info->GetCompositeDataSetType() >= 0)
  {
    vtkPVCompositeDataInformation* cdi = info->GetCompositeDataInformation();
    assert(cdi);
    return !vtkFieldArrayHasOneTuplePerCompositeDataSetLeaf(cdi, name);
  }

  // don't filter.
  return false;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentedArrayListDomain::GetExtraDataInformation()
{
  // This extra care is needed for vtkSMRepresentedArrayListDomain since
  // this->RepresentationProxy can be a parent proxy in which case, when loading
  // state or undoing/redoing, the Update() on the domain can get called
  // prematurely -- before the outer proxy's state has been updated.
  // GatheringInformation in that case can force the proxy to be created
  // messing up the state of other subproxies yet to be deserialized!
  return this->RepresentationProxy && this->RepresentationProxy->GetObjectsCreated()
    ? this->RepresentationProxy->GetRepresentedDataInformation()
    : NULL;
}

//----------------------------------------------------------------------------
int vtkSMRepresentedArrayListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem)
{
  if (!this->Superclass::ReadXMLAttributes(prop, elem))
  {
    return 0;
  }

  int use_true_parent = 1;
  if (elem->GetScalarAttribute("use_true_parent", &use_true_parent) != 0 && use_true_parent == 0)
  {
    this->SetUseTrueParentForRepresentatedDataInformation(false);
  }
  else
  {
    this->SetUseTrueParentForRepresentatedDataInformation(true);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseTrueParentForRepresentatedDataInformation: "
     << this->UseTrueParentForRepresentatedDataInformation << endl;
}
