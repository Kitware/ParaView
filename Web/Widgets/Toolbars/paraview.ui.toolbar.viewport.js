/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for VCR Time control
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-toolbar-viewport'
 *
 * @class jQuery.paraview.ui.toolbar.viewport
 */
(function (GLOBAL, $) {

    var BASE_TOOLBAR_HTML =
    "<ul>"
    + "<li action='resetCamera' class='action' alt='Reset camera' title='Reset camera'><div class='icon'></div></li>"
    + "<li action='toggleOrientationAxis' class='action on' alt='Toggle visibility of orientation axis' title='Toggle visibility of orientation axis'><div class='icon'></div></li>"
    + "<li action='toggleCenterOfRotation' class='action on' alt='Toggle visibility of center of rotation' title='Toggle visibility of center of rotation'><div class='icon'></div></li>"
//    + "<li action='image' class='switch' other='webgl' alt='Toggle delivery mechanism from Image to Geometry' title='Toggle delivery mechanism from Image to Geometry'><div class='icon'></div></li>"
    + "<li action='toggleInfo' class='action' alt='Toggle viewport information' title='Toggle viewport information'><div class='icon'></div></li>"
    + "<select class='switch' type='enum'><option value='image' selected>Image</option><option value='webgl'>WebGL</option><option value='vgl'>VGL-WebGL</option></select>"
    + "</ul>\n",
    VIEWPORT_DATA_KEY = 'pvViewport';

    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.ui.toolbar.viewport
     * @method viewportToolbar
     * @param {vtkWeb.Viewport} viewport
     * ParaViewWeb session object.
     *
     * Usage:
     *      $('.container-div').vcrToolbar(session);
     */
    $.fn.viewportToolbar = function(viewport) {
        // Handle data with default values
        return this.each(function() {
            var me = $(this).addClass('paraview toolbar viewport');

            // Save data
            me.data(VIEWPORT_DATA_KEY, viewport);

            // Update HTML
            me[0].innerHTML = BASE_TOOLBAR_HTML;

            // Attach listeners
            $('li.action', me).bind('click', actionListener);
            $('li.switch', me).bind('click', switchListener).hide();
            $('select.switch', me).bind('change', switchListener);

            // Make sure webgl is enable to show the switch button
 //           try {
 //               if(vtkWeb.ViewportFactory.webgl) {
//                    $('li.switch', me).show();
 //               }
//            } catch(error) {}

        });
    };

    // =======================================================================
    // ==== Listener methods =================================================
    // =======================================================================

    function actionListener() {
        var me = $(this), rootWidget = getToolbarWiget(me),
        viewport = getViewport(rootWidget),
        action = me.attr('action');

        if(viewport != null && viewport != undefined) {
            if(action === 'resetCamera') {
                viewport.resetCamera();
            } else if(action === 'toggleOrientationAxis') {
                viewport.updateOrientationAxesVisibility(me.toggleClass('on').hasClass('on'));
            } else if(action === 'toggleCenterOfRotation') {
                viewport.updateCenterAxesVisibility(me.toggleClass('on').hasClass('on'));
            } else if(action === 'toggleInfo') {
                viewport.showStatistics(me.toggleClass('on').hasClass('on'));
            }
        }

        /**
         * Event triggered when a Proxy has changed.
         *
         * @member jQuery.paraview.ui.toolbar.viewport
         * @event viewport-action
         * @param {String} action
         * Type of action that get triggered such as ['resetCamera', 'centerOfRotation', 'orientationAxis']
         * @param {Boolean} status
         * On/Off for  centerOfRotation and orientationAxis
         */
        rootWidget.trigger({
            type: 'viewport-action',
            action: action,
            status: true
        });
    }

    // =======================================================================

    function switchListener() {
        var me = $(this), rootWidget = getToolbarWiget(me),
        viewport = getViewport(rootWidget),
        action = me.attr('action'),
        other = me.attr('other'),
        selected = me.attr('value');

        if(viewport != null && viewport != undefined) {
 //           me.attr('action', other).attr('other', action);
            viewport.setActiveRenderer(selected);
            viewport.invalidateScene();
        }

//        rootWidget.trigger({
//            type: 'viewport-action',
//            action: other,
//            status: true
//        });
    }

    // =======================================================================
    // ==== Helper internal functions ========================================
    // =======================================================================

    function getViewport(anyInnerProxyWidget) {
        return getToolbarWiget(anyInnerProxyWidget).data(VIEWPORT_DATA_KEY);
    }

    // =======================================================================

    function getToolbarWiget(anyInnerProxyWidget) {
        return anyInnerProxyWidget.closest('.paraview.toolbar');
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-toolbar-viewport');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
