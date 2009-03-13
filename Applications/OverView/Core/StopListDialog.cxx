/*
 * Copyright 2006 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
 /*=========================================================================

  Program:   Visualization Toolkit
  Module:    StopListDialog.cxx 
  Language:  C++
  Date:      $Date$ 
  Version:   $Revision$ 

=========================================================================*/

#include "StopListDialog.h"
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>

static QString
FileErrorToString(QFile::FileError error)
{
  switch (error)
    {
    case QFile::NoError:
      return "No error"; 
    case QFile::ReadError:
      return "An error occurred while reading."; 
    case QFile::WriteError: 
      return "An error occurred while writing."; 
    case QFile::FatalError:
      return "A fatal error occurred."; 
    case QFile::ResourceError:
      return "A resource error occurred."; 
    case QFile::OpenError:
      return "The file could not be opened."; 
    case QFile::AbortError:
      return "The operation was aborted."; 
    case QFile::TimeOutError:
      return "A timeout occurred.";
    case QFile::UnspecifiedError:
      return "An unspecified error occurred. Be very afraid.";
    case QFile::RemoveError:
      return "The file could not be removed.";
    case QFile::RenameError:
      return "The file could not be renamed.";
    case QFile::PositionError:
      return "The position in the file could not be changed.";
    case QFile::ResizeError:
      return "The file could not be resized.";
    case QFile::PermissionsError:
      return "The file could not be accessed.";
    case QFile::CopyError:
      return "The file could not be copied.";
    default:
      return "Unknown error code " + QString::number(error);
    }
}

// ----------------------------------------------------------------------
    

StopListDialog::StopListDialog(QWidget *parent)
  : pqDialog(parent)
{
  this->setupUi(this);

  QObject::connect(this->importPushButton,
       SIGNAL(clicked()),
       this,
       SLOT(handleImportButton()));

  QObject::connect(this->exportPushButton,
       SIGNAL(clicked()),
       this,
       SLOT(handleExportButton()));

  QObject::connect(this->addItemPushButton,
       SIGNAL(clicked()),
       this,
       SLOT(handleAddItemButton()));

  QObject::connect(this->newWordLineEdit,
       SIGNAL(returnPressed()),
       this,
       SLOT(handleAddItemButton()));
/*
  QObject::connect(this->applyButton,
       SIGNAL(clicked()),
       this,
       SLOT(handleApplyButton()));
*/
}

// ----------------------------------------------------------------------

StopListDialog::~StopListDialog()
{
}

// ----------------------------------------------------------------------

QStringList
StopListDialog::stopWords() const
{
  QStringList list;
  for(int i=0; i<this->stopListView->count(); ++i)
    {
    list.push_back(this->stopListView->item(i)->text());
    }

  return list;
}

// ----------------------------------------------------------------------

void
StopListDialog::setStopWords(const QStringList &words)
{
  QStringList mutableList = words;
  mutableList.sort();

  this->stopListView->clear();
  this->stopListView->addItems(words);
}

// ----------------------------------------------------------------------

void
StopListDialog::handleAddItemButton()
{
  QString newItemText = this->newWordLineEdit->text(); 
  if (newItemText.length() > 0)
    {
    if (!this->stopWords().contains(newItemText, Qt::CaseInsensitive))
      {
      QStringList currentWords = this->stopWords();
      currentWords.push_back(newItemText);
      currentWords.sort();
      this->setStopWords(currentWords);
      this->newWordLineEdit->clear();
      }
    }
}

// ----------------------------------------------------------------------

void
StopListDialog::handleImportButton()
{
  bool loadStatus = false;
  int result = 0;

  while (result == 0 && loadStatus == false)
    {
    QString filename = 
      QFileDialog::getOpenFileName(this, "Select file containing stop list");
    QString message;
    
    loadStatus = this->importStopList(filename, &message);
    
    if (!loadStatus)
      {
      QString errorText;
      errorText.sprintf("Error importing from file %s:\n%s\n\n",
      filename.toAscii().data(),
      message.toAscii().data());
      result = QMessageBox::warning(this, "Import Stop List",
            errorText,
            "Retry",
            "Abort", 0, 0, 1);
      }
    // If the user selected 'Retry' we'll get a 0 back from that
    // function and the loop will continue.
    }
}

// ----------------------------------------------------------------------

void
StopListDialog::handleExportButton()
{
  bool saveStatus = false;
  int result = 0;

  while (result == 0 && saveStatus == false)
    {
    QString filename = 
      QFileDialog::getSaveFileName(this, "Select file for exported stop list");
    QString message;
    QString errorText;
    saveStatus = this->exportStopList(filename, &message);
    errorText.sprintf("Error exporting to file %s:\n%s\n\n",
            filename.toAscii().data(),
            message.toAscii().data());
    if (!saveStatus)
      {
      result = QMessageBox::warning(this, "Export Stop List",
            errorText,
            "Retry",
            "Abort", 0, 0, 1);
      }
    // If the user selected 'Retry' we'll get a 0 back from that
    // function and the loop will continue.
    }
}

// ----------------------------------------------------------------------

bool
StopListDialog::importStopList(const QString &filename,
            QString *errorMessage)
{
  QFile infile(filename);
  if (infile.open(QIODevice::ReadOnly) == false)
    {
    if (errorMessage)
      {
      *errorMessage = FileErrorToString(infile.error());
      }
    return false;
    }
  else
    {
    QTextStream instream(&infile);
    QStringList wordList;
    
    QString nextLine = instream.readLine();
    while (nextLine != QString::null)
      {
      wordList.push_back(nextLine.simplified());
      nextLine = instream.readLine();
      }
    infile.close();
    this->setStopWords(wordList);
    }
  return true;
}

// ----------------------------------------------------------------------

bool
StopListDialog::exportStopList(const QString &filename,
            QString *errorMessage)
{
  QFile outfile(filename);
  if (outfile.open(QIODevice::WriteOnly) == false)
    {
    if (errorMessage)
      {
      *errorMessage = FileErrorToString(outfile.error());
      }
    return false;
    }
  else
    {
    QTextStream outstream(&outfile);
    QStringList words = this->stopWords();
    for (int i = 0; i < words.size(); ++i)
      {
      if (words[i].length() > 0)
        {
        outstream << words[i] << "\n";
        }
      }
    outfile.close();
    return true;
    }
}

// ----------------------------------------------------------------------

void
StopListDialog::handleApplyButton()
{
  QStringList words = this->stopWords();
  emit applyStopList(words);
}
