/*=========================================================================

   Program: ParaView
   Module:    pqTextWidgetDisplay.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqTextWidgetDisplay.h"

#include "vtkSmartPointer.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include "pqPipelineSource.h"

class pqTextWidgetDisplayInternal
{
public:
  vtkSmartPointer<vtkSMPropertyLink> InputLink;
};

//-----------------------------------------------------------------------------
pqTextWidgetDisplay::pqTextWidgetDisplay(const QString& group, 
  const QString& name, vtkSMProxy* display, pqServer* server,
    QObject* _parent): 
  pqConsumerDisplay(group, name, display, server, _parent)
{
  this->Internal = new pqTextWidgetDisplayInternal;
};

//-----------------------------------------------------------------------------
pqTextWidgetDisplay::~pqTextWidgetDisplay()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqTextWidgetDisplay::onInputChanged()
{
  this->Superclass::onInputChanged();
  this->Internal->InputLink = 0;
  pqPipelineSource* input = this->getInput();
  if (input)
    {
    vtkSMPropertyLink* link = vtkSMPropertyLink::New();
    link->AddLinkedProperty(
      input->getProxy(), "Text", vtkSMLink::INPUT);
    link->AddLinkedProperty(
      this->getProxy(), "Text", vtkSMLink::OUTPUT);
    this->Internal->InputLink = link;
    link->Delete();
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
