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
        "z-index" : "0",
        "overflow": "hidden"
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

    DEFAULT_STATISTIC_HTML = "<div class='statistics'></div>",
    DEFAULT_STATISTIC_CSS = {
        "position": "absolute",
        "top": "0px",
        "left": "0px",
        "right": "0px",
        "bottom": "0px",
        "z-index" : "999",
        "display" : "none"
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
                    renderersContainer.trigger($.extend(event, {
                        type: 'mouse',
                        action: 'up',
                        current_button: current_button
                    }));
                } else if(event.type === 'mousedown') {
                    current_button = event.which;
                    renderersContainer.trigger($.extend(event, {
                        type: 'mouse',
                        action: 'down',
                        current_button: current_button
                    }));
                } else if(event.type === 'mousemove' && current_button != null) {
                    renderersContainer.trigger($.extend(event, {
                        type: 'mouse',
                        action: 'move',
                        current_button: current_button
                    }));
                }
            }
        }

        // Bind listener to UI container
        mouseListenerContainer.bind("contextmenu click mouseover", preventDefault);
        mouseListenerContainer.bind('mousedown mouseup mousemove', mouseInteraction);
    }

    // ----------------------------------------------------------------------
    // Viewport statistic manager
    // ----------------------------------------------------------------------

    function createStatisticManager() {
        var statistics = {}, formatters = {};

        // Fill stat formatters
        for(var factoryKey in paraview.ViewportFactory) {
            var factory = paraview.ViewportFactory[factoryKey];
            if(factory.hasOwnProperty('stats')) {
                for(var key in factory.stats) {
                    formatters[key] =  factory.stats[key];
                }
            }
        }

        function handleEvent(event) {
            var id = event.stat_id,
            value = event.stat_value,
            statObject = null;

            if(!statistics.hasOwnProperty(id) && formatters.hasOwnProperty(id)) {
                if(formatters[id].type === 'time') {
                    statObject = statistics[id] = createTimeValueRecord();
                } else if (formatters[id].type === 'value') {
                    statObject = statistics[id] = createValueRecord();
                }
            } else {
                statObject = statistics[id];
            }

            if(statObject != null) {
                statObject.record(value);
            }
        }

        // ------------------------------------------------------------------

        function toHTML() {
            var buffer = createBuffer(), hasContent = false, key, formater, stat,
            min, max;

            // Extract stat data
            buffer.append("<table class='viewport-stat'>");
            buffer.append("<tr class='stat-header'><td class='label'></td><td class='value'>Current</td><td class='min'>Min</td><td class='max'>Max</td><td class='avg'>Average</td></tr>");
            for(key in statistics) {
                if(formatters.hasOwnProperty(key) && statistics[key].valid) {
                    formater = formatters[key];
                    stat = statistics[key];
                    hasContent = true;

                    // The functiion may swap the order
                    min = formater.convert(stat.min);
                    max = formater.convert(stat.max);

                    buffer.append("<tr><td class='label'>");
                    buffer.append(formater.label);
                    buffer.append("</td><td class='value'>");
                    buffer.append(formater.convert(stat.value));
                    buffer.append("</td><td class='min'>");
                    buffer.append((min < max) ? min : max);
                    buffer.append("</td><td class='max'>");
                    buffer.append((min > max) ? min : max);
                    buffer.append("</td><td class='avg'>");
                    buffer.append(formater.convert(stat.getAverageValue()));
                    buffer.append("</td></tr>");
                }
            }
            buffer.append("</table>");

            return hasContent ? buffer.toString() : "";
        }

        // ------------------------------------------------------------------

        return {
            eventHandler: handleEvent,
            toHTML: toHTML,
            reset: function() {
                statistics = {};
            }
        }
    }

    // ----------------------------------------------------------------------

    function createBuffer() {
        var idx = -1, buffer = [];
        return {
            clear: function(){
                idx = -1;
                buffer = [];
            },
            append: function(str) {
                buffer[++idx] = str;
                return this;
            },
            toString: function() {
                return buffer.join('');
            }
        };
    }

    // ----------------------------------------------------------------------

    function createTimeValueRecord() {
        var lastTime, sum, count;

        // Default values
        lastTime = 0;
        sum = 0;

        return {
            value: 0.0,
            valid: false,
            min: +1000000000.0,
            max: -1000000000.0,

            record: function(v) {
                if(v === 0) {
                    this.start();
                } else if (v === 1) {
                    this.stop();
                }
            },

            start: function() {
                lastTime = new Date().getTime();
            },

            stop: function() {
                if(lastTime != 0) {
                    this.valid = true;
                    var time = new Date().getTime();
                    this.value = time - lastTime;
                    this.min = (this.min < this.value) ? this.min : this.value;
                    this.max = (this.max > this.value) ? this.max : this.value;
                    //
                    sum += this.value;
                    count++;
                }
            },

            reset: function() {
                count = 0;
                sum = 0;
                lastTime = 0;
                this.value = 0;
                this.min = +1000000000.0;
                this.max = -1000000000.0;
                this.valid = false;
            },

            getAverageValue: function() {
                if(count == 0) {
                    return 0;
                }
                return (sum / count);
            }
        }
    }

    // ----------------------------------------------------------------------

    function createValueRecord() {
        var sum, count;

        return {
            value: 0.0,
            valid: false,
            min: +1000000000.0,
            max: -1000000000.0,

            record: function(v) {
                this.valid = true;
                this.value = v;
                this.min = (this.min < this.value) ? this.min : this.value;
                this.max = (this.max > this.value) ? this.max : this.value;
                //
                sum += this.value;
                count++;
            },

            reset: function() {
                count = 0;
                sum = 0;
                this.value = 0;
                this.min = +1000000000.0;
                this.max = -1000000000.0;
                this.valid = false;
            },

            getAverageValue: function() {
                if(count === 0) {
                    return 0;
                }
                return (sum / count);
            }
        }
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
        statContainer = $(DEFAULT_STATISTIC_HTML).css(DEFAULT_STATISTIC_CSS),
        onDoneQueue = [],
        statisticManager = createStatisticManager(),
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
                rendererContainer.trigger('active');
                statContainer[0].innerHTML = '';
                statisticManager.reset();
            },

            /**
             * Method that should be called each time something in the scene as changed
             * and we want to update the viewport to reflect the latest state of the scene.
             *
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            invalidateScene: function(onDone) {
                onDoneQueue.push(onDone);
                rendererContainer.trigger('invalidateScene');
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
                onDoneQueue.push(onDone);
                rendererContainer.trigger('render');
            },

            /**
             * Reset the camera of the scene to make it fit in the screen as well
             * as invalidating the scene automatically.
             *
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            resetCamera: function(onDone) {
                onDoneQueue.push(onDone);
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
            updateOrientationAxesVisibility: function (show, onDone) {
                return session.call("pv:updateOrientationAxesVisibility", Number(config.view), show).then(function () {
                    onDoneQueue.push(onDone);
                    rendererContainer.trigger('invalidateScene');
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
            updateCenterAxesVisibility: function (show, onDone) {
                return session.call("pv:updateCenterAxesVisibility", Number(config.view), show).then(function () {
                    onDoneQueue.push(onDone);
                    rendererContainer.trigger('invalidateScene');
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
                if (container.attr("__pv_viewport__") !== "true") {
                    container.attr("__pv_viewport__", "true");
                    container.append(rendererContainer).append(mouseListener).append(statContainer);
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
                    parentElement.attr("__pv_viewport__", "false");
                    rendererContainer.remove();
                    mouseListener.remove();
                    statContainer.remove();
                }
            },

            /**
             * Update statistic visibility
             *
             * @member pv.Viewport
             * @param {Boolean} visible
             */
            statVisibility: function(isVisible) {
                if(isVisible) {
                    statContainer.show();
                } else {
                    statContainer.hide();
                }
            },

            /**
             * Clear current statistic values
             *
             * @member pv.Viewport
             */
            resetStatistics: function() {
                statisticManager.reset();
                statContainer.empty();
            }
        };

        // Attach config object to renderer parent
        rendererContainer.data('config', config);

        // Attach onDone listener
        rendererContainer.bind('done', function(){
            while(onDoneQueue.length > 0) {
                var callback = onDoneQueue.pop();
                try {
                    if(callback) {
                        callback();
                    }
                } catch(error) {
                    console.log("On Done callback error:");
                    console.log(error);
                }
            }
        });

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

        // Attach stat listener
        rendererContainer.bind('stats', function(event){
            statisticManager.eventHandler(event);
            statContainer[0].innerHTML = statisticManager.toHTML();
        });

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
