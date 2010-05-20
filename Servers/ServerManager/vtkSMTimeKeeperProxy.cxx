/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeKeeperProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeKeeperProxy.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <vtkstd/set>

class vtkSMTimeKeeperProxy::vtkInternal
{
public:
  typedef vtkstd::set<vtkSmartPointer<vtkSMViewProxy> > ViewsType;
  ViewsType Views;

  typedef vtkstd::set<vtkSmartPointer<vtkSMSourceProxy> > SourcesType;
  SourcesType Sources;
};

vtkStandardNewMacro(vtkSMTimeKeeperProxy);
//----------------------------------------------------------------------------
vtkSMTimeKeeperProxy::vtkSMTimeKeeperProxy()
{
  this->Time = 0.0;
  this->Internal = new vtkInternal();
  this->Observer = vtkMakeMemberFunctionCommand(*this,
    &vtkSMTimeKeeperProxy::UpdateTimeSteps);
}

//----------------------------------------------------------------------------
vtkSMTimeKeeperProxy::~vtkSMTimeKeeperProxy()
{
  vtkMemberFunctionCommand<vtkSMTimeKeeperProxy>::SafeDownCast(this->Observer)->Reset();
  this->Observer->Delete();
  this->Observer = 0;
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::AddView(vtkSMViewProxy* view)
{
  if (view)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      view->GetProperty("ViewTime"));
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
void vtkSMTimeKeeperProxy::RemoveView(vtkSMViewProxy* view)
{
  if (view)
    {
    this->Internal->Views.erase(view);
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::RemoveAllViews()
{
  this->Internal->Views.clear();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::AddTimeSource(vtkSMSourceProxy* src)
{
  if (!src->GetProperty("TimestepValues") && !src->GetProperty("TimeRange"))
    {
    return;
    }

  src->AddObserver(vtkCommand::UpdateInformationEvent, this->Observer);

  this->Internal->Sources.insert(src);
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::RemoveTimeSource(vtkSMSourceProxy* src)
{
  this->Internal->Sources.erase(src);
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::RemoveAllTimeSources()
{
  this->Internal->Sources.clear();
  this->UpdateTimeSteps();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::SetTime(double time)
{
  if (this->Time != time)
    {
    this->Time = time;
    vtkInternal::ViewsType::iterator iter;
    for (iter = this->Internal->Views.begin();
      iter != this->Internal->Views.end(); ++iter)
      {
      vtkSMViewProxy* view = (*iter);
      if (view)
        {
        vtkSMDoubleVectorProperty* dvp =
          vtkSMDoubleVectorProperty::SafeDownCast(
            view->GetProperty("ViewTime"));
        dvp->SetElement(0, this->Time);
        view->UpdateProperty("ViewTime");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::UpdateTimeSteps()
{
  vtkstd::set<double> timesteps;
  double timerange[2] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};

  vtkInternal::SourcesType::iterator iter;
  for (iter = this->Internal->Sources.begin();
    iter != this->Internal->Sources.end(); ++iter)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      iter->GetPointer()->GetProperty("TimestepValues"));
    if (dvp)
      {
      unsigned int numElems = dvp->GetNumberOfElements();
      for (unsigned int cc=0; cc < numElems; cc++)
        {
        double cur_elem = dvp->GetElement(cc);
        timesteps.insert(cur_elem);
        timerange[0] = timerange[0] > cur_elem? cur_elem : timerange[0];
        timerange[1] = timerange[1] < cur_elem? cur_elem : timerange[1];
        }
      }

    dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      iter->GetPointer()->GetProperty("TimeRange"));
    if (dvp && dvp->GetNumberOfElements() > 0)
      {
      double cur_elem = dvp->GetElement(0);
      timerange[0] = timerange[0] > cur_elem? cur_elem : timerange[0];
      timerange[1] = timerange[1] < cur_elem? cur_elem : timerange[1];

      cur_elem = dvp->GetElement(dvp->GetNumberOfElements()-1);
      timerange[0] = timerange[0] > cur_elem? cur_elem : timerange[0];
      timerange[1] = timerange[1] < cur_elem? cur_elem : timerange[1];
      }
    }

  double *new_values = new double[timesteps.size() + 1];
  vtkstd::set<double>::iterator iter2;
  unsigned int cc=0;
  //cout << "------------" << endl;
  for (iter2 = timesteps.begin(); iter2 != timesteps.end(); ++iter2, ++cc)
    {
    new_values[cc] = *iter2;
    //cout << setprecision(20)<< *iter2 << endl;
    }

  if (timerange[0] == VTK_DOUBLE_MAX && timerange[1] == VTK_DOUBLE_MIN)
    {
    timerange[0] = 0.0;
    timerange[1] = 1.0;
    }

  vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("TimeRange"))->SetElements2(
    timerange[0], timerange[1]);

  vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("TimestepValues"))->SetElements(new_values, cc);
  delete[] new_values;
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Time: " << this->Time << endl;
}


