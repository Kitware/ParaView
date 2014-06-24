/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for Remote connection
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-toolbar-connect'
 *
 * @class jQuery.paraview.ui.toolbar.connect
 */
(function (GLOBAL, $) {

    var BASE_REMOTE_TOOLBAR_HTML =
    "<ul>"
    + "<li class='action' action='pv.remote.connect'         alt='Connect to a remote pvserver' title='Connect to a remote pvserver'><div class='icon'></div></li>"
    + "<li class='action' action='pv.remote.disconnect'      alt='Disconnect' title='Disconnect'><div class='icon'></div></li>"
    + "<li class='action' action='pv.remote.reverse.connect' alt='Wait for a remote server to connect' title='Wait for a remote server to connect'><div class='icon'></div></li>"
    + "</ul>\n",
    SESSION_DATA_KEY = 'paraview-session',
    CONNECT_ARG_KEYS = ['host', 'port', 'rs_host', 'rs_port'],
    /**
     * @class pv.ConnectionOptions
     *
     * @property {pv.Session} session
     * Session object use to communicate with the server.
     *
     * @property {boolean} showConnect
     * Show the connect button if true, otherwise just hide it.
     *
     * @property {boolean} showDisconnect
     * Show the disconnect button if true, otherwise just hide it.
     *
     * @property {boolean} showReverseConnect
     * Show the reverse connection button if true, otherwise just hide it.
     *
     * @property {boolean} closeWindowOnDisconnect
     * If true, when user click on disconnect, the browser tab will close.
     */
    DEFAULT_OPTIONS = {
        session: null,
        showConnect: true,
        showDisconnect: true,
        showReverseConnect: true,
        closeWindowOnDisconnect: true
    };

    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.ui.toolbar.connect
     * @method connectionToolbar
     * @param {pv.Session} session
     * ParaViewWeb session object.
     *
     * @param {pv.ConnectionOptions} options
     * Options
     *
     * Usage:
     *      $('.container-div').connectionToolbar({session: session, showReverseConnect: false});
     */
    $.fn.connectionToolbar = function(options) {
        var opt = $.extend({}, DEFAULT_OPTIONS, options);

        // Handle data with default values
        return this.each(function() {
            var me = $(this).addClass('paraview toolbar connect');

            // Save data
            me.data(SESSION_DATA_KEY, options.session);

            // Update HTML
            me[0].innerHTML = BASE_REMOTE_TOOLBAR_HTML;

            // Update UI based on config
            var buttons = $('.action', me);

            if(!opt.showConnect) {
                buttons.eq(0).hide();
            }
            if(!opt.showDisconnect) {
                buttons.eq(1).hide();
            }
            if(!opt.showReverseConnect) {
                buttons.eq(2).hide();
            }
            if(opt.closeWindowOnDisconnect) {
                buttons.eq(1).addClass('close');
            }

            // Attach listeners
            $('li.action', me).bind('click', actionListener);
        });
    };

    // =======================================================================
    // ==== Listener methods =================================================
    // =======================================================================

    function actionListener() {
        var me = $(this), rootWidget = getToolbarWiget(me),
        session = getSession(rootWidget),
        action = me.attr('action'),
        arg;

        if(session != null && session != undefined) {
            // Build arg based on the action
            if(action === 'connect') {
                arg = {};
                urlToParse = prompt("Connect to remote server:", "localhost:11111").split(':');
                for(var idx = 0; idx < urlToParse.length; idx++) {
                    if(idx % 2 === 0) {
                        arg[CONNECT_ARG_KEYS[idx]] = urlToParse[idx];
                    } else {
                        arg[CONNECT_ARG_KEYS[idx]] = Number(urlToParse[idx])
                    }
                }

            } else if(action === 'pvDisconnect') {
                arg = "disconnect";
            } else if(action === 'reverseConnect') {
                arg = Number(prompt("Wait connection on port:", "11111"));
            }

            session.call(action, [arg]).then(function(){
                if(me.hasClass('close')) {
                    session.call("application.exit");
                    session.close();
                    setTimeout("window.close()", 100);
                }
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
    // ==== Events triggered on the pipelineBrowser ==========================
    // =======================================================================

    /**
     * Event triggered when a Proxy has changed.
     *
     * @member jQuery.paraview.ui.toolbar.connect
     * @event connect-action
     * @param {String} action
     * Type of action that get triggered such as ['connect', 'pv-disconnect', 'reverseConnect']
     */
    function fireAction(rootWidget, action) {
        rootWidget.trigger({
            type: 'connect-action',
            action: action
        });
    }

    // =======================================================================
    // ==== Helper internal functions ========================================
    // =======================================================================

    function getSession(anyInnerProxyWidget) {
        return getToolbarWiget(anyInnerProxyWidget).data(SESSION_DATA_KEY);
    }

    // =======================================================================

    function getToolbarWiget(anyInnerProxyWidget) {
        return anyInnerProxyWidget.closest('.paraview.toolbar.connect');
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-toolbar-connect');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
