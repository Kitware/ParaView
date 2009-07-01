
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

""" Applying the patch from one branch version to another  """

from gui.utils import getBuilderWindow
from gui.utils import getCurrentVersion
from PyQt4 import QtGui
import CaptureAPI

class PluginPatch(object):
    """ Class methods for patching two versions """

    isStarted = False
    controller = None
    srcStartVersion = 0
    srcEndVersion = 0
    dstVersion = 0
    selectedVersion = 0
    patchOps = []
    failedOps = []
    totalOpsCount = 0

    @classmethod
    def isValidPatch(cls, controller, srcStartVersion, srcEndVersion, dstVersion):
        valid = True
        try:
            valid = controller.vistrail.actionChain(srcEndVersion,srcStartVersion)!=[]
        except:
            valid = False
        
        return valid

    @classmethod
    def start(cls, controller, srcStartVersion, srcEndVersion, dstVersion):
        """ Start the patch """
        if not cls.isStarted:
            cls.isStarted = True
            cls.controller = controller
            cls.srcStartVersion = srcStartVersion
            cls.srcEndVersion = srcEndVersion
            cls.dstVersion = dstVersion
            cls.selectedVersion = getCurrentVersion()
            cls.computePatchingInfo()
            cls.prepareVisTrails()
            cls.performPatching()

    @classmethod
    def computePatchingInfo(cls):
        """ compute operations between srcStart and srcEnd """
        actions=cls.controller.vistrail.actionChain(cls.srcEndVersion,cls.srcStartVersion)
        cls.patchOps=[]
        from plugin.patcher.patch_vistrailcontroller import getPipeline,str2idx
        for action in actions:
            pipeline=getPipeline([action])
            pds=pipeline.plugin_datas
            ops=[]
            for pd in pds:
                indices=str2idx(pd.data)
                ops.append(cls.controller.vistrail.get_string(indices))
            cls.patchOps.append(ops)

    @classmethod
    def prepareVisTrails(cls):
        """ Disable tracking """
        CaptureAPI.setAppTracking(int(False))

    @classmethod
    def performPatching(cls):
        """ Actual run the ops to generate the scene """
        cls.controller.change_selected_version(cls.dstVersion)
        cls.failedOps=[]
        cls.totalOpsCount=0
        for ops in cls.patchOps:
            cls.totalOpsCount+=len(ops)
            for op in ops:
                try:
                    CaptureAPI.unpickleAndPerformOperation(op)
                except:
                    pass
        CaptureAPI.refreshApp()

    @classmethod
    def restoreVisTrails(cls):
        """ Enable back reporting """
        CaptureAPI.setAppTracking(int(True))

    @classmethod
    def getReportText(cls):
        """ Return a string reporting the operations"""
        good = cls.totalOpsCount - len(cls.failedOps)
        text = '%d of %d' % (good, cls.totalOpsCount)
        return text

    @classmethod
    def stop(cls,accepted):
        """ Clean up everything """
        if cls.isStarted:
            cls.isStarted=False
            if accepted:
                cls.controller.change_selected_version(cls.dstVersion)
                for ops in cls.patchOps:
                    opstr=''
                    for op in ops:
                        opstr+=op;
                    cls.controller.update_scene_script('VisTrails Patch',0,opstr)
            else:
                cls.controller.change_selected_version(cls.selectedVersion)
            cls.controller.update_app_with_current_version(getCurrentVersion())
            CaptureAPI.refreshApp()
            cls.restoreVisTrails()
