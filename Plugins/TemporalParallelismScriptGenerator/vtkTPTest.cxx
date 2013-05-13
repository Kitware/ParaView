#include "vtkTPTest.h"

#include <vtkObjectFactory.h>

#include <pqApplicationCore.h>
#include <pqFileDialog.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqPythonDialog.h>
#include <pqPythonManager.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>

#include <iostream>

vtkStandardNewMacro(vtkTPTest);

bool vtkTPTest::TestPlugin()
{
  std::cerr << "Plugin is getting executed" << std::endl;

  // Last Page, export the state.
  pqPythonManager* manager = qobject_cast<pqPythonManager*>(
    pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  pqPythonDialog* dialog = 0;
  
  std::cerr << "Running: " << std::endl;
  if (manager)
    {
    dialog = manager->pythonShellDialog();
    }
  if (!dialog)
    {
    qCritical("Failed to locate Python dialog. Cannot save state.");
    return true;
    }

  // mapping from readers and their filenames on the current machine
  // to the filenames on the remote machine
  /*QString reader_inputs_map;
  for (int cc=0; cc < this->Internals->nameWidget->rowCount(); cc++)
    {
    QTableWidgetItem* item0 = this->Internals->nameWidget->item(cc, 0);
    QTableWidgetItem* item1 = this->Internals->nameWidget->item(cc, 1);
    reader_inputs_map +=
      QString(" '%1' : '%2',").arg(item0->text()).arg(item1->text());
    }
  // remove last ","
  reader_inputs_map.chop(1);

  int timeCompartmentSize = this->Internals->timeCompartmentSize->value();
  QString command =
    QString(tp_export_py).arg(export_rendering).arg(reader_inputs_map).arg(rendering_info).arg(timeCompartmentSize).arg(filename);

  dialog->runString(command);*/

  return true;
}
