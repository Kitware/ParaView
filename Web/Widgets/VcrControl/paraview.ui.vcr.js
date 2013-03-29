/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for VCR Time control
 * related to ParaViewWeb usage.
 *
 * @class jQuery.paraview.VCRToolbar
 */
(function (GLOBAL, $) {

    var BASE_VCR_TOOLBAR_HTML =
    "<ul>"
    + "<li action='first' class='action'><div class='icon' alt='First frame'></div></li>"
    + "<li action='prev'  class='action'><div class='icon' alt='Previous frame'></div></li>"
    + "<li action='play'  class='toggle play'><div class='icon' alt='Play animation'></div></li>"
    + "<li action='pause' class='toggle hide pause'><div class='icon' alt='Pause animation'></div></li>"
    + "<li action='next'  class='action'><div class='icon' alt='Next frame'></div></li>"
    + "<li action='last'  class='action'><div class='icon' alt='Last frame'></div></li>"
    + "</ul>\n";

    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.VCRToolbar
     * @method vcrToolbar
     * @param {pv.Session} session
     * ParaViewWeb session object.
     *
     * Usage:
     *      $('.-container-div').vcrToolbar(session);
     */
    $.fn.vcrToolbar = function(session) {
        // Handle data with default values
        return this.each(function() {
            var me = $(this).addClass('paraview vcrToolbar');

            // Save data
            me.data('pvSession', session);

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
            session.call("pv:vcr", action);
        }

        fireAction(rootWidget, action);

        /**
         * Event triggered when anything in the pipeline has changed and therefore
         * a viewport update should occurs.
         *
         * @member jQuery.paraview.VCRToolbar
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
                session.call("pv:vcr", 'next').then(function(){
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
         * @member jQuery.paraview.VCRToolbar
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

    /**
         * Event triggered when anything in the pipeline has changed and therefore
         * a viewport update should occurs.
         *
         * @member jQuery.paraview.VCRToolbar
         * @event dataChanged
         */

    // =======================================================================
    // ==== Helper internal functions ========================================
    // =======================================================================

    function getSession(anyInnerProxyWidget) {
        return getVcrToolbarWiget(anyInnerProxyWidget).data('pvSession');
    }

    // =======================================================================

    function getVcrToolbarWiget(anyInnerProxyWidget) {
        return anyInnerProxyWidget.closest('.paraview.vcrToolbar');
    }

}(window, jQuery));