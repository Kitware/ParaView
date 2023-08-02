// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqQVTKWidget_h
#define pqQVTKWidget_h

#include "QVTKInteractor.h"
#include "pqCoreModule.h"
#include "pqQVTKWidgetBase.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QVariant>
#include <QWidget>

class vtkSMProxy;
class vtkSMSession;

/**
 * @class pqQVTKWidget
 * @brief QWidget subclass to show rendering results from vtkSMViewProxy.
 *
 * pqQVTKWidget is used by ParaView to show rendering results from a
 * vtkSMViewProxy (or subclass) in QWidget. Internally, it uses a
 * QVTKOpenGLNativeWidget, or QVTKOpenGLStereoWidget based on whether stereo
 * mode is enabled and supported on the platform.
 *
 * This class adds awareness for view proxies to the widget it owns.
 * The advantage of doing so is that pqQVTKWidget can automatically update the
 * "ViewSize" property on the view proxy whenever the widget's size/position changes.
 */
class PQCORE_EXPORT pqQVTKWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqQVTKWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f, bool isStereo);
  ~pqQVTKWidget() override;

  /**
   * Set the view proxy.
   */
  void setViewProxy(vtkSMProxy*);

  /**
   * Set the session.
   * This is only used when ViewProxy is not set.
   */
  void setSession(vtkSMSession*);

  /**
   * Return the Proxy ID if any, otherwise return 0
   */
  vtkTypeUInt32 getProxyId();

  /**
   * Set/Get the name of the property to use to update the size of the widget
   * on the proxy. By default "ViewSize" is used.
   */
  void setSizePropertyName(const QString& pname) { this->SizePropertyName = pname; }
  const QString& sizePropertyName() const { return this->SizePropertyName; }

  void notifyQApplication(QMouseEvent*);

  /**
   * Methods that decorate QVTKOpenGL*Widget methods
   */
  void setRenderWindow(vtkRenderWindow* win);
  vtkRenderWindow* renderWindow() const;

  QVTKInteractor* interactor() const;
  bool isValid();

  void setEnableHiDPI(bool flag);
  void setCustomDevicePixelRatio(double cdpr);
  double effectiveDevicePixelRatio() const;
  void setViewSize(int width, int height);

  /**
   * Provide access to the internal QWidget subclass used for actual rendering.
   * This may be QVTKOpenGLStereoWidget or QVTKOpenGLNativeWidget
   */
  QWidget* renderWidget() const;

  ///@{
  /**
   * Set / get the cursor shape used when the mouse is over the widget.
   * You should use these functions instead of QWidget::setCursor / QWidget::cursor.
   * The reason is that, using original ones do not work properly when ParaView is
   * launched with stereo mode activated.
   */
  void setCursorCustom(const QCursor& cursor);
  QCursor cursorCustom() const;
  ///@}

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void paintMousePointer(int x, int y);

private Q_SLOTS:
  void prepareContextForRendering();

protected:
  bool renderVTK();
  bool canRender();

#if PARAVIEW_USING_QVTKOPENGLSTEREOWIDGET
  void resizeEvent(QResizeEvent* evt) override;
#endif

private:
  Q_DISABLE_COPY(pqQVTKWidget)
  vtkSmartPointer<vtkSMProxy> ViewProxy;
  vtkWeakPointer<vtkSMSession> Session;
  QString SizePropertyName;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

  bool useStereo;
  QVariant baseClass;
};

class vtkGenericOpenGLRenderWindow;
typedef vtkGenericOpenGLRenderWindow pqQVTKWidgetBaseRenderWindowType;

#endif
