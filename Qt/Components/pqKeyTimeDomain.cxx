/*=========================================================================

   Program: ParaView
   Module:    pqKeyTimeDomain.cxx

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
#include "pqKeyTimeDomain.h"

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QDoubleSpinBox>

#include "pqAnimationScene.h"
#include "pqSMAdaptor.h"


//-----------------------------------------------------------------------------
pqKeyTimeDomain::pqKeyTimeDomain(QDoubleSpinBox* p, vtkSMProperty* prop, 
  int index)
: pqDoubleSpinBoxDomain(p, prop, index)
{
}

//-----------------------------------------------------------------------------
pqKeyTimeDomain::~pqKeyTimeDomain()
{
}

//-----------------------------------------------------------------------------
void pqKeyTimeDomain::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Scene)
    {
    QObject::disconnect(this->Scene, 0, this, 0);
    }
  this->Scene = scene;
  if (scene)
    {
    QObject::connect(scene, SIGNAL(startTimeChanged()), 
      this, SLOT(internalDomainChanged()));
    QObject::connect(scene, SIGNAL(endTimeChanged()), 
      this, SLOT(internalDomainChanged()));
    this->internalDomainChanged();
    }
}

//-----------------------------------------------------------------------------
void pqKeyTimeDomain::setRange(double min, double max)
{
  if (this->Scene)
    {
    vtkSMProxy* scene = this->Scene->getProxy();
    double start = pqSMAdaptor::getElementProperty(
      scene->GetProperty("StartTime")).toDouble();
    double end = pqSMAdaptor::getElementProperty(
      scene->GetProperty("EndTime")).toDouble();
    if (start != end)
      {
      min = start + min*(end-start);
      max = start + max*(end-start);
      }
    }
  this->Superclass::setRange(min, max);
}


//-----------------------------------------------------------------------------
void pqKeyTimeDomain::setSingleStep(double step)
{
  this->Superclass::setSingleStep(step);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
