/*=========================================================================

   Program: ParaView
   Module:  pqModelTransformSupportBehavior.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqModelTransformSupportBehavior_h
#define pqModelTransformSupportBehavior_h

#include "pqApplicationComponentsModule.h"
#include "vtkTuple.h"
#include <QObject>

class pqView;
class vtkSMSourceProxy;

/**
* @ingroup Behaviors
* pqModelTransformSupportBehavior is designed for supporting ChangeOfBasis
* matrix for MantId.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqModelTransformSupportBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqModelTransformSupportBehavior(QObject* parent = 0);
  ~pqModelTransformSupportBehavior() override;

  static vtkTuple<double, 16> getChangeOfBasisMatrix(
    vtkSMSourceProxy*, int outputPort = 0, bool* isvalid = nullptr);
  static vtkTuple<double, 6> getBoundingBoxInModelCoordinates(
    vtkSMSourceProxy*, int outputPort = 0, bool* isvalid = nullptr);
  static vtkTuple<std::string, 3> getAxisTitles(
    vtkSMSourceProxy*, int outputPort = 0, bool* isvalid = nullptr);

protected Q_SLOTS:
  virtual void viewAdded(pqView*);
  virtual void viewUpdated();

protected:
  virtual void enableModelTransform(pqView*, vtkSMSourceProxy*);
  virtual void disableModelTransform(pqView*);

private:
  Q_DISABLE_COPY(pqModelTransformSupportBehavior)
};

#endif
