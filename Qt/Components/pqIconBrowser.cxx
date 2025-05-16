// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqIconBrowser.h"

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqIconListModel.h"

#include "ui_pqIconBrowser.h"

#include <QAction>
#include <QDebug>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>

struct pqIconBrowser::pqInternal
{
  pqIconBrowser* Self = nullptr;
  Ui::pqIconBrowser Ui;
  QString CurrentIconPath;
  pqIconListModel* IconModel;
  QSortFilterProxyModel* NameFilter;
  QSortFilterProxyModel* TypeFilter;

  pqInternal(pqIconBrowser* self)
    : Self(self)
    , IconModel(new pqIconListModel(self))
    , NameFilter(new QSortFilterProxyModel(self))
    , TypeFilter(new QSortFilterProxyModel(self))
  {
    this->Ui.setupUi(self);
    auto applyBtn = this->Ui.buttonBox->button(QDialogButtonBox::Ok);
    applyBtn->setObjectName("ApplyIcon");
    auto closeBtn = this->Ui.buttonBox->button(QDialogButtonBox::Close);
    closeBtn->setObjectName("CloseDialog");

    // set up filtering
    this->Ui.filterBox->addItem(tr("All"), "");
    auto origins = pqIconListModel::getAvailableOrigins();
    for (auto origin = origins.begin(); origin != origins.end(); origin++)
    {
      this->Ui.filterBox->addItem(origin.value(), origin.key());
    }

    this->NameFilter->setSourceModel(this->IconModel);
    this->TypeFilter->setSourceModel(this->NameFilter);
    this->Ui.listView->setModel(this->TypeFilter);

    this->NameFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    this->NameFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(this->Ui.searchBox, &pqSearchBox::textChanged,
      [&](const QString& filter) { this->NameFilter->setFilterRegularExpression(filter); });

    this->TypeFilter->setFilterRole(pqIconListModel::OriginRole);
    this->TypeFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
    this->TypeFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(this->Ui.filterBox, QOverload<int>::of(&QComboBox::activated),
      [&]()
      {
        auto type = this->Ui.filterBox->currentData().toString();
        this->TypeFilter->setFilterRegularExpression(type);
      });

    // set up connection to use icon
    QObject::connect(this->Ui.listView, &QListView::doubleClicked,
      [&]()
      {
        this->updateCurrentIconPath();
        this->Self->accept();
      });

    QObject::connect(applyBtn, &QPushButton::released,
      [&]()
      {
        this->updateCurrentIconPath();
        this->Self->accept();
      });

    // import icon
    QObject::connect(this->Ui.importIcon, &QToolButton::released,
      [&]()
      {
        auto iconPath = this->importIconToUserDir();
        auto iconInfo = QFileInfo(iconPath);
        if (iconInfo.exists())
        {
          this->IconModel->addIcon(iconInfo);
          this->TypeFilter->sort(0);
          auto index = this->IconModel->index(this->IconModel->rowCount() - 1);
          auto viewIndex = this->mapFromSource(index);
          this->Ui.listView->selectionModel()->setCurrentIndex(
            viewIndex, QItemSelectionModel::ClearAndSelect);
          this->Ui.listView->scrollTo(viewIndex);
        }
      });

    // remove icon
    QObject::connect(this->Ui.remove, &QToolButton::released,
      [&]()
      {
        this->updateCurrentIconPath();
        this->removeIconFromUserDir(QFileInfo(this->CurrentIconPath));
        this->resetIconList();
      });

    // remove icon
    QObject::connect(this->Ui.removeAll, &QToolButton::released,
      [&]()
      {
        QMessageBox::StandardButton ret = QMessageBox::question(pqCoreUtilities::mainWidget(),
          QCoreApplication::translate("pqIconBrowser", "Delete All"),
          QCoreApplication::translate(
            "pqIconBrowser", "All custom icons will be deleted. Are you sure?"));
        if (ret == QMessageBox::StandardButton::Yes)
        {
          this->removeAllIconsFromUserDir();
          this->resetIconList();
        }
      });

    // selection changed
    QObject::connect(this->Ui.listView->selectionModel(), &QItemSelectionModel::selectionChanged,
      [&]() { this->updateUIState(); });

    this->fillIconList();
  }

  /**
   * Map index from source to view, through proxy filters.
   */
  QModelIndex mapFromSource(const QModelIndex& index)
  {
    auto nameIndex = this->NameFilter->mapFromSource(index);
    return this->TypeFilter->mapFromSource(nameIndex);
  }

  /**
   * Map index to source to view, through proxy filters.
   */
  QModelIndex mapToSource(const QModelIndex& index)
  {
    auto typeIndex = this->TypeFilter->mapToSource(index);
    return this->NameFilter->mapToSource(typeIndex);
  }

  /**
   * Return file path to an image from an open file dialog.
   */
  QString getOpenIconPath()
  {
    auto formats = pqIconListModel::getSupportedIconFormats();
    QStringList extensions;
    for (const auto& format : formats)
    {
      extensions << QString("*%1").arg(format);
    }
    auto filter = QString("%1 (%2)").arg(tr("Images")).arg(extensions.join(" "));

    pqFileDialog fileDialog(
      nullptr, pqCoreUtilities::mainWidget(), tr("Load Icon"), QString(), filter, false, false);
    fileDialog.setObjectName("IconOpenDialog");
    fileDialog.setFileMode(pqFileDialog::ExistingFile);
    if (fileDialog.exec() == QDialog::Accepted)
    {
      return fileDialog.getSelectedFiles()[0];
    }

    return QString();
  }

  /**
   * Import icon to user settings. Returns the new icon path.
   */
  QString importIconToUserDir()
  {
    auto originalFilePath = this->getOpenIconPath();
    auto iconDir = this->getIconUserDirectory();
    auto destinationIconPath = iconDir.absoluteFilePath(QFileInfo(originalFilePath).fileName());
    destinationIconPath = pqCoreUtilities::getNoneExistingFileName(destinationIconPath);
    if (QFile::copy(originalFilePath, destinationIconPath))
    {
      return destinationIconPath;
    }

    return QString();
  }

  void removeAllIconsFromUserDir()
  {
    auto iconDir = this->getIconUserDirectory();
    auto formats = pqIconListModel::getSupportedIconFormats();
    QStringList filter;
    for (const auto& format : formats)
    {
      filter << QString("*%1").arg(format);
    }

    auto iconInfoList = iconDir.entryInfoList(filter);
    for (const auto& iconInfo : iconInfoList)
    {
      QFile::remove(iconInfo.absoluteFilePath());
    }
  }

  void removeIconFromUserDir(const QFileInfo& iconInfo)
  {
    if (iconInfo.exists())
    {
      QFile::remove(iconInfo.absoluteFilePath());
    }
  }

  /**
   * Return the list of Qt resources directory that contain icons.
   */
  QList<QDir> getIconResources()
  {
    QList<QDir> iconDirectories;
    QStringList iconDirNames;
    iconDirNames << "Icons*";

    auto resourceRoot = QDir(":/");
    for (const auto& dirinfo : resourceRoot.entryInfoList(QDir::Dirs))
    {
      auto subdir = QDir(dirinfo.absoluteFilePath());
      auto iconInfos = subdir.entryInfoList(iconDirNames, QDir::Dirs);
      for (const auto& iconDirPath : iconInfos)
      {
        iconDirectories << QDir(iconDirPath.absoluteFilePath());
      }
    }

    return iconDirectories;
  }

  QDir getIconUserDirectory()
  {
    auto iconDirName = QString("Icons");
    QStringList iconDirPaths = pqCoreUtilities::findParaviewPaths(iconDirName, false, true);
    if (iconDirPaths.isEmpty())
    {
      auto userDirPath = pqCoreUtilities::getParaViewUserDirectory();
      auto userDir = QDir(userDirPath);
      if (!userDir.mkdir(iconDirName))
      {
        qWarning() << "unable to create icons directory in user settings";
        return QDir();
      }
      return QDir(userDir.absoluteFilePath(iconDirName));
    }

    return QDir(iconDirPaths.first());
  }

  /**
   * Return a list of icon directory.
   */
  QList<QDir> getIconDirectories()
  {
    QList<QDir> iconDirectories;

    QStringList iconDirPaths = pqCoreUtilities::findParaviewPaths(QString("Icons"), true, true);
    for (const auto& iconDir : iconDirPaths)
    {
      iconDirectories << QDir(iconDir);
    }

    return iconDirectories;
  }

  /**
   * Fill model with icons from available directories.
   */
  void fillIconList()
  {
    auto iconDirs = this->getIconDirectories();
    iconDirs << this->getIconResources();

    auto formats = pqIconListModel::getSupportedIconFormats();
    QStringList filter;
    for (const auto& format : formats)
    {
      filter << QString("*%1").arg(format);
    }

    for (const auto& directory : iconDirs)
    {
      auto iconInfos = directory.entryInfoList(filter);
      this->IconModel->addIcons(iconInfos);
    }
    this->updateUIState();
  }

  void resetIconList()
  {
    this->CurrentIconPath.clear();
    this->IconModel->clearAll();
    this->fillIconList();
    this->updateCurrentIconPath();
  }

  /**
   * Update the selected file path from the current model item.
   * This can be a Qt resource path (i.e. starting with ":")
   */
  void updateCurrentIconPath()
  {
    auto viewIndex = this->Ui.listView->currentIndex();
    auto index = this->mapToSource(viewIndex);
    if (index.isValid())
    {
      this->CurrentIconPath = index.data(pqIconListModel::PathRole).toString();
    }
  }

  /**
   * Enable / disable buttons depending on selected item
   */
  void updateUIState()
  {
    auto selected = this->Ui.listView->selectionModel()->selectedIndexes();
    bool enableRemoval = false;
    if (!selected.empty())
    {
      auto viewIndex = this->Ui.listView->currentIndex();
      auto index = this->mapToSource(viewIndex);
      enableRemoval = pqIconListModel::isUserDefined(index);
    }

    this->Ui.remove->setEnabled(enableRemoval);
  }
};

//-----------------------------------------------------------------------------
pqIconBrowser::pqIconBrowser(QWidget* parent)
  : Superclass(parent)
  , Internal(new pqInternal(this))
{
}

//-----------------------------------------------------------------------------
pqIconBrowser::~pqIconBrowser() = default;

//-----------------------------------------------------------------------------
QString pqIconBrowser::getSelectedIconPath()
{
  return this->Internal->CurrentIconPath;
}

//-----------------------------------------------------------------------------
QString pqIconBrowser::getIconPath(const QString& currentPath)
{
  pqIconBrowser browser(pqCoreUtilities::mainWidget());
  browser.setObjectName("IconBrowser");
  if (browser.exec() == QDialog::Accepted)
  {
    return browser.getSelectedIconPath();
  }

  return currentPath;
}
