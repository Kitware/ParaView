/*=========================================================================

   Program: ParaView
   Module:    pqScalarOpacityFunction.cxx

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

#include "pqScalarOpacityFunction.h"

#include "pqSMAdaptor.h"
#include <QPair>
#include "vtkSMDoubleVectorProperty.h"


pqScalarOpacityFunction::pqScalarOpacityFunction(const QString& group,
  const QString& name, vtkSMProxy* proxy, pqServer* server,
  QObject* parentObject)
: pqProxy(group, name, proxy, server, parentObject)
{
}

pqScalarOpacityFunction::~pqScalarOpacityFunction()
{
}

void pqScalarOpacityFunction::setScalarRange(double min, double max)
{
  vtkSMProxy* opacityFunction = this->getProxy();
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    opacityFunction->GetProperty("Points"));

  QList<QVariant> controlPoints = pqSMAdaptor::getMultipleElementProperty(dvp);
  if (controlPoints.size() == 0)
    {
    return;
    }

  int max_index = dvp->GetNumberOfElementsPerCommand() * (
    (controlPoints.size()-1)/ dvp->GetNumberOfElementsPerCommand());
  QPair<double, double> current_range(controlPoints[0].toDouble(),
    controlPoints[max_index].toDouble());

  // Adjust vtkPiecewiseFunction points to the new range.
  double dold = (current_range.second - current_range.first);
  dold = (dold > 0) ? dold : 1;

  double dnew = (max -min);

  if (dnew > 0)
    {
    double scale = dnew/dold;
    for (int cc=0; cc < controlPoints.size(); 
         cc+= dvp->GetNumberOfElementsPerCommand())
      {
      controlPoints[cc] = 
        scale * (controlPoints[cc].toDouble()-current_range.first) + min;
      }
    }
  else
    {
    // allowing an opacity transfer function with a scalar range of 0.
    // In this case, the piecewise function only contains the endpoints.
    // We are new setting defaults for midPoint (0.5) and sharpness(0.0) 
    controlPoints << 0.0 << 0.0 << 0.5 << 0.0 ;
    controlPoints << 1.0 << 1.0 << 0.5 << 0.0 ;
    }

  pqSMAdaptor::setMultipleElementProperty(dvp, controlPoints);
  opacityFunction->UpdateVTKObjects();
}


