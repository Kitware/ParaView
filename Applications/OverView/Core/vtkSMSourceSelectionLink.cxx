/*=========================================================================

   Program: ParaView
   Module:    vtkSMSourceSelectionLink.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkSMSourceSelectionLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMSourceProxy.h"

#include "pqMultiViewFrame.h"
#include "pqView.h"
#include "pqViewManager.h"

#include <QList>

#include <vtksys/stl/set>

class vtkSMSourceSelectionLinkInternals
{
public:
  vtksys_stl::set<vtkSmartPointer<vtkSMSourceProxy> > Sources;
};

class vtkSMSourceSelectionLinkCommand : public vtkCommand
{
public:
  static vtkSMSourceSelectionLinkCommand* New()
    { return new vtkSMSourceSelectionLinkCommand(); }
  void Execute(vtkObject* caller, unsigned long id, void* callData);
  vtkSMSourceSelectionLink* Target;
};

void vtkSMSourceSelectionLinkCommand::Execute(
  vtkObject* caller, unsigned long, void*)
{
  this->Target->SelectionChanged(vtkSMSourceProxy::SafeDownCast(caller));
}

vtkStandardNewMacro(vtkSMSourceSelectionLink);
vtkSMSourceSelectionLink::vtkSMSourceSelectionLink()
{
  this->Internals = new vtkSMSourceSelectionLinkInternals();
  this->Command = vtkSMSourceSelectionLinkCommand::New();
  this->Command->Target = this;
  this->ViewManager = 0;
  this->InSelectionChanged = false;
}

vtkSMSourceSelectionLink::~vtkSMSourceSelectionLink()
{
  delete this->Internals;
  this->Command->Delete();
}

void vtkSMSourceSelectionLink::AddSource(vtkSMSourceProxy* source)
{
  this->Internals->Sources.insert(source);
  source->AddObserver(vtkCommand::SelectionChangedEvent, this->Command);
}

void vtkSMSourceSelectionLink::RemoveSource(vtkSMSourceProxy* source)
{
  this->Internals->Sources.erase(source);
  source->RemoveObserver(this->Command);
}

void vtkSMSourceSelectionLink::SelectionChanged(vtkSMSourceProxy* source)
{
  // Avoid infinite loops
  if (!this->InSelectionChanged)
    {
    this->InSelectionChanged = true;
    vtksys_stl::set<vtkSmartPointer<vtkSMSourceProxy> >::iterator it, itEnd;
    it = this->Internals->Sources.begin();
    itEnd = this->Internals->Sources.end();
    for (; it != itEnd; ++it)
      {
      if (it->GetPointer() != source)
        {
        (*it)->SetSelectionInput(0, source->GetSelectionInput(0), 0);
        }
      }
    if (this->ViewManager)
      {
      QList<pqMultiViewFrame*> list =
        qFindChildren<pqMultiViewFrame*>(this->ViewManager);
      for (int i = 0; i < list.size(); ++i)
        {
        pqView* view = this->ViewManager->getView(list[i]);
        if (view)
          {
          view->render();
          }
        }
      }
    this->InSelectionChanged = false;
    }
}

void vtkSMSourceSelectionLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

