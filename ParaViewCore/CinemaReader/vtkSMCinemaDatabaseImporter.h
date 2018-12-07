/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaDatabaseImporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMCinemaDatabaseImporter
 * @brief helper to import a Cinema database in ParaView
 *
 * vtkSMCinemaDatabaseImporter can be used by client code to import a Cinema
 * database in ParaView. The importer creates multiple proxies for
 * data-producers in the Cinema database.
 */

#ifndef vtkSMCinemaDatabaseImporter_h
#define vtkSMCinemaDatabaseImporter_h

#include "vtkSMObject.h"

#include "vtkPVCinemaReaderModule.h" // for export macros
#include <string>                    // needed for std::string

class vtkPVCinemaDatabaseInformation;
class vtkSMSelfGeneratingSourceProxy;
class vtkSMSession;
class vtkSMViewProxy;

class VTKPVCINEMAREADER_EXPORT vtkSMCinemaDatabaseImporter : public vtkSMObject
{
public:
  static vtkSMCinemaDatabaseImporter* New();
  vtkTypeMacro(vtkSMCinemaDatabaseImporter, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true if the \c session supports Cinema databases. Currently, only
   * **builtin** sessions and single rank client-server sessions are supported.
   * @param[in] session session to check for Cinema importing support.
   * @returns `true` if supported, else `false`.
   */
  virtual bool SupportsCinema(vtkSMSession* session);

  /**
   * Imports a Cinema database and creates sources as needed for objects in the
   * database.
   * @param[in] dbase path to the Cinema database, typically an `info.json` file.
   * @param[in] session session to load the Cinema database on.
   * @param[in] view if provided, view to show cinema sources by default in.
   * @returns `true` on success, else `false`.
   */
  virtual bool ImportCinema(
    const std::string& dbase, vtkSMSession* session, vtkSMViewProxy* view = NULL);

protected:
  vtkSMCinemaDatabaseImporter();
  ~vtkSMCinemaDatabaseImporter() override;

  void AddPropertiesForControls(vtkSMSelfGeneratingSourceProxy* reader,
    const std::string& parametername, const vtkPVCinemaDatabaseInformation* cinfo);

private:
  vtkSMCinemaDatabaseImporter(const vtkSMCinemaDatabaseImporter&) = delete;
  void operator=(const vtkSMCinemaDatabaseImporter&) = delete;
};

#endif
