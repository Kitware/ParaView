
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################

from gui.application import VistrailsApplication
_app = VistrailsApplication

##############################################################################
# Exceptions

class NoVistrail(Exception):
    pass

class NoGUI(Exception):
    pass

##############################################################################

def switch_to_pipeline_view():
    """switch_to_pipeline_view():

    Changes current viewing mode to pipeline view in the builder window.

    """
    _app.builderWindow.viewModeChanged(0)

def switch_to_history_view():
    """switch_to_history_view():

    Changes current viewing mode to history view in the builder window.

    """
    _app.builderWindow.viewModeChanged(1)

def switch_to_query_view():
    """switch_to_query_view():

    Changes current viewing mode to query view in the builder window.

    """
    _app.builderWindow.viewModeChanged(2)

################################################################################
# Access to current state

def get_builder_window():
    """get_builder_window():

    returns the main VisTrails GUI window

    raises NoGUI.

    """
    try:
        return _app.builderWindow
    except AttributeError:
        raise NoGUI
    
def get_current_controller():
    """get_current_controller():

    returns the VistrailController of the currently selected vistrail.

    raises NoVistrail.

    """
    try:
        return get_builder_window().viewManager.currentWidget().controller
    except AttributeError:
        raise NoVistrail

def get_current_vistrail():
    """get_current_vistrail():

    Returns the currently selected vistrail.

    """
    return get_current_controller().vistrail

def get_current_vistrail_view():
    """get_current_vistrail():

    Returns the currently selected vistrail view.

    """
    return get_current_controller().vistrail_view

def close_current_vistrail(quiet=False):
    get_builder_window().viewManager.closeVistrail(get_current_vistrail_view())

def get_module_registry():
    import core.modules.module_registry
    return core.modules.module_registry.registry

##############################################################################
# Do things

def add_module(x, y, identifier, name, namespace, controller=None):
    if controller is None:
        controller = get_current_controller()
    result = controller.add_module(x, y, identifier, name, namespace)
    controller.current_pipeline_view.setupScene(controller.current_pipeline)
    return result
    
def add_connection(output_id, output_port, input_id, input_port, 
                   controller=None):
    from core.vistrail.connection import Connection
    # FIXME add_module and add_connection should be analogous
    # add_connection currently works completely differently
    if controller is None:
        controller = get_current_controller()
    connection = Connection.fromPorts(output_port,
                                      input_port)
    connection.sourceId = output_id
    connection.destinationId = input_id
    connection.id = controller.current_pipeline.fresh_connection_id()
    result = controller.add_connection(connection)
    controller.current_pipeline_view.setupScene(controller.current_pipeline)
    return result

def create_group(module_ids, connection_ids, controller=None):
    if controller is None:
        controller = get_current_controller()
    controller.create_group(module_ids, connection_ids, 'Group')
    controller.current_pipeline_view.setupScene(controller.current_pipeline)

##############################################################################

def select_version(version, ctrl=None):
    """select_version(int or str, ctrl=None):

    Given an integer, selects a version with the given number from the
    given vistrail (or the current one if no controller is given).

    Given a string, selects a version with that tag.

    """
    if ctrl is None:
        ctrl = get_current_controller()
    vistrail = ctrl.vistrail
    if type(version) == str:
        version = vistrail.get_tag_by_name(version).id
    ctrl.change_selected_version(version)
    ctrl.invalidate_version_tree(False)

def undo():
    get_current_vistrail_view().undo()

def redo():
    get_current_vistrail_view().redo()

def get_available_versions():
    """get_available_version(): ([int], {int: str})

    From the currently selected vistrail, return all available
    versions and the existing tags.

    """
    ctrl = get_current_controller()
    vistrail = ctrl.vistrail
    return (vistrail.actionMap.keys(),
            dict([(t.time, t.name) for t in vistrail.tagMap.values()]))

def open_vistrail_from_file(filename):
    from core.db.locator import FileLocator

    f = FileLocator(filename)
    
    manager = get_builder_window().viewManager
    view = manager.open_vistrail(f)
    return view

def close_vistrail(view, quiet=True):
    get_builder_window().viewManager.closeVistrail(view, quiet=quiet)

def new_vistrail():
    # Returns VistrailView - remember to be consistent about it..
    result = _app.builderWindow.viewManager.newVistrail(False)
    return result

##############################################################################
# Testing

import unittest
import copy
import random
import gui.utils

class TestAPI(gui.utils.TestVisTrailsGUI):

    def test_close_current_vistrail_no_vistrail(self):
        self.assertRaises(NoVistrail, lambda: get_current_vistrail_view())

    def test_new_vistrail_no_save(self):
        v = new_vistrail()
        import gui.vistrail_view
        assert isinstance(v, gui.vistrail_view.QVistrailView)
        assert not v.controller.changed
        close_vistrail(v)

    def test_new_vistrail_button_states(self):
        assert _app.builderWindow.newVistrailAction.isEnabled()
        assert not _app.builderWindow.closeVistrailAction.isEnabled()
        assert not _app.builderWindow.saveFileAction.isEnabled()
        assert not _app.builderWindow.saveFileAsAction.isEnabled()
        new_vistrail()
        assert _app.builderWindow.newVistrailAction.isEnabled()
        assert _app.builderWindow.closeVistrailAction.isEnabled()
        assert _app.builderWindow.saveFileAction.isEnabled()
        assert _app.builderWindow.saveFileAsAction.isEnabled()

    
    
    
