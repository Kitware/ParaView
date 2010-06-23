/*=========================================================================

   Program: ParaView
   Module:    pqPythonToolsWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include <vtkPython.h> // Python first
#include "QtPythonConfig.h"

#include "pqPythonToolsWidget.h"
#include "ui_pqPythonToolsWidget.h"
#include "pqPythonMacroSupervisor.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonDialog.h"
#include "pqPythonShell.h"
#include "pqPythonManager.h"
#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QCompleter>
#include <QMessageBox>
#include <QScrollArea>
#include <QDirModel>
#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

//----------------------------------------------------------------------------
class pqPythonToolsWidget::pqInternal : public Ui::pqPythonToolsWidget
{
public:
  QString                 ScriptDirectory;
  QDirModel               Model;
  pqPythonScriptEditor*   Editor;
};

//----------------------------------------------------------------------------
pqPythonToolsWidget::pqPythonToolsWidget(QWidget* p) : Superclass(p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  // Set name filters for dir model to only display
  // directories and python scripts
  QStringList nameFilters;
  nameFilters.append("*.py");
  this->Internal->Model.setNameFilters(nameFilters);
  this->Internal->Model.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);

  this->Internal->Editor = new pqPythonScriptEditor(p);
  QObject::connect(this->Internal->Editor, SIGNAL(fileSaved()),
    this, SLOT(onRefreshClicked()));

  // Get the script directory
  QString scriptDir;
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("pqPythonToolsWidget/ScriptDirectory"))
    {
    scriptDir = pqApplicationCore::instance()->settings()->value(
      "pqPythonToolsWidget/ScriptDirectory").toString();
    }
  else
    {
    scriptDir = this->getPVModuleDirectory();
    if (scriptDir.size())
      {
      scriptDir += QDir::separator() + QString("demos");
      }
    }

  this->Internal->ScriptView->setModel(&this->Internal->Model);
  this->Internal->ScriptView->hideColumn(1);
  this->Internal->ScriptView->hideColumn(2);
  this->Internal->ScriptView->hideColumn(3);
  QObject::connect(this->Internal->ScriptView->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
    this, SLOT(selectionChanged(const QModelIndex&)));
  QObject::connect(this->Internal->ScriptView, SIGNAL(activated(const QModelIndex&)),
    this, SLOT(itemActivated(const QModelIndex&)));
  this->setScriptDirectory(scriptDir);

  // Setup a directory completer
  QCompleter *completer = new QCompleter(this);
  completer->setModel(&this->Internal->Model);
  this->Internal->ScriptDirectoryEntry->setCompleter(completer);

  // Trace buttons
  QObject::connect(this->Internal->StartTraceButton, SIGNAL(clicked()),
    this, SLOT(onStartTraceClicked()));
  QObject::connect(this->Internal->StopTraceButton, SIGNAL(clicked()),
    this, SLOT(onStopTraceClicked()));
  QObject::connect(this->Internal->TraceStateButton, SIGNAL(clicked()),
    this, SLOT(onTraceStateClicked()));
  QObject::connect(this->Internal->ShowTraceButton, SIGNAL(clicked()),
    this, SLOT(onShowTraceClicked()));
  QObject::connect(this->Internal->EditTraceButton, SIGNAL(clicked()),
    this, SLOT(onEditTraceClicked()));
  QObject::connect(this->Internal->SaveTraceButton, SIGNAL(clicked()),
    this, SLOT(onSaveTraceClicked()));
  this->Internal->StopTraceButton->setEnabled(0);

  // Listen for when the interpreter is reset so we can reset the trace buttons
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    QObject::connect(pyDiag, SIGNAL(interpreterInitialized()),
      this, SLOT(onInterpreterReset()));
    }


  // Macro tab signals
  QObject::connect(this->Internal->RemoveMacroButton, SIGNAL(clicked()),
    this, SLOT(onRemoveMacroClicked()));
  QObject::connect(this->Internal->MacroListBox, SIGNAL(itemSelectionChanged()),
    this, SLOT(onMacroListSelectionChanged()));
  QObject::connect(this->Internal->MacroListBox, SIGNAL(itemChanged(QListWidgetItem*)),
    this, SLOT(onMacroNameChanged(QListWidgetItem*)));
  this->Internal->RemoveMacroButton->setEnabled(0);

  // Directory view buttons
  QObject::connect(this->Internal->ScriptDirectoryEntry, SIGNAL(editingFinished()),
    this, SLOT(onScriptDirectoryEditFinished()));
  QObject::connect(this->Internal->ChooseDirectoryButton, SIGNAL(clicked()),
    this, SLOT(onChooseDirectoryClicked()));
  QObject::connect(this->Internal->RefreshButton, SIGNAL(clicked()),
    this, SLOT(onRefreshClicked()));
  QObject::connect(this->Internal->RunSelectedButton, SIGNAL(clicked()),
    this, SLOT(onRunSelectedClicked()));
  QObject::connect(this->Internal->AddToMacrosButton, SIGNAL(clicked()),
    this, SLOT(onAddToMacrosClicked()));
  QObject::connect(this->Internal->NewScriptButton, SIGNAL(clicked()),
    this, SLOT(onNewScriptClicked()));
  this->Internal->AddToMacrosButton->setEnabled(0);
  this->Internal->RunSelectedButton->setEnabled(0);

  this->resetMacroList();
}

//----------------------------------------------------------------------------
pqPythonToolsWidget::~pqPythonToolsWidget()
{
  delete this->Internal;
}

// Some util methods
//----------------------------------------------------------------------------
namespace {
QString filenameFromListItem(QListWidgetItem* item)
{
  if (item)
    {
    return item->toolTip();
    }
  return "";
}

QString macroNameFromFileName(const QString& filename)
{
  QString name = QFileInfo(filename).fileName().replace(".py", "");
  if (!name.length())
    {
    name = "Unnamed macro";
    }
  return name;
}
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onInterpreterReset()
{
  this->Internal->StartTraceButton->setEnabled(1);
  this->Internal->TraceStateButton->setEnabled(1);
  this->Internal->StopTraceButton->setEnabled(0);
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onScriptDirectoryEditFinished()
{
  this->setScriptDirectory(this->Internal->ScriptDirectoryEntry->text());
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onMacroListSelectionChanged()
{
  bool enabled = (this->Internal->MacroListBox->selectedItems().size() > 0);
  this->Internal->RemoveMacroButton->setEnabled(enabled);
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onRefreshClicked()
{
  this->Internal->Model.refresh();
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onNewScriptClicked()
{
  this->Internal->Editor->show();
  this->Internal->Editor->raise();
  this->Internal->Editor->activateWindow();
  this->Internal->Editor->newFile();
}

//----------------------------------------------------------------------------
QString pqPythonToolsWidget::scriptDirectory()
{
  return this->Internal->ScriptDirectoryEntry->text();
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::setScriptDirectory(const QString& dir)
{
  if (dir == this->Internal->ScriptDirectory)
    {
    return;
    }

  this->Internal->ScriptDirectoryEntry->setText(dir);
  pqApplicationCore::instance()->settings()->setValue(
    "pqPythonToolsWidget/ScriptDirectory", dir);

  // Setup the treeview only if the script directory is valid
  if (QDir(dir).exists())
    {
    this->Internal->ScriptView->setRootIndex(this->Internal->Model.index(dir));
    this->Internal->Editor->setSaveDialogDefaultDirectory(dir);
    }
  else
    {
    this->Internal->ScriptView->setRootIndex(QModelIndex());
    this->Internal->Editor->setSaveDialogDefaultDirectory(QDir::homePath());
    }
}

//----------------------------------------------------------------------------
pqPythonDialog* pqPythonToolsWidget::pythonShellDialog()
{
  pqPythonManager* manager = qobject_cast<pqPythonManager*>(
    pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  if (manager)
    {
    return manager->pythonShellDialog();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onStartTraceClicked()
{
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("from paraview import smtrace\n"
                      "smtrace.start_trace()\n"
                      "print 'Trace started.'\n");
    this->Internal->StartTraceButton->setEnabled(0);
    this->Internal->TraceStateButton->setEnabled(0);
    this->Internal->StopTraceButton->setEnabled(1);
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onTraceStateClicked()
{
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("from paraview import smstate\n"
                      "smstate.run()\n");
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onStopTraceClicked()
{
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("from paraview import smtrace\n"
                      "smtrace.stop_trace()\n"
                      "print 'Trace stopped.'\n");
    this->Internal->StartTraceButton->setEnabled(1);
    this->Internal->TraceStateButton->setEnabled(1);
    this->Internal->StopTraceButton->setEnabled(0);
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onEditTraceClicked()
{
  QString traceString = this->getTraceString();
  this->Internal->Editor->show();
  this->Internal->Editor->raise();
  this->Internal->Editor->activateWindow();
  if (this->Internal->Editor->newFile())
    {
    this->Internal->Editor->setText(traceString);
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onShowTraceClicked()
{
  QString traceString = this->getTraceString();
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->print("\n#---- Trace output ----#\n" + traceString);
    pyDiag->runString("\n");
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onSaveTraceClicked()
{
  QString traceString = this->getTraceString();
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                            this->scriptDirectory(),
                            tr("Python script (*.py)"));
  if (fileName.isEmpty())
    {
    return;
    }
  if (!fileName.endsWith(".py"))
    {
    fileName.append(".py");
    }

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
    qWarning() << "Could not open file:" << fileName;
    return;
    }

  QTextStream out(&file);
  out << traceString;

  // Refresh our dir view because the script was most likely saved into the 
  // script directory
  this->onRefreshClicked();
}

//----------------------------------------------------------------------------
QString pqPythonToolsWidget::getTraceString()
{
  QString traceString;
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("from paraview import smtrace\n"
                      "__smtraceString = smtrace.get_trace_string()\n");
    pyDiag->shell()->makeCurrent();
    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);
    PyObject* string_object = PyDict_GetItemString(
      global_dict, "__smtraceString");
    char* string_ptr = string_object ? PyString_AsString(string_object) : 0;
    if (string_ptr)
      {
      traceString = string_ptr;
      }
    pyDiag->shell()->releaseControl();
    }
  return traceString;
}

//----------------------------------------------------------------------------
QString pqPythonToolsWidget::getPVModuleDirectory()
{
  QString dirString;
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runString("import os\n"
                      "__pvModuleDirectory = os.path.dirname(paraview.__file__)\n");
    pyDiag->shell()->makeCurrent();
    PyObject* main_module = PyImport_AddModule((char*)"__main__");
    PyObject* global_dict = PyModule_GetDict(main_module);
    PyObject* string_object = PyDict_GetItemString(
      global_dict, "__pvModuleDirectory");
    char* string_ptr = string_object ? PyString_AsString(string_object) : 0;
    if (string_ptr)
      {
      dirString = string_ptr;
      }
    pyDiag->shell()->releaseControl();
    }
  return dirString;
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onRunSelectedClicked()
{
  QModelIndex index = this->Internal->ScriptView->currentIndex();
  QString filepath = this->Internal->Model.filePath(index);
  pqPythonDialog* pyDiag = this->pythonShellDialog();
  if (pyDiag)
    {
    pyDiag->runScript(QStringList(filepath));
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::selectionChanged(const QModelIndex& index)
{
  // If a directory is selected, disable the buttons and return
  if (this->Internal->Model.isDir(index))
    {
    this->Internal->AddToMacrosButton->setEnabled(0);
    this->Internal->RunSelectedButton->setEnabled(0);
    return;
    }

  // Else enable the buttons
  this->Internal->AddToMacrosButton->setEnabled(1);
  this->Internal->RunSelectedButton->setEnabled(1);
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::itemActivated(const QModelIndex& index)
{
  // Ignore directories
  if (this->Internal->Model.isDir(index))
    {
    return;
    }

  // Edit the script
  this->Internal->Editor->show();
  this->Internal->Editor->raise();
  this->Internal->Editor->activateWindow();
  this->Internal->Editor->open(this->Internal->Model.filePath(index));
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::addMacroToListBox(const QString& macroName, const QString& filename)
{
  emit this->addMacroRequested(macroName, filename);

  // Check if we already have a macro for this filename
  for (int i = 0; i < this->Internal->MacroListBox->count(); ++i)
    {
    QListWidgetItem* item = this->Internal->MacroListBox->item(i);
    if (filenameFromListItem(item) == filename)
      {
      item->setText(macroName);
      return;
      }
    }

  QListWidgetItem* item = new QListWidgetItem(macroName);
  item->setToolTip(filename);
  item->setFlags(item->flags() | Qt::ItemIsEditable);
  this->Internal->MacroListBox->addItem(item);
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onRemoveMacroClicked()
{
  QListWidgetItem* item = this->Internal->MacroListBox->takeItem(
    this->Internal->MacroListBox->currentRow());
  if (!item)
    {
    return;
    }

  QString filename = filenameFromListItem(item);
  pqPythonMacroSupervisor::removeStoredMacro(filename);
  emit removeMacroRequested(filename);
  delete item;
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onMacroNameChanged(QListWidgetItem* item)
{
  if (!item)
    {
    return;
    }

  QString newName = item->text();
  QString filename = filenameFromListItem(item);
  if (!newName.length())
    {
    // Set a default name and return, this slot will be called again.
    item->setText("Unnamed macro");
    return;
    }

  // Store the new name, and emit signal to alert of the name change
  pqPythonMacroSupervisor::storeMacro(newName, filename);
  emit this->addMacroRequested(newName, filename);

}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::resetMacroList()
{
  while (this->Internal->MacroListBox->count())
    {
    delete this->Internal->MacroListBox->takeItem(0);
    }

  // Key is filename, value is macro name.
  QMap<QString, QString> macros = pqPythonMacroSupervisor::getStoredMacros();
  QMap<QString, QString>::const_iterator itr;
  for (itr = macros.constBegin(); itr != macros.constEnd(); ++itr)
    {
    this->addMacroToListBox(itr.value(), itr.key());
    }
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onAddToMacrosClicked()
{
  QModelIndex index = this->Internal->ScriptView->currentIndex();
  QString filepath = this->Internal->Model.filePath(index);
  QString macroName = macroNameFromFileName(filepath);

  // Store the macro in pqSettings
  pqPythonMacroSupervisor::storeMacro(macroName, filepath);

  // Add to the listbox
  this->addMacroToListBox(macroName, filepath);
}

//----------------------------------------------------------------------------
void pqPythonToolsWidget::onChooseDirectoryClicked()
{
  QString defaultDir = this->scriptDirectory();
  QString dir = QFileDialog::getExistingDirectory (this->parentWidget(),
    tr("Choose script directory..."), defaultDir);
  if (dir.length())
    {
    this->Internal->ScriptDirectoryEntry->setText(dir);
    this->setScriptDirectory(dir);
    }
}

//----------------------------------------------------------------------------
bool pqPythonToolsWidget::contains(const QString & filePath)
{
  // Return true is filePath is a child of the script root directory
  QModelIndex index = this->Internal->Model.index(filePath);
  QModelIndex rootIndex = this->Internal->ScriptView->rootIndex();
  while (index.isValid())
    {
    index = index.parent();
    if (index == rootIndex)
      {
      return true;
      }
    }
  return false;
}
