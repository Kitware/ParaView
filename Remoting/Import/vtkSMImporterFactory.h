// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMImporterFactory
 * @brief   Helper class to import meshes, textures, lights and camera from a file into a render
 * view.
 *
 * vtkSMImporterFactory is a helper class to aid in importing files supported by the VTK importers.
 */

#ifndef vtkSMImporterFactory_h
#define vtkSMImporterFactory_h

#include "vtkRemotingImportModule.h" //needed for exports

#include <string> // for std::string

class vtkSMImporterProxy;
class vtkSMSession;

class VTKREMOTINGIMPORT_EXPORT vtkSMImporterFactory
{
public:
  /**
   * Returns a formatted string with all supported file types.
   * An example returned string would look like:
   * \verbatim
   * "GLTF Files (*.gltf);;OBJ Files (*.obj)"
   * \endverbatim
   */
  static std::string GetSupportedFileTypes(vtkSMSession* session);

  /**
   * Imports the views from the given input file. Returns a new importer instance
   * (or nullptr). Caller must release the returned object explicitly.
   */
  static vtkSMImporterProxy* CreateImporter(const char* filename, vtkSMSession* session);
};
#endif
