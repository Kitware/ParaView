#ifndef DoubleLineEdit_h
#define DoubleLineEdit_h

#include <QObject>

class DoubleLineEditTester : public QObject
{
  Q_OBJECT;
private slots:
  void basic();
  void useGlobalPrecision();
  void useGlobalPrecision_data();
};

#endif
