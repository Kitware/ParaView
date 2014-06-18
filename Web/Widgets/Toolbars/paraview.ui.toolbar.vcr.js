/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for VCR Time control
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-toolbar-vcr'
 *
 * @class jQuery.paraview.ui.toolbar.vcr
 */
(function (GLOBAL, $) {

    var BASE_VCR_TOOLBAR_HTML =
    "<ul>"
    + "<li action='first' class='action'            alt='First frame' title='First frame'><div class='icon'></div></li>"
    + "<li action='prev'  class='action'            alt='Previous frame' title='Previous frame'><div class='icon'></div></li>"
    + "<li action='play'  class='toggle play'       alt='Play animation' title='Play animation'><div class='icon'></div></li>"
    + "<li action='pause' class='toggle hide pause' alt='Pause animation' title='Pause animation'><div class='icon'></div></li>"
    + "<li action='next'  class='action'            alt='Next frame' title='Next frame'><div class='icon' alt='Next frame'></div></li>"
    + "<li action='last'  class='action'            alt='Last frame' title='Last frame'><div class='icon'></div></li>"
    + "<input type='text' class='time' value='0' disabled>"
    + "</ul>\n",
    SESSION_DATA_KEY = 'paraview-session';

    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.ui.toolbar.vcr
     * @method vcrToolbar
     * @param {vtkWeb.Session} session
     * ParaViewWeb session object.
     *
     * Usage:
     *      $('.container-div').vcrToolbar(session);
     */
    $.fn.vcrToolbar = function(session) {
        // Handle data with default values
        return this.each(function() {
            var me = $(this).addClass('paraview toolbar vcr');

            // Save data
            me.data(SESSION_DATA_KEY, session);

            // Update HTML
            me[0].innerHTML = BASE_VCR_TOOLBAR_HTML;

            // Attach listeners
            $('li.action', me).bind('click', actionListener);
            $('li.play', me).bind('click', playAction);
            $('li.toggle', me).bind('click', toggleListener);
        });
    };

    // =======================================================================
    // ==== Listener methods =================================================
    // =======================================================================

    function actionListener() {
        var me = $(this), rootWidget = getVcrToolbarWiget(me),
        session = getSession(rootWidget),
        action = me.attr('action');

        if(session != null && session != undefined) {
            session.call("pv.vcr.action", [action]).then(function(time){
                $('input.time', rootWidget).val(time);
            });
        }

        fireAction(rootWidget, action);

        /**
         * Event triggered when anything in the pipeline has changed and therefore
         * a viewport update should occurs.
         *
         * @member jQuery.paraview.ui.toolbar.vcr
         * @event dataChanged
         */
        rootWidget.trigger('dataChanged');
    }

    // =======================================================================

    function playAction() {
        var me = $(this), rootWidget = getVcrToolbarWiget(me),
        session = getSession(rootWidget);

        function next() {
            if(session != null && session != undefined) {
                session.call("pv.vcr.action", ['next']).then(function(time){
                    $('input.time', rootWidget).val(time);
                    rootWidget.trigger('dataChanged');
                    if($('.pause', rootWidget).is(':visible')) {
                        setTimeout(next, 10);
                    }
                });
            }
        }

        setTimeout(next, 10);
    }

    // =======================================================================

    function toggleListener() {
        var me = $(this);
        me.addClass('hide').siblings('.toggle').removeClass('hide');
    }

    // =======================================================================
    // ==== Events triggered on the pipelineBrowser ==========================
    // =======================================================================

    /**
     * Event triggered when a Proxy has changed.
     *
     * @member jQuery.paraview.ui.toolbar.vcr
     * @event vcr-action
     * @param {String} action
     * Type of action that get triggered such as ['first', 'prev', 'play', 'pause', 'next', 'last']
     */
    function fireAction(rootWidget, action) {
        rootWidget.trigger({
            type: 'vcr-action',
            action: action
        });
    }

    // =======================================================================
    // ==== Helper internal functions ========================================
    // =======================================================================

    function getSession(anyInnerProxyWidget) {
        return getVcrToolbarWiget(anyInnerProxyWidget).data(SESSION_DATA_KEY);
    }

    // =======================================================================

    function getVcrToolbarWiget(anyInnerProxyWidget) {
        return anyInnerProxyWidget.closest('.paraview.toolbar.vcr');
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-toolbar-vcr');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
