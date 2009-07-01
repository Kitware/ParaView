
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
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################

##############################################################################
# Patch VistrailController
from PyQt4 import QtCore, QtGui
from core.log.log import Log
from core.vistrail.action import Action
from core.vistrail.operation import AddOp
from core.vistrail.plugin_data import PluginData
from core.vistrail.pipeline import Pipeline
from core.vistrails_tree_layout_lw import VistrailsTreeLayoutLW
from db.domain import DBWorkflow
from db.services.vistrail import performActions
import core.db.action
import cPickle
import CaptureAPI
import sys

def VC___init__(self, vis=None, auto_save=True, name=''):
    """ VistrailController(vis: Vistrail, name: str) -> VistrailController
    Create a controller from vis

    """
    QtCore.QObject.__init__(self)
    self.name = ''
    self.file_name = ''
    self.set_file_name(name)
    self.vistrail = vis
    self.log = Log()
    self.current_version = -1
    self.current_pipeline = None
    self.current_pipeline_view = None
    self.vistrail_view = None
    self.reset_pipeline_view = False
    self.reset_version_view = True
    self.quiet = False
    self.search = None
    self.search_str = None
    self.refine = False
    self.changed = False
    self.full_tree = False
    self.analogy = {}
    self._auto_save = auto_save
    self.locator = None
    self.timer = QtCore.QTimer(self)
    self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.write_temporary)
    self.timer.start(1000 * 60 * 2) # Save every two minutes
    self.need_view_adjust = True
    self.playback_start = -1
    self.playback_end = -1
    self._current_graph_layout = VistrailsTreeLayoutLW()
    self.animate_layout = False
    self.presetOps = ''
    self.num_versions_always_shown = int(CaptureAPI.getPreference('VisTrailsNumberOfVisibleVersions'))
    self.snapshot_action_count = 0

def VC_change_selected_version(self, newVersion):
    if self.current_version==newVersion: return
    old_version = self.current_version
    self.current_version = newVersion
    if newVersion>=0:
        try:
            self.current_pipeline = self.vistrail.getPipeline(newVersion)
            self.current_pipeline.ensure_connection_specs()
        except: 
            from gui.application import VistrailsApplication
            QtGui.QMessageBox.critical(VistrailsApplication.builderWindow,
                                       'Versioning Error',
                                       ('Cannot open selected version because of unsupported operations'))
            self.current_pipeline = None
            self.current_version = 0
    else:
        self.current_pipeline = None
    # Bail out for now
    self.recompute_terse_graph()
    self.invalidate_version_tree(False)
    self.ensure_version_in_view(newVersion)
    self.emit(QtCore.SIGNAL('versionWasChanged'), newVersion)
    #### Host Application BEGIN
    self.update_app_with_current_version(old_version)
    #### Host Application END

def idx2str(indices):
    return str(indices[0]) + ':' + str(indices[1])

def str2idx(s):
    return map(int, s.split(':'))

def VC_update_scene_script(self, description, snapshot, script):
    """ update_scene_script(description: string, snapshot: int, script: string) -> None

    """
    if self.current_pipeline:
        indices = self.vistrail.store_string(script)
        p = PluginData(id=self.vistrail.idScope.getNewId(PluginData.vtType),
                       data = idx2str(indices))
        add_op = AddOp(id=self.vistrail.idScope.getNewId(AddOp.vtType),
                       what=PluginData.vtType,
                       objectId=p.id,
                       data=p)
        action = Action(id=self.vistrail.idScope.getNewId(Action.vtType),
                        operations=[add_op])

        self.add_new_action(action)
        self.vistrail.change_description(description, self.current_version)
        self.vistrail.change_snapshot(snapshot, self.current_version)
        self.perform_action(action)
        self.reset_pipeline_view = True
        self.emit(QtCore.SIGNAL('versionWasChanged'), self.current_version)
        self.reset_pipeline_view = False
        self.recompute_terse_graph()
        self.invalidate_version_tree(False)
        self.ensure_version_in_view(self.current_version)
        self.emit(QtCore.SIGNAL('scene_updated'))
        if not snapshot:
            self.snapshot_action_count += 1
            if int(CaptureAPI.getPreference('VisTrailsSnapshotEnabled')) and \
                    self.snapshot_action_count >= int(CaptureAPI.getPreference('VisTrailsSnapshotCount')):
                self.snapshot_action_count = 0
                CaptureAPI.createSnapshot(False)

def VC_getOpsFromPipeline(self, pipeline):
    ops = ''
    pds = pipeline.plugin_datas
    for pd in pds:
        indices = str2idx(pd.data)
        ops+=self.vistrail.get_string(indices)
    return ops

def getPipeline(actions):
    workflow = DBWorkflow()
    performActions(actions, workflow)
    Pipeline.convert(workflow)
    return workflow

def VC_get_latest_snapshot_index(self):
    """ get_latest_snapshot_index() -> int 
    Traverse the actions in the current pipeline in reverse order and return the
    index of the last snapshot.

    """
    actions = self.vistrail.actionChain(self.current_version)
    length = len(actions)
    for i in xrange(length-1,-1,-1):
        if self.vistrail.get_snapshot(actions[i].id):
            return i
    return 0


def VC_extract_ops(self, commonVersion, oldVersion, newVersion, includePreset=True, brief=True):
    """ It is very naive now, there are 3 get pipeline calls. In the
    future we need to convert actions directly into application modules to
    avoid wasting memory. But it is good for now with partial updates
    on application side. """
    pNew = getPipeline(self.vistrail.actionChain(newVersion, commonVersion))
    if brief and commonVersion==oldVersion:
        return ('', '', self.getOpsFromPipeline(pNew))
    pShared = self.vistrail.getPipelineVersionNumber(commonVersion)
    pOld = getPipeline(self.vistrail.actionChain(oldVersion, commonVersion))
    sharedOps = self.getOpsFromPipeline(pShared)
    if includePreset:
        sharedOps = self.presetOps + sharedOps
    return (sharedOps,
            self.getOpsFromPipeline(pOld),
            self.getOpsFromPipeline(pNew))

def VC_extract_ops_per_version(self, startVersion, endVersion):
    """ Similar to the extract_ops but categorize all of them into a
    series of ops for each version """
    actions = self.vistrail.actionChain(endVersion, startVersion)
    ops = [self.getOpsFromPipeline(getPipeline([action]))
           for action in actions]
    return ops

def VC_diff_ops_linear(self, v1, v2):
    """ Return the list of ops changes from version v1 to version v2 """
    result = {}
    v = v2
    while 1:
        if v==v1:
            result[v] = None
            break
        if v==0: return None
        action = self.vistrail.actionMap[v]
        result[v] = self.getOpsFromPipeline(getPipeline([action]))
        v = action.parent
    return result

def VC_update_app_with_current_version(self, oldVersion = 0, partialUpdates=True):
    if self.current_pipeline:
        from gui.application import VistrailsApplication
        if VistrailsApplication.builderWindow.updateApp:
            currentVersion = self.current_version
            if CaptureAPI.flushCurrentContext(oldVersion):
                oldVersion = self.current_version
                VistrailsApplication.builderWindow.changeVersionWithoutUpdatingApp(currentVersion)
            commonVersion = self.vistrail.getFirstCommonVersion(oldVersion, currentVersion)
            useCamera = int(CaptureAPI.getPreference('VisTrailsUseRecordedViews'))
            CaptureAPI.updateAppWithCurrentVersion(partialUpdates,commonVersion,oldVersion,currentVersion,int(useCamera))

def VC_ensure_version_in_view(self, newVersion):
    """ Ensure the version is visible in the vistrail_view """
    if self.vistrail_view and newVersion>=0:
        versionView = self.vistrail_view.versionTab.versionView
        if newVersion in versionView.scene().versions:
            versionView.ensureVisible(versionView.scene().versions[newVersion], 0, 0)

def VC_store_preset_attributes(self):    
    """ Inspect the state and store all preset attributes into the
    plugin_info annotation."""
    CaptureAPI.storePresetAttributes()

def VC_finish_store_preset_attributes(self,info):    
    """ Inspect the scene and store all preset attributes into the
    plugin_info annotation."""
    indices=self.vistrail.store_string(info)
    self.vistrail.plugin_info=idx2str(indices)

def VC_load_preset_attributes(self):
    """ Load the preset attributes from the plugin_info annotation """
    if self.vistrail.plugin_info!='':
        indices = str2idx(self.vistrail.plugin_info)
        self.presetOps = self.vistrail.get_string(indices)
                                        
def VC_set_vistrail(self, vistrail, locator):
    original_set_vistrail(self, vistrail, locator)
    self.load_preset_attributes()
    

def VC_cleanup(self):
    locator = self.get_locator()
    if locator:
        locator.clean_temporaries()
    self.vistrail.clean_saved_files()
    self.disconnect(self.timer, QtCore.SIGNAL("timeout()"), self.write_temporary)
    self.timer.stop()

def VC_expand_versions(self, v1, v2):
        """ expand_versions(v1: int, v2: int) -> None
        Expand all versions between v1 and v2
        
        """
        full = self.vistrail.getVersionGraph()
        changed = False
        p = full.parent(v2)
        # Check if all are same
        curr_desc = self.vistrail.get_description(p)
        prev_desc = self.vistrail.get_description(v2)
        expand_all = True
        while p>v1:
            curr_desc = self.vistrail.get_description(p)
            if curr_desc != prev_desc:
                expand_all = False
                break
            p = full.parent(p)
        p = full.parent(v2)
        prev_desc = self.vistrail.get_description(v2)
        while p>v1:
            curr_desc = self.vistrail.get_description(p)
            if expand_all or prev_desc != curr_desc:
                self.vistrail.expandVersion(p)
                changed = True
            p = full.parent(p)
            prev_desc = curr_desc
        if changed:
            self.set_changed(True)
        self.recompute_terse_graph()
        self.invalidate_version_tree(False, True)

import gui.vistrail_controller
gui.vistrail_controller.VistrailController.__init__ = VC___init__
gui.vistrail_controller.VistrailController.change_selected_version = VC_change_selected_version
gui.vistrail_controller.VistrailController.update_scene_script = VC_update_scene_script
gui.vistrail_controller.VistrailController.diff_ops_linear = VC_diff_ops_linear
gui.vistrail_controller.VistrailController.extract_ops_per_version = VC_extract_ops_per_version
gui.vistrail_controller.VistrailController.ensure_version_in_view = VC_ensure_version_in_view
original_set_vistrail = gui.vistrail_controller.VistrailController.set_vistrail
gui.vistrail_controller.VistrailController.set_vistrail = VC_set_vistrail
gui.vistrail_controller.VistrailController.get_latest_snapshot_index = VC_get_latest_snapshot_index

gui.vistrail_controller.VistrailController.update_app_with_current_version = VC_update_app_with_current_version
gui.vistrail_controller.VistrailController.extract_ops = VC_extract_ops
gui.vistrail_controller.VistrailController.store_preset_attributes = VC_store_preset_attributes
gui.vistrail_controller.VistrailController.finish_store_preset_attributes = VC_finish_store_preset_attributes
gui.vistrail_controller.VistrailController.load_preset_attributes = VC_load_preset_attributes
gui.vistrail_controller.VistrailController.getOpsFromPipeline = VC_getOpsFromPipeline
gui.vistrail_controller.VistrailController.cleanup = VC_cleanup
gui.vistrail_controller.VistrailController.expand_versions = VC_expand_versions
