/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeKeeper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeKeeper.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <map>
#include <set>
#include <vector>

class vtkSMTimeKeeper::vtkInternal
{
public:
  typedef std::set<vtkSmartPointer<vtkSMProxy> > ViewsType;
  ViewsType Views;

  typedef std::set<vtkSmartPointer<vtkSMSourceProxy> > SourcesType;

  // Add sources added using AddTimeSource as saved in these two sets. Those
  // that have timesteps are saved in "Sources" while those that don't are saved
  // in "IrrelevantSources". We need IrrelevantSources to keep the reference to
  // the source proxy, otherwise the proxy may get release prematurely when it
  // is being removed from the "TimeSources" property.
  SourcesType Sources;
  SourcesType IrrelevantSources;

  std::set<void*> SuppressedSources;

  typedef std::map<void*, unsigned long> ObserverIdsMap;
  ObserverIdsMap ObserverIds;

  ~vtkInternal() { this->ClearSourcesAndObservers(); }

  void ClearSourcesAndObservers()
  {
    SourcesType::iterator srcIter;
    for (srcIter = this->Sources.begin(); srcIter != this->Sources.end(); ++srcIter)
    {
      ObserverIdsMap::iterator iter = this->ObserverIds.find(srcIter->GetPointer());
      if (iter != this->ObserverIds.end())
      {
        srcIter->GetPointer()->RemoveObserver(iter->second);
        this->ObserverIds.erase(iter);
      }
    }
    this->Sources.clear();
    this->ObserverIds.clear();
    this->IrrelevantSources.clear();
  }
};

vtkStandardNewMacro(vtkSMTimeKeeper);
vtkCxxSetObjectMacro(vtkSMTimeKeeper, TimestepValuesProperty, vtkSMProperty);
vtkCxxSetObjectMacro(vtkSMTimeKeeper, TimeRangeProperty, vtkSMProperty);
vtkCxxSetObjectMacro(vtkSMTimeKeeper, TimeLabelProperty, vtkSMProperty);
//----------------------------------------------------------------------------
vtkSMTimeKeeper::vtkSMTimeKeeper()
{
  this->Time = 0.0;
  this->Internal = new vtkInternal();
  this->TimestepValuesProperty = nullptr;
  this->TimeRangeProperty = nullptr;
  this->TimeLabelProperty = nullptr;
  this->DeferUpdateTimeSteps = false;
}

//----------------------------------------------------------------------------
vtkSMTimeKeeper::~vtkSMTimeKeeper()
{
  delete this->Internal;

  this->SetTimestepValuesProperty(nullptr);
  this->SetTimeRangeProperty(nullptr);
  this->SetTimeLabelProperty(nullptr);
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::AddView(vtkSMProxy* view)
{
  if (view)
  {
    vtkSMDoubleVectorProperty* dvp =
      vtkSMDoubleVectorProperty::SafeDownCast(view->GetProperty("ViewTime"));
    if (!dvp)
    {
      vtkErrorMacro("Failed to locate ViewTime property. Cannot add the view.");
    }
    else
    {
      this->Internal->Views.insert(view);
      dvp->SetElement(0, this->Time);
      view->UpdateProperty("ViewTime");
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::RemoveView(vtkSMProxy* view)
{
  if (view)
  {
    this->Internal->Views.erase(view);
  }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::RemoveAllViews()
{
  this->Internal->Views.clear();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::AddTimeSource(vtkSMSourceProxy* src)
{
  if (!src->GetProperty("TimestepValues") && !src->GetProperty("TimeRange") &&
    !src->GetProperty("TimeLabelAnnotation"))
  {
    this->Internal->IrrelevantSources.insert(src);
    return;
  }

  unsigned long id =
    src->AddObserver(vtkCommand::UpdateInformationEvent, this, &vtkSMTimeKeeper::UpdateTimeSteps);
  this->Internal->Sources.insert(src);
  this->Internal->ObserverIds[src] = id;
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::RemoveTimeSource(vtkSMSourceProxy* src)
{
  vtkInternal::ObserverIdsMap::iterator iter = this->Internal->ObserverIds.find(src);
  if (iter != this->Internal->ObserverIds.end() && src)
  {
    src->RemoveObserver(iter->second);
    this->Internal->ObserverIds.erase(iter);
    this->Internal->Sources.erase(src);
    this->UpdateTimeSteps();
  }
  else
  {
    this->Internal->IrrelevantSources.erase(src);
  }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::RemoveAllTimeSources()
{
  this->Internal->ClearSourcesAndObservers();
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::AddSuppressedTimeSource(vtkSMSourceProxy* src)
{
  this->Internal->SuppressedSources.insert(src);
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::RemoveSuppressedTimeSource(vtkSMSourceProxy* src)
{
  this->Internal->SuppressedSources.erase(src);
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::SetTime(double time)
{
  if (this->Time != time)
  {
    this->Time = time;
    vtkInternal::ViewsType::iterator iter;
    for (iter = this->Internal->Views.begin(); iter != this->Internal->Views.end(); ++iter)
    {
      vtkSMProxy* view = (*iter);
      if (view)
      {
        vtkSMDoubleVectorProperty* dvp =
          vtkSMDoubleVectorProperty::SafeDownCast(view->GetProperty("ViewTime"));
        dvp->SetElement(0, this->Time);
        view->UpdateProperty("ViewTime");
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::UpdateTimeInformation()
{
  const bool prev = this->DeferUpdateTimeSteps;
  this->DeferUpdateTimeSteps = true;
  for (vtkSMSourceProxy* source : this->Internal->Sources)
  {
    source->UpdatePipelineInformation();
  }
  this->DeferUpdateTimeSteps = prev;

  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::UpdateTimeSteps()
{
  if (this->DeferUpdateTimeSteps)
  {
    return;
  }

  std::set<double> timesteps;
  double timerange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  const char* label = nullptr;
  int nbDiffCustomLabel = 0;

  vtkInternal::SourcesType::iterator iter;
  for (iter = this->Internal->Sources.begin(); iter != this->Internal->Sources.end(); ++iter)
  {
    if (this->Internal->SuppressedSources.find(iter->GetPointer()) !=
      this->Internal->SuppressedSources.end())
    {
      // source has been suppressed.
      continue;
    }
    vtkSMDoubleVectorProperty* dvp =
      vtkSMDoubleVectorProperty::SafeDownCast(iter->GetPointer()->GetProperty("TimestepValues"));
    if (dvp)
    {
      unsigned int numElems = dvp->GetNumberOfElements();
      for (unsigned int cc = 0; cc < numElems; cc++)
      {
        double cur_elem = dvp->GetElement(cc);
        timesteps.insert(cur_elem);
        timerange[0] = timerange[0] > cur_elem ? cur_elem : timerange[0];
        timerange[1] = timerange[1] < cur_elem ? cur_elem : timerange[1];
      }
    }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(iter->GetPointer()->GetProperty("TimeRange"));
    if (dvp && dvp->GetNumberOfElements() > 0)
    {
      double cur_elem = dvp->GetElement(0);
      timerange[0] = timerange[0] > cur_elem ? cur_elem : timerange[0];
      timerange[1] = timerange[1] < cur_elem ? cur_elem : timerange[1];

      cur_elem = dvp->GetElement(dvp->GetNumberOfElements() - 1);
      timerange[0] = timerange[0] > cur_elem ? cur_elem : timerange[0];
      timerange[1] = timerange[1] < cur_elem ? cur_elem : timerange[1];
    }

    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      iter->GetPointer()->GetProperty("TimeLabelAnnotation"));
    if (svp && svp->GetNumberOfElements() > 0)
    {
      if (label && strcmp(label, svp->GetElement(0)))
      {
        nbDiffCustomLabel++;
      }
      label = svp->GetElement(0);
    }
  }

  if (timerange[0] == VTK_DOUBLE_MAX && timerange[1] == VTK_DOUBLE_MIN)
  {
    timerange[0] = 0.0;
    timerange[1] = 1.0;
  }

  vtkSMDoubleVectorProperty::SafeDownCast(this->TimeRangeProperty)
    ->SetElements2(timerange[0], timerange[1]);

  std::vector<double> timesteps_vector;
  timesteps_vector.insert(timesteps_vector.begin(), timesteps.begin(), timesteps.end());
  if (timesteps_vector.size() > 0)
  {
    vtkSMDoubleVectorProperty::SafeDownCast(this->TimestepValuesProperty)
      ->SetElements(&timesteps_vector[0], static_cast<unsigned int>(timesteps_vector.size()));
  }
  else
  {
    vtkSMDoubleVectorProperty::SafeDownCast(this->TimestepValuesProperty)->SetNumberOfElements(0);
  }

  // Make sure the label is valid and if several source try to override that
  // label in a different manner, we simply rollback to the default "Time :" value
  vtkSMStringVectorProperty::SafeDownCast(this->TimeLabelProperty)
    ->SetElement(0, (nbDiffCustomLabel > 0 || label == nullptr) ? "Time" : label);
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Time: " << this->Time << endl;
}
