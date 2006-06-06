
/// \file pqAddSourceDialog.cxx
/// \date 5/26/2006

#include "pqAddSourceDialog.h"
#include "ui_pqAddSourceDialog.h"
#include "pqSourceInfoModel.h"

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QAbstractProxyModel>
#include <QString>
#include <QStringList>


class pqAddSourceDialogForm : public Ui::pqAddSourceDialog {};


pqAddSourceDialog::pqAddSourceDialog(QWidget *widgetParent)
  : QDialog(widgetParent)
{
  this->Sources = 0;
  this->History = 0;
  this->SourceInfo = 0;
  this->Form = new pqAddSourceDialogForm();
  this->Form->setupUi(this);

  // Set up the change root functionality.
  QObject::connect(this->Form->SourceList,
      SIGNAL(activated(const QModelIndex &)),
      this, SLOT(changeRoot(const QModelIndex &)));
  QObject::connect(this->Form->SourceGroup, SIGNAL(activated(int)),
      this, SLOT(changeRoot(int)));

  // Listen to the button press events.
  QObject::connect(this->Form->OkButton, SIGNAL(clicked()),
      this, SLOT(accept()));
  QObject::connect(this->Form->CancelButton, SIGNAL(clicked()),
      this, SLOT(reject()));
  QObject::connect(this->Form->BackButton, SIGNAL(clicked()),
      this, SLOT(navigateBack()));
  QObject::connect(this->Form->UpButton, SIGNAL(clicked()),
      this, SLOT(navigateUp()));
  QObject::connect(this->Form->NewFolderButton, SIGNAL(clicked()),
      this, SLOT(addFolder()));
  QObject::connect(this->Form->AddToFavoritesButton, SIGNAL(clicked()),
      this, SLOT(addFavorite()));
}

pqAddSourceDialog::~pqAddSourceDialog()
{
  if(this->Form)
    {
    delete this->Form;
    }
}

void pqAddSourceDialog::setSourceList(QAbstractItemModel *sources)
{
  if(sources != this->Sources)
    {
    this->Sources = sources;
    this->Form->SourceList->setModel(this->Sources);

    // Check for a source info model and save the pointer. The source
    // info model can be used to determine if the selected item is a
    // folder or source. Skip all the proxy models to find the 'real'
    // model.
    QAbstractItemModel *model = this->Sources;
    while(model)
      {
      QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(model);
      if(proxy)
        {
        model = proxy->sourceModel();
        }
      else
        {
        break;
        }
      }

    this->SourceInfo = qobject_cast<pqSourceInfoModel *>(model);

    // Set up the path in the combobox.
    this->Form->SourceGroup->clear();
    this->Form->SourceGroup->addItem(QIcon(":/pqWidgets/pqFolder16.png"), "Filters");
    this->Form->SourceGroup->setCurrentIndex(0);

    // Listen to the selection events from the view.
    QItemSelectionModel *selection = this->Form->SourceList->selectionModel();
    QObject::connect(selection,
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(updateFromSources(const QModelIndex &, const QModelIndex &)));
    }
}

void pqAddSourceDialog::setHistoryList(QAbstractListModel *history)
{
  if(history != this->History)
    {
    this->History = history;
    this->Form->SourceHistory->setModel(this->History);

    // Listen to the selection events from the view.
    QItemSelectionModel *selection = this->Form->SourceHistory->selectionModel();
    QObject::connect(selection,
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(updateFromHistory(const QModelIndex &, const QModelIndex &)));
    }
}

void pqAddSourceDialog::getPath(QString &path)
{
  QStringList groups;
  this->getPath(this->Form->SourceList->rootIndex(), groups);
  path = groups.join("/");
}

void pqAddSourceDialog::setPath(const QString &path)
{
  if(!this->Sources)
    {
    return;
    }

  // Split the path apart. Search the model hierarchy for the index
  // with the specified path. Set as much of the path as possible.
  int row = 0;
  int rows = 0;
  QString indexName;
  QModelIndex index;
  QModelIndex rootIndex;
  QStringList groups = path.split("/");
  QStringList::Iterator iter = groups.begin();
  for( ; iter != groups.end(); ++iter)
    {
    // Search the child indexes for the path name.
    rows = this->Sources->rowCount(rootIndex);
    for(row = 0; row < rows; row++)
      {
      index = this->Sources->index(row, 0, rootIndex);
      indexName = this->Sources->data(index, Qt::DisplayRole).toString();
      if(indexName == *iter)
        {
        rootIndex = index;
        break;
        }
      }

    if(row >= rows)
      {
      break;
      }
    }

  if(rootIndex != this->Form->SourceList->rootIndex())
    {
    this->changeRoot(rootIndex);
    }
}

void pqAddSourceDialog::setSource(const QString &name)
{
  this->Form->SourceName->setText(name);
}

void pqAddSourceDialog::navigateBack()
{
}

void pqAddSourceDialog::navigateUp()
{
  if(this->Form->SourceGroup->count() > 1)
    {
    int newIndex = this->Form->SourceGroup->count() - 2;
    this->Form->SourceGroup->setCurrentIndex(newIndex);
    this->changeRoot(newIndex);
    }
}

void pqAddSourceDialog::addFolder()
{
}

void pqAddSourceDialog::addFavorite()
{
}

void pqAddSourceDialog::changeRoot(const QModelIndex &index)
{
  // Check the index. If it is a source, activating it should be
  // the same thing as hitting the ok button.
  if(this->SourceInfo && this->SourceInfo->isSource(index))
    {
    this->accept();
    return;
    }

  // Change the root index for the source list.
  this->Form->SourceList->setRootIndex(index);

  // Get the path for the specified index. Use the path list to set up
  // the group combobox.
  QStringList path;
  this->getPath(index, path);
  path.prepend("Filters");
  this->Form->SourceGroup->clear();
  QIcon folder = QIcon(":/pqWidgets/pqFolder16.png");
  QStringList::Iterator iter = path.begin();
  for( ; iter != path.end(); ++iter)
    {
    this->Form->SourceGroup->addItem(folder, *iter);
    }

  // Select the last path name added as the current group.
  this->Form->SourceGroup->setCurrentIndex(
      this->Form->SourceGroup->count() - 1);
}

void pqAddSourceDialog::changeRoot(int index)
{
  if(!this->Sources || index == this->Form->SourceGroup->count() - 1)
    {
    return;
    }

  // The last index in the combobox represents the current root index
  // int the source list. Change the root index to the one pointed to
  // by the selected combobox index. Remove the combobox items after
  // the selected index as well.
  QModelIndex rootIndex = this->Form->SourceList->rootIndex();
  for(int i = this->Form->SourceGroup->count() - 1; i > index; i--)
    {
    rootIndex = this->Sources->parent(rootIndex);
    this->Form->SourceGroup->removeItem(i);
    }

  // Change the root index for the source list.
  this->Form->SourceList->setRootIndex(rootIndex);
}

void pqAddSourceDialog::updateFromSources(const QModelIndex &current,
    const QModelIndex &)
{
  if(this->Sources)
    {
    // Get the name for the selected index. Set the name when the
    // selected item is a source. Clear the name when the selected
    // item is a folder.
    QString name;
    if(current.isValid())
      {
      if(!this->SourceInfo)
        {
        name = this->Sources->data(current, Qt::DisplayRole).toString();
        }
      else if(this->SourceInfo->isSource(current))
        {
        name = this->Sources->data(current, Qt::DisplayRole).toString();
        }
      }

    // Set the selected name as the text in the line edit.
    this->setSource(name);
    }
}

void pqAddSourceDialog::updateFromHistory(const QModelIndex &current,
    const QModelIndex &)
{
  if(this->History && current.isValid())
    {
    QString name = this->History->data(current, Qt::DisplayRole).toString();
    this->setSource(name);
    }
}

void pqAddSourceDialog::getPath(const QModelIndex &index, QStringList &path)
{
  QModelIndex current = index;
  while(current.isValid())
    {
    path.prepend(this->Sources->data(current, Qt::DisplayRole).toString());
    current = this->Sources->parent(current);
    }
}



