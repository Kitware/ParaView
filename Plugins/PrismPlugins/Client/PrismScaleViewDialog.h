/*=========================================================================

Program: ParaView
Module:    PrismScaleViewDialog.h

=========================================================================*/
#ifndef __PrismScaleViewDialog_h 
#define __PrismScaleViewDialog_h

#include <QDialog>
#include <QString>

class PrismView;
class PrismScaleViewDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  /// \c view cannot be NULL.
  PrismScaleViewDialog(PrismView *view,
    QWidget* parent=0, Qt::WindowFlags flags=0);
  virtual ~PrismScaleViewDialog();

  bool hasCustomBounds() const;
  int* scalingMode() const;
  double* customBounds() const;

protected slots:
  void onModeChanged(const QString& mode);
  void onCustomBoundsChanged();

protected
  void modeChanged(const int& pos, const int& value);
  
private:
  class pqInternals;
  pqInternals *Internals;
  
  PrismView *View;
};

#endif
