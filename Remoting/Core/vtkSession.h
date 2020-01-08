/*=========================================================================

  Program:   ParaView
  Module:    vtkSession.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSession
 *
 * vtkSession defines a session i.e. a conversation, if you will. It can be
 * between different processes or same process. What types of conversations are
 * possible i.e what protocols are supported, is defined by the subclasses.
*/

#ifndef vtkSession_h
#define vtkSession_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class VTKREMOTINGCORE_EXPORT vtkSession : public vtkObject
{
public:
  vtkTypeMacro(vtkSession, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns true is this session is active/alive/valid.
   */
  virtual bool GetIsAlive() = 0;

protected:
  vtkSession();
  ~vtkSession() override;

  // Needed when TileDisplay are retrieved
  friend class vtkSMRenderViewProxy;

  /**
   * Subclasses must call this to mark the session active. This sets the active
   * session pointer held by the vtkProcessModule, making it easier for filters
   * etc. that need information about the active session to access it.
   */
  virtual void Activate();

  /**
   * Subclasses must call this to mark the session inactive. This sets the active
   * session pointer held by the vtkProcessModule, making it easier for filters
   * etc. that need information about the active session to access it.
   */
  virtual void DeActivate();

private:
  vtkSession(const vtkSession&) = delete;
  void operator=(const vtkSession&) = delete;
};

#endif
