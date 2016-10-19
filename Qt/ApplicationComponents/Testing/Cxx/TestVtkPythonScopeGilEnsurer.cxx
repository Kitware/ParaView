// Include vtkPython.h first to avoid pythonXY_d.lib not found linking error on
// Windows debug builds.
#include "vtkPython.h"

#include "QApplication"
#include "pqPVApplicationCore.h"

#include <iostream>

int main(int argc, char** argv)
{
  // set stdout to line buffering (aka C++ std::cout)
  setvbuf(stdout, (char*)NULL, _IOLBF, BUFSIZ);

  // Initialize Python
  Py_SetProgramName((char*)("PythonApp"));
  Py_Initialize(); // Initialize the interpreter
  PySys_SetArgv(argc, argv);
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
  if (!myCoreApp->pythonManager())
  {
    std::cerr << "PV init error" << std::endl;
    return EXIT_FAILURE;
  }
  delete myCoreApp;
  return EXIT_SUCCESS;
}
