/**
 * ParaViewWeb JavaScript Library.
 *
 * This module allow the Web client to create viewport to ParaView views.
 * Those viewport are interactive windows that are used to render 2D/3D content
 * and response to user mouse interactions.
 *
 * @class paraview.viewport
 */
(function (GLOBAL, $) {

    // ----------------------------------------------------------------------
    // Viewport constants
    // ----------------------------------------------------------------------

    var DEFAULT_RENDERERS_CONTAINER_HTML = "<div class='renderers'></div>",
    DEFAULT_RENDERERS_CONTAINER_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "0"
    },

    DEFAULT_MOUSE_LISTENER_HTML = "<div class='mouse-listener'></div>",
    DEFAULT_MOUSE_LISTENER_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "1000"
    },
    module = {},

    /**
     * @class pv.ViewPortConfig
     * Configuration object used to create a viewport.
     */
    DEFAULT_VIEWPORT_OPTIONS = {
        /**
         * @member pv.ViewPortConfig
         * @property {pv.Session} session
         * Object used to communicate with the remote server.
         *
         * Default: null but MUST BE OVERRIDE !!!
         */
        session: null,
        /**
         * @member pv.ViewPortConfig
         * @property {Number} view
         * Specify the GlobalID of the view that we want to render. By default,
         * set to -1 to use the active view.
         *
         * Default: -1
         */
        view: -1,
        /**
         * @member pv.ViewPortConfig
         * @property {Boolean} enableInteractions
         * Enable by default the user intaration but any mouse interaction can
         * be disable if needed.
         *
         * Default: true
         */
        enableInteractions: true,

        /**
         * @member pv.ViewPortConfig
         * @property {String} renderer
         * Name of the renderer to be used. Can only be 'image' or 'webgl'.
         *
         * Default: 'image'
         */
        renderer: 'image'
    };

    // ----------------------------------------------------------------------
    // Mouse interaction helper methods for viewport
    // ----------------------------------------------------------------------

    function preventDefault(event) {
        event.preventDefault();
    }

    // ----------------------------------------------------------------------

    function attachMouseListener(mouseListenerContainer, renderersContainer) {
        var current_button = null;

        // Internal method used to pre-process the interaction to standardise it
        // for a ParaView usage.
        function mouseInteraction(event) {
            if(event.hasOwnProperty("type")) {
                if(event.type === 'mouseup') {
                    current_button = null;
                    renderersContainer.trigger($.extend(event, { type: 'mouse', action: 'up', current_button: current_button}));
                } else if(event.type === 'mousedown') {
                    current_button = event.which;
                    renderersContainer.trigger($.extend(event, { type: 'mouse', action: 'down', current_button: current_button}));
                } else if(event.type === 'mousemove' && current_button != null) {
                    renderersContainer.trigger($.extend(event, { type: 'mouse', action: 'move', current_button: current_button}));
                }
            }
        }

        // Bind listener to UI container
        mouseListenerContainer.bind("contextmenu click mouseover", preventDefault);
        mouseListenerContainer.bind('mousedown mouseup mousemove', mouseInteraction);
    }

    // ----------------------------------------------------------------------
    // Viewport container definition
    // ----------------------------------------------------------------------

    /**
     * Create a new viewport for a ParaView View.
     * The options are defined by {@link pv.ViewPortConfig}.
     *
     * @param {pv.ViewPortConfig} options
     * Configure the viewport to create the way we want.
     *
     * @return {pv.Viewport}
     */
    function createViewport(options) {
        // Make sure we have a valid autobahn session
        if (options.session === null) {
            throw "'session' must be provided within the option.";
        }

        // Create viewport
        var config = $.extend({}, DEFAULT_VIEWPORT_OPTIONS, options),
        session = options.session,
        rendererContainer = $(DEFAULT_RENDERERS_CONTAINER_HTML).css(DEFAULT_RENDERERS_CONTAINER_CSS),
        mouseListener = $(DEFAULT_MOUSE_LISTENER_HTML).css(DEFAULT_MOUSE_LISTENER_CSS),
        viewport = {
            /**
             * Update the active renderer to be something else.
             * This allow the user to switch from Image Delivery to Geometry delivery
             * or even any other available renderer type available.
             *
             * The available renderers are indexed inside the following object paraview.ViewportFactory.
             *
             * @member pv.Viewport
             * @param {String} rendererName
             * Key used to ID the renderer type.
             */
            setActiveRenderer: function(rendererName) {
                $('.' + rendererName, rendererContainer).addClass('active').show().siblings().removeClass('active').hide();
            },

            /**
             * Method that should be called each time something in the scene as changed
             * and we want to update the viewport to reflect the latest state of the scene.
             *
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            invalidateScene: function(onDone) {
                rendererContainer.trigger({
                    type: 'invalidateScene',
                    callback: onDone
                });
            },

            /**
             * Method that should be called when nothing has changed in the scene
             * but for some reason the viewport has been dirty.
             * (i.e. Toggeling the statistic information within the viewport)
             *
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            render: function(onDone) {
                rendererContainer.trigger({
                    type: 'render',
                    callback: onDone
                });
            },

            /**
             * Reset the camera of the scene to make it fit in the screen as well
             * as invalidating the scene automatically.
             *
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            resetCamera: function(onDone) {
                return session.call("pv:resetCamera", Number(config.view)).then(function () {
                    rendererContainer.trigger('invalidateScene');
                });
            },

            /**
             * Update Orientation Axes Visibility for the given view
             *
             * @member pv.Viewport
             * @param {Boolean} show
             * Show: true / Hide: false
             * @param {Function} ondone Function to call after rendering is complete.
             */
            updateOrientationAxesVisibility: function (show, ondone) {
                return session.call("pv:updateOrientationAxesVisibility", Number(options.view), show).then(function () {
                    rendererContainer.trigger({
                        type: 'render',
                        callback: onDone
                    });
                });
            },

            /**
             * Update the Center Axes Visibility for the given view
             *
             * @member pv.Viewport
             * @param {Boolean} show
             * Show: true / Hide: false
             * @param {Function} ondone Function to call after rendering is complete.
             */
            updateCenterAxesVisibility: function (show, ondone) {
                return session.call("pv:updateCenterAxesVisibility", Number(config.view), show).then(function () {
                    rendererContainer.trigger({
                        type: 'render',
                        callback: onDone
                    });
                });
            },

            /**
             * Attach viewport to a DOM element
             *
             * @member pv.Viewport
             * @param {String} selector
             * The will be used internally to get the jQuery associated element
             *
             *     <div class="renderer"></div>
             *     viewport.bind(".renderer");
             *
             *     <div id="renderer"></div>
             *     viewport.bind("#renderer");
             *
             *     <html>
             *       <body>
             *         <!-- renderer -->
             *         <div></div>
             *       </body>
             *     </html>
             *     viewport.bind("body > div");
             */
            bind: function (selector) {
                var container = $(selector);
                if (container.attr("__pv_viewport__") !== true) {
                    container.attr("__pv_viewport__", true);
                    container.append(rendererContainer).append(mouseListener);
                    rendererContainer.trigger('invalidateScene');
                }
            },

            /**
             * Remove viewport from DOM element
             *
             * @member pv.Viewport
             */
            unbind: function () {
                var parentElement = rendererContainer.parent();
                if (parentElement) {
                    parentElement.attr("__pv_viewport__", false);
                    rendererContainer.remove();
                    mouseListener.remove();
                }
            }
        };

        // Attach config object to renderer parent
        rendererContainer.data('config', config);

        // Create any renderer type that is available
        for(var key in paraview.ViewportFactory) {
            try {
                paraview.ViewportFactory[key].builder(rendererContainer);
            } catch(error) {
                console.log("Error while trying to load renderer: " + key);
                console.log(error);
            }
        }

        // Set default renderer
        viewport.setActiveRenderer(config.renderer);

        // Attach mouse listener if requested
        if (config.enableInteractions) {
            attachMouseListener(mouseListener, rendererContainer);
        }

        return viewport;
    }

    // ----------------------------------------------------------------------
    // Init paraview module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("paraview")) {
        module = GLOBAL.paraview || {};
    } else {
        GLOBAL.paraview = module;
    }

    // ----------------------------------------------------------------------
    // Export internal methods to the paraview module
    // ----------------------------------------------------------------------
    module.createViewport = function (option) {
        return createViewport(option);
    };
}(window, jQuery));
