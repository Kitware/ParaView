// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInteractiveViewLink_h
#define pqInteractiveViewLink_h

#include "pqCoreModule.h"
#include <QObject>

class pqRenderView;
class vtkPVXMLElement;

/**
 * pqInteractiveViewLink is uaed by pqLinksModel to create
 * interactive view links, which are ViewLink allowing to see a view "trough"
 * another view.
 */
class PQCORE_EXPORT pqInteractiveViewLink : public QObject
{
  Q_OBJECT;
  typedef QObject Superclass;

public:
  pqInteractiveViewLink(pqRenderView* displayView, pqRenderView* linkedView, double xPos = 0.375,
    double yPos = 0.375, double xSize = 0.25, double ySize = 0.25);
  ~pqInteractiveViewLink() override;

  // Save this interactive view link in xml node
  virtual void saveXMLState(vtkPVXMLElement* xml);

  // Set/Get the view link opacity, between 0 and 1
  virtual void setOpacity(double opacity);
  virtual double getOpacity();

  // Set/get whether to hide the background of the linked view
  virtual void setHideLinkedViewBackground(bool hide);
  virtual bool getHideLinkedViewBackground();

protected:
  // Generate a correct draw method call from current situation
  // on the back buffer
  virtual void drawViewLink();

  // draw pixels on the display Window at ViewLink position
  virtual void drawViewLink(int setFront);

  // update the linked window
  virtual void renderLinkedView();

  // method to set rendered flag to true
  virtual void linkedWindowRendered();

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  // draw pixels on the front buffer
  virtual void finalRenderDisplayView();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
