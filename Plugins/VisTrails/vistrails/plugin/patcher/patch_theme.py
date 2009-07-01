
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
# Patch gui.theme
from core.system import systemType
import gui.theme
import plugin.pgui
from PyQt4 import QtCore, QtGui
from core.utils.color import ColorByName
import plugin.pgui.resources.images_rc

class PluginTheme(object):
    """
    This is the default plug-in theme.  It is kept seperate from the Default theme
    to ensure that we only use our own icons.
    """
    def __init__(self):
        """ PluginTheme() -> PluginTheme
        This is for initializing all icons, colors, and measurements

        """
        ####################
        ### Measurements ###
        ####################
        
        # Padded space of Version shape and its label
        self.VERSION_LABEL_MARGIN = (60, 35)
        
        # Control the size and gap for the 3 little segments when
        # draw connections between versions
        self.LINK_SEGMENT_LENGTH = 15
        self.LINK_SEGMENT_GAP = 5
        self.LINK_SEGMENT_SQUARE_LENGTH = 12

        # The default minimum size of the graphics views
        self.BOUNDING_RECT_MINIMUM = 64

        ##############
        ### Colors ###
        ##############

        # Background brush of the version tree
        self.VERSION_TREE_BACKGROUND_BRUSH = QtGui.QBrush(QtGui.QColor(255, 255, 255, 255))    
        
        # Pen to draw version tree node
        self.VERSION_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('black')))), 2)    
        self.GHOSTED_VERSION_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('light_grey')))), 2)    
        self.VERSION_SELECTED_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('goldenrod_medium')))), 4)

        # Brush and pen to draw a version label
        self.VERSION_LABEL_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('black')))), 2)
        self.GHOSTED_VERSION_LABEL_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('light_grey')))), 2)
        self.VERSION_LABEL_SELECTED_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('black')))), 2)

        # Brush to draw version belongs to the current user
        self.VERSION_USER_BRUSH = QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('desatcornflower'))))
        self.GHOSTED_VERSION_USER_BRUSH = QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('very_light_grey'))))

        # Brush to draw version belongs to the other users
        self.VERSION_OTHER_BRUSH = QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('melon'))))
    
        # Brush and pen to draw a link between two versions
        self.LINK_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('black')))), 1.5)
        self.LINK_SELECTED_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('goldenrod_medium')))), 3)
        self.GHOSTED_LINK_PEN = QtGui.QPen(QtGui.QBrush(
            QtGui.QColor(*(ColorByName.get_int('light_grey')))), 2)

        # Color of the hover/unhover labels
        self.HOVER_DEFAULT_COLOR = QtGui.QColor(
            *ColorByName.get_int('black'))
        self.HOVER_SELECT_COLOR = QtGui.QColor(
            *ColorByName.get_int('blue'))
        
        #############
        ### Fonts ###
        #############
        
        self.VERSION_FONT = QtGui.QFont("Arial", 15, QtGui.QFont.Bold)
        self.VERSION_FONT_METRIC = QtGui.QFontMetrics(self.VERSION_FONT)
        self.VERSION_DESCRIPTION_FONT = QtGui.QFont("Arial", 15, QtGui.QFont.Normal, 
                                                    True)
        self.VERSION_DESCRIPTION_FONT_METRIC = \
            QtGui.QFontMetrics(self.VERSION_DESCRIPTION_FONT)

        if systemType in ['Windows', 'Microsoft']:
            font = QtGui.QFont("Arial", 8)
            font.setBold(True)
            self.VERSION_PROPERTIES_FONT = font
            self.VERSION_PROPERTIES_FONT_METRIC = QtGui.QFontMetrics(font)
        else:
            self.VERSION_PROPERTIES_FONT = QtGui.QFont("Arial", 12)
            self.VERSION_PROPERTIES_FONT_METRIC = \
                QtGui.QFontMetrics(self.VERSION_PROPERTIES_FONT)

        self.VERSION_PROPERTIES_PEN =  QtGui.QBrush(
            QtGui.QColor(20, 100, 20, 255))
        
        
        #############
        ### Icons ###
        #############
        
        self.APPLICATION_ICON = QtGui.QIcon(':/images/logo.png')
        self.APPLICATION_PIXMAP = QtGui.QPixmap(':/images/logo.png')
        self.SELECT_CURSOR = QtGui.QCursor(QtCore.Qt.ArrowCursor)
        self.OPEN_HAND_CURSOR = QtGui.QCursor(QtCore.Qt.OpenHandCursor)
        self.CLOSE_HAND_CURSOR = QtGui.QCursor(QtCore.Qt.ClosedHandCursor)
        self.ZOOM_CURSOR = QtGui.QCursor(QtCore.Qt.SizeVerCursor)
        self.NEW_VISTRAIL_ICON = QtGui.QIcon(':/images/new.png') 
        self.OPEN_VISTRAIL_ICON = QtGui.QIcon(':/images/open.png')
        self.SAVE_VISTRAIL_ICON = QtGui.QIcon(':/images/save.png') 
        self.UNDO_ICON = QtGui.QIcon(':/images/undo.png')
        self.REDO_ICON = QtGui.QIcon(':/images/redo.png')
        self.PAN_ICON = QtGui.QIcon(':/images/pan.png')
        self.SELECT_ICON = QtGui.QIcon(':/images/arrow.png')
        self.ZOOM_ICON = QtGui.QIcon(':/images/zoom.png')
        self.QUERY_VIEW_ICON = QtGui.QIcon(':/images/find.png')
        self.QUERY_ARROW_ICON = QtGui.QIcon(':/images/find_arrow.png')
        self.PLAY_ICON = QtGui.QIcon(':/images/play.png')
        self.PAUSE_ICON = QtGui.QIcon(':/images/pause.png')
        self.STOP_ICON = QtGui.QIcon(':/images/stop.png')
        self.PICK_VERSIONS_ICON = QtGui.QIcon(':/images/pickversions.png')
        if systemType in ['Darwin']:
            self.CLOSE_ICON = QtGui.QIcon(':/images/mac_close.png')
        else:
            self.CLOSE_ICON = QtGui.QIcon(':/images/close.png')
        

        ###########################
        ### Unused Measurements ###
        ###########################

        self.VIRTUAL_CELL_LABEL_SIZE = (0,0)
        self.PIP_IN_FRAME_WIDTH = 0
        self.PIP_OUT_FRAME_WIDTH = 0
        self.PIP_DEFAULT_SIZE = (0,0)
        self.MODULE_LABEL_MARGIN = (0,0,0,0)
        self.MODULE_PORT_MARGIN = (0,0,0,0)
        self.PORT_WIDTH = 0
        self.PORT_HEIGHT = 0
        self.PORT_RECT = QtCore.QRectF(0, 0, 0, 0)
        self.MODULE_PORT_SPACE = 0
        self.MODULE_PORT_PADDED_SPACE = 0
        self.CONFIGURE_WIDTH = 0
        self.CONFIGURE_HEIGHT = 0
        self.CONFIGURE_SHAPE = QtGui.QPolygonF()

        #####################
        ### Unused Colors ###
        #####################

        self.PIP_FRAME_COLOR = QtGui.QColor(0,0,0,0)

        ####################
        ### Unused Icons ###
        ####################
        
        empty_icon = QtGui.QIcon(':/images/empty.png')
        empty_pixmap = QtGui.QPixmap(':/images/empty.png')

        self.DISCLAIMER_IMAGE = empty_pixmap
        self.EXECUTE_PIPELINE_ICON = empty_icon
        self.EXECUTE_EXPLORE_ICON = empty_icon
        self.TABBED_VIEW_ICON = empty_icon
        self.HORIZONTAL_VIEW_ICON = empty_icon
        self.VERTICAL_VIEW_ICON = empty_icon
        self.DOCK_VIEW_ICON = empty_icon
        self.OPEN_VISTRAIL_DB_ICON = empty_icon
        self.DB_ICON = empty_icon
        self.FILE_ICON = empty_icon
        self.CONSOLE_MODE_ICON = empty_icon
        self.BOOKMARKS_ICON = empty_icon
        self.BOOKMARKS_REMOVE_ICON = empty_icon
        self.BOOKMARKS_RELOAD_ICON = empty_icon
        self.VISUAL_DIFF_BACKGROUND_IMAGE = empty_icon
        self.VISUAL_DIFF_SHOW_PARAM_ICON = empty_icon
        self.VISUAL_DIFF_SHOW_LEGEND_ICON = empty_icon
        self.VISUAL_DIFF_CREATE_ANALOGY_ICON = empty_icon
        self.VIEW_MANAGER_CLOSE_ICON = empty_icon
        self.DOCK_BACK_ICON = empty_icon
        self.ADD_STRING_ICON = empty_icon
        self.UP_STRING_ICON = empty_icon
        self.DOWN_STRING_ICON = empty_icon
        self.PIPELINE_ICON = empty_icon
        self.HISTORY_ICON = empty_icon
        self.QUERY_ICON = empty_icon
        self.EXPLORE_ICON = empty_icon
        self.VISUAL_QUERY_ICON = empty_icon
        self.VIEW_FULL_TREE_ICON = empty_icon
        self.PERFORM_PARAMETER_EXPLORACTION_ICON = empty_icon
        self.VERSION_DRAG_PIXAMP = empty_pixmap
        self.EXPLORE_COLUMN_PIXMAP = empty_pixmap
        self.EXPLORE_ROW_PIXMAP = empty_pixmap
        self.EXPLORE_SHEET_PIXMAP = empty_pixmap
        self.EXPLORE_TIME_PIXMAP = empty_pixmap
        self.EXPLORE_SKIP_PIXMAP = empty_pixmap
        self.REMOVE_PARAM_PIXMAP = empty_pixmap
        self.RIGHT_ARROW_PIXMAP = empty_pixmap
        self.QUERY_EDIT_ICON = empty_icon

        ######################
        ### Unused Brushes ###
        ######################

        empty_brush = QtGui.QBrush(QtGui.QColor(255,255,255,255))
        empty_pen = QtGui.QPen(empty_brush, 1)
        self.PIPELINE_VIEW_BACKGROUND_BRUSH = empty_brush
        self.QUERY_BACKGROUND_BRUSH = empty_brush
        self.MODULE_BRUSH = empty_brush
        self.MODULE_PEN = empty_pen
        self.MODULE_SELECTED_PEN = empty_pen
        self.MODULE_LABEL_PEN = empty_pen
        self.MODULE_LABEL_SELECTED_PEN = empty_pen
        self.GHOSTED_MODULE_LABEL_PEN = empty_pen
        self.PORT_PEN = empty_pen
        self.PORT_BRUSH = empty_brush
        self.PORT_SELECTED_PEN = empty_pen
        self.PORT_OPTIONAL_PEN = empty_pen
        self.PORT_OPTIONAL_BRUSH = empty_brush
        self.CONFIGURE_PEN = empty_pen
        self.CONFIGURE_BRUSH = empty_brush

        ####################
        ### Unused Fonts ###
        ####################

        self.MODULE_FONT = QtGui.QFont("Arial", 14, QtGui.QFont.Bold)
        self.MODULE_FONT_METRIC = QtGui.QFontMetrics(self.MODULE_FONT)
        self.MODULE_DESC_FONT = QtGui.QFont("Arial", 12)
        self.MODULE_DESC_FONT_METRIC = QtGui.QFontMetrics(self.MODULE_DESC_FONT)

def new_get_current_theme():
    return PluginTheme()

gui.theme.get_current_theme = new_get_current_theme
