/*=========================================================================

   Program: ParaView
   Module:    pqScalarBarDisplay.h

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
#ifndef __pqScalarBarDisplay_h
#define __pqScalarBarDisplay_h

#include "pqDisplay.h"
class pqScalarBarDisplayInternal;
class pqScalarsToColors;

// PQ object for a scalar bar. Keeps itself connected with the pqScalarsToColors
// object, if any.
class PQCORE_EXPORT pqScalarBarDisplay : public pqDisplay
{
  Q_OBJECT
public:
  pqScalarBarDisplay(const QString& group, const QString& name,
    vtkSMProxy* scalarbar, pqServer* server,
    QObject* parent=0);
  virtual ~pqScalarBarDisplay();

  // Get the lookup table this scalar bar shows, if any.
  pqScalarsToColors* getLookupTable() const;

  // Returns if the status of the visbility property of this display.
  // Note that for a display to be visible in a render module,
  // it must be \c shownIn that render modules as well as 
  // visibility must be set to 1.
  virtual bool isVisible() const;

  // Set the visibility. Note that this affects the visibility of the
  // display in all render modules it is added to, and only in all the
  // render modules it is added to. This method does not call a re-render
  // on the render module, caller must call that explicitly.
  virtual void setVisible(bool visible);

protected slots:
  void onLookupTableModified();
private:
  pqScalarBarDisplayInternal* Internal;
};



#endif

