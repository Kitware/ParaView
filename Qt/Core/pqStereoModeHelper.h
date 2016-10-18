/*=========================================================================

   Program: ParaView
   Module:  pqStereoModeHelper.h

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
#ifndef pqStereoModeHelper_h
#define pqStereoModeHelper_h

#include "pqCoreModule.h" // needed for export macros.
#include <QScopedPointer> // needed for QScopedPointer.
#include <QStringList>    // needed for QStringList.

class pqServer;
class pqView;

/**
* pqStereoModeHelper is used to temporarily change stereo mode on all views
* on the specified server.
*
* Often times, one wants to temporarily change the stereo mode for all views
* that support stereo and then restore it back to their previous value e.g.
* when saving screenshots, or animations. This class helps us do that. Simply
* instantiate this class on the stack. In the constructor, it changes the
* stereo mode for all views (or a single view) and restores it back to its original
* value in the destructor.
*/
class PQCORE_EXPORT pqStereoModeHelper
{
public:
  /**
  * Constructor to change stereo mode on all views on a particular
  * server/session.
  *
  * @param stereoMode the new stereo mode to use. 0 for no stereo. For other
  * acceptable values, see vtkRenderWindow.h.
  * @param server the server to use to locate the views to change stereo mode
  * on.
  */
  pqStereoModeHelper(int stereoMode, pqServer* server);

  /**
  * Another constructor to change the stereo mode on a single view rather
  * than all views.
  *
  * @param stereoMode the new stereo mode to use. 0 for no stereo. For other
  * acceptable values, see vtkRenderWindow.h.
  * @param view the view to update the stereo mode on.
  */
  pqStereoModeHelper(int stereoMode, pqView* view);

  virtual ~pqStereoModeHelper();

  /**
  * Helper method to get available stereo modes for a render view.
  *
  * @return a list of labels for available stereo modes.
  */
  static const QStringList& availableStereoModes();

  /**
  * Helper method to convert a stereo mode label to a VTK_STEREO_* value
  * defined in vtkRenderWindow.
  *
  * @return 0 for invalid label or no-stereo, otherwise a positive integer
  * representating the chosen StereoType for a vtkRenderWindow.
  */
  static int stereoMode(const QString& label);

private:
  Q_DISABLE_COPY(pqStereoModeHelper)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
