
#ifndef _MyView_h
#define _MyView_h

#include "pqView.h"
#include <QMap>
#include <QColor>
class QLabel;

/// a simple view that shows a QLabel with the display's name in the view
class MyView : public pqView
{
  Q_OBJECT
public:
    /// constructor takes a bunch of init stuff and must have this signature to 
    /// satisfy pqView
  MyView(const QString& viewtypemodule, 
         const QString& group, 
         const QString& name, 
         vtkSMViewProxy* viewmodule, 
         pqServer* server, 
         QObject* p);
  ~MyView();

  /// don't support save images
  bool saveImage(int, int, const QString& ) { return false; }
  vtkImageData* captureImage(int) { return NULL; }
  vtkImageData* captureImage(const QSize&) { return NULL; }

  /// return the QWidget to give to ParaView's view manager
  QWidget* getWidget();

  /// returns whether this view can display the given source
  bool canDisplay(pqOutputPort* opPort) const;

  /// set the background color of this view
  void setBackground(const QColor& col);
  QColor background() const;

protected slots:
  /// helper slots to create labels
  void onRepresentationAdded(pqRepresentation*);
  void onRepresentationRemoved(pqRepresentation*);

protected:

  QWidget* MyWidget;
  QMap<pqRepresentation*, QLabel*> Labels;

};

#endif // _MyView_h

