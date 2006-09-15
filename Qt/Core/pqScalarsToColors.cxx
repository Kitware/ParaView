/*=========================================================================

   Program: ParaView
   Module:    pqScalarsToColors.cxx

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
#include "pqScalarsToColors.h"

#include "vtkSMProxy.h"

#include <QPointer>
#include <QList>
#include <QtDebug>

#include "pqSMAdaptor.h"
#include "pqDisplay.h"
#include "pqScalarBarDisplay.h"

//-----------------------------------------------------------------------------
class pqScalarsToColorsInternal
{
public:
  QList<QPointer<pqScalarBarDisplay> > ScalarBars;
};

//-----------------------------------------------------------------------------
pqScalarsToColors::pqScalarsToColors(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->Internal = new pqScalarsToColorsInternal;
}

//-----------------------------------------------------------------------------
pqScalarsToColors::~pqScalarsToColors()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::addScalarBar(pqScalarBarDisplay* sb)
{
  if (this->Internal->ScalarBars.indexOf(sb) == -1)
    {
    this->Internal->ScalarBars.push_back(sb);
    }
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::removeScalarBar(pqScalarBarDisplay* sb)
{
  this->Internal->ScalarBars.removeAll(sb);
}

//-----------------------------------------------------------------------------
pqScalarBarDisplay* pqScalarsToColors::getScalarBar(pqRenderModule* ren) const
{
  foreach(pqScalarBarDisplay* sb, this->Internal->ScalarBars)
    {
    if (sb && sb->shownIn(ren))
      {
      return sb;
      }
    }
  return 0;
}
