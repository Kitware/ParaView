/*=========================================================================

   Program: ParaView
   Module:  pqStereoModeHelper.cxx

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
#include "pqStereoModeHelper.h"

#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

class pqStereoModeHelper::pqInternals
{
  struct MData
  {
    vtkWeakPointer<vtkSMProxy> Proxy;
    int StereoType;
    int StereoRender;
    MData(vtkSMProxy* proxy, int newType)
      : Proxy(proxy)
    {
      if (proxy)
      {
        vtkSMPropertyHelper sr(proxy, "StereoRender", true);
        vtkSMPropertyHelper st(proxy, "StereoType", true);
        this->StereoRender = sr.GetAsInt();
        this->StereoType = st.GetAsInt();
        if ((newType == 0 && this->StereoRender != 0) ||
          (newType != 0 && (this->StereoType != newType || this->StereoRender == 0)))
        {
          sr.Set(newType == 0 ? 0 : 1);
          st.Set(newType == 0 ? this->StereoType : newType);
          this->Proxy->UpdateVTKObjects();
        }
        else
        {
          this->Proxy = NULL;
        }
      }
    }
    void Restore() const
    {
      if (this->Proxy)
      {
        vtkSMPropertyHelper(this->Proxy, "StereoType", true).Set(this->StereoType);
        vtkSMPropertyHelper(this->Proxy, "StereoRender", true).Set(this->StereoRender);
        this->Proxy->UpdateVTKObjects();
      }
    }
  };
  QList<MData> Items;

public:
  void PushStereoMode(vtkSMProxy* view, int type) { this->Items.push_back(MData(view, type)); }

  ~pqInternals()
  {
    BEGIN_UNDO_EXCLUDE();
    foreach (const MData& mdata, this->Items)
    {
      mdata.Restore();
    }
    END_UNDO_EXCLUDE();
  }
};

//-----------------------------------------------------------------------------
pqStereoModeHelper::pqStereoModeHelper(int paramStereoMode, pqServer* server)
  : Internals(new pqStereoModeHelper::pqInternals())
{
  BEGIN_UNDO_EXCLUDE();
  QList<pqView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>(server);
  foreach (pqView* view, views)
  {
    this->Internals->PushStereoMode(view->getProxy(), paramStereoMode);
  }
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
pqStereoModeHelper::pqStereoModeHelper(int paramStereoMode, pqView* view)
  : Internals(new pqStereoModeHelper::pqInternals())
{
  BEGIN_UNDO_EXCLUDE();
  if (view)
  {
    this->Internals->PushStereoMode(view->getProxy(), paramStereoMode);
  }
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
pqStereoModeHelper::~pqStereoModeHelper()
{
}

//-----------------------------------------------------------------------------
const QStringList& pqStereoModeHelper::availableStereoModes()
{
  static QStringList list;
  if (list.size() == 0)
  {
    list << "No Stereo"
         << "Red-Blue"
         << "Interlaced"
         << "Left Eye Only"
         << "Right Eye Only"
         << "Dresden"
         << "Anaglyph"
         << "Checkerboard"
         << "Split Viewport Horizontal";
  }
  return list;
}

//-----------------------------------------------------------------------------
int pqStereoModeHelper::stereoMode(const QString& val)
{
  int idx = pqStereoModeHelper::availableStereoModes().indexOf(val);
  return idx <= 0 ? 0 : (idx + 1); // See the order of stereo types in vtkRenderWindow.h
}
