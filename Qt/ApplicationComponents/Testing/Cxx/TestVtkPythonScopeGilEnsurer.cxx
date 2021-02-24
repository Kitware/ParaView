// Include vtkPython.h first to avoid pythonXY_d.lib not found linking error on
// Windows debug builds.
#include "vtkPython.h"

#include "QApplication"
#include "pqPVApplicationCore.h"
#include "pqPythonManager.h"

#include <iostream>

int main(int argc, char** argv)
{
  // set stdout to line buffering (aka C++ std::cout)
  setvbuf(stdout, (char*)nullptr, _IOLBF, BUFSIZ);

  Py_Initialize(); // Initialize the interpreter
  PyRun_SimpleString("import threading\n");
  PyEval_InitThreads(); // Create (and acquire) the interpreter lock
  PyThreadState* pts = PyGILState_GetThisThreadState();
  PyEval_ReleaseThread(pts);

  // The below should always work (illustration of lock protection)
  {
    vtkPythonScopeGilEnsurer gilEnsurer;

    // Nothing important, just a bunch of calls to some Py* functions!
    PyRun_SimpleString("import base64");
    PyObject* sysmod = PyImport_AddModule("sys");
    PyObject* sysdict = PyModule_GetDict(sysmod);
    PyDict_GetItemString(sysdict, "modules");
  }

  // Now the Qt part:
  QApplication qtapp(argc, argv);

  // And finally the ParaView part:
  pqPVApplicationCore* myCoreApp = new pqPVApplicationCore(argc, argv);

  // Make sure compilation of ParaView was made with Python support:
  pqPythonManager* pythonMgr = myCoreApp->pythonManager();
  if (!pythonMgr)
  {
    std::cerr << "ParaView was built without python, nothing was tested here" << std::endl;
    return EXIT_FAILURE;
  }

  // Initialize Python interpreter
  if (!pythonMgr->initializeInterpreter())
  {
    std::cerr << "The ParaView python interpreter could not be initialized" << std::endl;
    return EXIT_FAILURE;
  }

  // Check initalization
  if (!pythonMgr->interpreterIsInitialized())
  {
    std::cerr << "The python interpreter was not initialized" << std::endl;
    return EXIT_FAILURE;
  }
  delete myCoreApp;

  return EXIT_SUCCESS;
}
