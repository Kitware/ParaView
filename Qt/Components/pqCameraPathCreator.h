/*=========================================================================

   Program: ParaView
   Module:    pqCameraPathCreator.h

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
#ifndef __pqCameraPathCreator_h 
#define __pqCameraPathCreator_h

#include <QDialog>
#include <QList>

#include "pqComponentsExport.h"

class PQCOMPONENTS_EXPORT pqCameraPathCreator : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  pqCameraPathCreator(QWidget* parent=0, Qt::WindowFlags f=0);
  virtual ~pqCameraPathCreator();

  // Overridden to fire pathSelected() is accepted.
  virtual void done(int r);

  static QList<QVariant> createOrbit(const double center[3],
    const double normal[3], double radius, int resolution);
signals:
  void pathSelected(const QList<QVariant>&);
  void closedPath(bool);
  void pathNormal(const QList<QVariant>&);

protected slots:
  void fixedLocation(bool);
  void orbit(bool);
  void setMode(int);

private:
  pqCameraPathCreator(const pqCameraPathCreator&); // Not implemented.
  void operator=(const pqCameraPathCreator&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
  int Mode;
  enum { ORBIT, FIXED_LOCATION };
};

#endif


