
#include <QActionGroup>
#include <QString>
class pqServerManagerModelItem;
class pqPipelineSource;

class PrismToolBarActions : public QActionGroup
{
  Q_OBJECT
public:
  PrismToolBarActions(QObject* p);
  ~PrismToolBarActions();

public slots:
  void onGeometryFileOpen();
  void onSESAMEFileOpen();

private slots:
  void onSelectionChanged();
private:
  pqServerManagerModelItem *getActiveObject() const;

 QAction *GeometryViewAction;
 QAction *SesameViewAction;
 pqPipelineSource* createFilterForActiveSource(
  const QString& xmlname);

};

