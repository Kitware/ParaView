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

    /**
     * @class pv.ViewPortConfig
     * Configuration object used to create a viewport.
     * 
     *     DEFAULT_VALUES = {
     *       useCanvas: true,
     *       view: -1,
     *       enableInteractions: true,
     *       interactiveQuality: 30,
     *       stillQuality: 100
     *     }
     */
    var DEFAULT_VIEWPORT_OPTIONS = {
        /**
         * @member pv.ViewPortConfig
         * @property {Boolean} useCanvas
         * True by default to benefit of HTML5 Canvas tag. If turned off, then
         * basic image tag will be used. But such tag seems to induce flickering
         * inside Firefox.
         * 
         * Default: true
         */
        useCanvas: true,
        /**
         * @member pv.ViewPortConfig
         * @property {Number} view
         * Specify the GlobalID of the view that we want to render. By default,
         * set to -1 to the active view will be used.
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
         * @property {Number} interactiveQuality
         * Compression quality that should be used to encode the image on the
         * server side while interacting.
         * 
         * Default: 30
         */
        interactiveQuality: 30,
        /**
         * @member pv.ViewPortConfig
         * @property {Number} stillQuality
         * Compression quality that should be used to encode the image on the
         * server side when we stoped interacting. 
         * 
         * Default: 100
         */
        stillQuality: 100
           
    }, module = {};

    /**
     * Create a new viewport for a ParaView View.
     * The options are defined by {@link pv.ViewPortConfig}.
     * 
     * @member paraview.viewport
     * 
     * @param {Object} session
     * [Autobahn](http://autobahn.ws/js) session object.
     * 
     * @param {pv.ViewPortConfig} options
     * Configure the viewport to create the way we want.
     * 
     * @return {pv.Viewport}
     */
    function createViewport(session, options) {
        // Make sure we have a valid autobahn session
        if (session === null) {
            throw "'session' must be provided.";
        }

        // Internal fields
        var config = $.extend({}, options, DEFAULT_VIEWPORT_OPTIONS),
        bgImage = new Image(),
        canvas = $("<canvas></canvas>"),
        ctx2d = canvas.get(0).getContext('2d'),
        viewport = null,
        current_button = null,
        action_pending = false,
        force_render = false,
        statistics = null,
        button_state = {
            left : false,
            right: false,
            middle : false
        },
        lastMTime = 0,
        render_onidle_timeout = null,
        quality = 100,
        headEvent = (GLOBAL.paraview.isMobile ? "v" : "");

        //-----
        // Internal function that requests a render on idle. Calling this
        // mutliple times will only result in the request being set once.
        function renderOnIdle() {
            if (render_onidle_timeout === null) {
                render_onidle_timeout = GLOBAL.setTimeout(render, 250);
            }
        }
        // ----
        /// Internal method that returns true if the mouse interaction event should be
        /// throttled.
        function eatMouseEvent(pvevent) {
            var force_event = (button_state.left !== pvevent.buttonLeft || button_state.right  !== pvevent.buttonRight || button_state.middle !== pvevent.buttonMiddle);
            if (!force_event && !pvevent.buttonLeft && !pvevent.buttonRight && !pvevent.buttonMiddle) {
                return true;
            }
            if (!force_event && action_pending) {
                return true;
            }
            button_state.left   = pvevent.buttonLeft;
            button_state.right  = pvevent.buttonRight;
            button_state.middle = pvevent.buttonMiddle;
            return false;
        }

        // Setup internal API
        function render(ondone) {
            if (force_render === false) {
                if (render_onidle_timeout !== null) {
                    // clear any renderOnIdle requests that are pending since we
                    // are sending a render request.
                    GLOBAL.clearTimeout(render_onidle_timeout);
                    render_onidle_timeout = null;
                }
                force_render = true;
                /**
                 * @class request.Render
                 * Container Object that provide all the input needed to request
                 * a rendering from the server side.
                 * 
                 *      {
                 *        size: [width, height], // Size of the image to generate
                 *        view: 234523,          // View proxy globalId
                 *        mtime: 23423456,       // Last Modified time image received
                 *        quality: 100,          // Image quality expected
                 *        localtime: 3563456     // Local time at sending for round trip computation statistic
                 *      }
                 */
                var renderCfg = {
                    /**
                     * @member request.Render
                     * @property {Array} size
                     * Size of the Viewport for which the image should be render
                     * for. [width, height] in pixel.
                     */
                    size: [ viewport.parent().innerWidth(), viewport.parent().innerHeight() ],
                    /**
                     * @member request.Render
                     * @property {Number} view
                     * GlobalID of the view Proxy.
                     */
                    view: Number(config.view),
                    /**
                     * @member request.Render
                     * @property {Number} MTime
                     * Last received image MTime.
                     */
                    mtime: lastMTime,
                    /**
                     * @member request.Render
                     * @property {Number} quality
                     * Image compression quality.
                     * -   0: Looks Bad but small in size.
                     * - 100: Looks good bug big in size.
                     */
                    quality: quality,
                    /**
                     * @property {Number} localTime
                     * Local client time used to compute the round trip time cost.
                     * Equals to new Date().getTime().
                     */
                    localTime : new Date().getTime()
                }, done_callback = ondone;

                /**
                 * @member pv.Viewport
                 * @event render-start
                 * @param {Number} view
                 * Proxy View ID.
                 */
                viewport.trigger({
                    type: "render-start",
                    view: Number(config.view)
                });

                session.call("pv:stillRender", renderCfg).then(function (res) {
                    /**
                     * @class reply.Render
                     * Object returned from the server as a response to a
                     * stillRender request. It includes information about the
                     * rendered image along with the rendered image itself.
                     *
                     *    {
                     *       image     : "sdfgsdfg/==",      // Image encoding in a String
                     *       size      : [width, height],    // Image size
                     *       format    : "jpeg;base64",      // Image type + encoding
                     *       global_id : 234652436,          // View Proxy ID
                     *       stale     : false,              // Image is stale
                     *       mtime     : 23456345,           // Image MTime
                     *       localTime : 3563456,            // Value provided at request
                     *       workTime  : 10                  // Number of ms that were needed for the processing
                     *    }
                     */
                    /**
                     * @member reply.Render
                     * @property {String} image
                     * Rendered image content encoded as a String.
                     */
                    /**
                     * @member reply.Render
                     * @property {Array} size
                     * Size of the rendered image (width, height).
                     */
                    /**
                     * @member reply.Render
                     * @property {String} format
                     * String indicating the format and encoding for the image
                     * e.g. "jpeg;base64" or "png;base64".
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} global_id
                     * GlobalID of the view proxy from which the image is
                     * obtained.
                     */
                    /**
                     * @member reply.Render
                     * @property {Boolean} stale
                     * For better frame-rates when interacting, ParaView may
                     * return a stale rendered image, while the newly rendered
                     * image is being processed. This flag indicates that a new
                     * rendering for this view is currently being processed on
                     * the server.
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} mtime
                     * Timestamp of the generated image. This is used to prevent
                     * a redelivery of the same image.
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} localTime
                     * Unchanged value that was in the request. This will help
                     * to compute round trip cost.
                     */
                    /**
                     * @member reply.Render
                     * @property {Number} workTime
                     * Delta time that was needed on the server side to handle
                     * the request. This does not include the json parsing.
                     * Just the high level opeartion achieved by ParaView.
                     */
                    config.view = Number(res.global_id);
                    lastMTime = res.mtime;
                    if(res.hasOwnProperty("image") && res.image !== null) {
                        /**
                         * @member pv.Viewport
                         * @event start-loading
                         */
                        viewport.trigger("start-loading");
                        bgImage.width  = res.size[0];
                        bgImage.height = res.size[1];
                        bgImage.src = "data:image/" + res.format  + "," + res.image;
                        
                        /**
                         * @member pv.Viewport
                         * @event render-end
                         * @param {Number} view
                         * Proxy View Id.
                         */
                        viewport.trigger({
                            type: "render-end",
                            view: Number(config.view)
                        });

                        /** 
                         * @member pv.Viewport
                         * @event round-trip
                         * @param {Number} time
                         * Time between the sending and the reception of an image less the
                         * server processing time.
                         */
                        viewport.trigger({
                            type: "round-trip",
                            time: Number(new Date().getTime() - res.localTime) - res.workTime
                        });
                        
                        /**
                         * @member pv.Viewport
                         * @event server-processing
                         * @param {Number} time
                         * Delta time between the reception of the message on the server and
                         * when the reply is construct and return from the method.
                         */
                        viewport.trigger({
                            type: "server-processing",
                            time: Number(res.workTime)
                        });
                    }
                    renderStatistics();
                    force_render = false;
                    if (done_callback) {
                        done_callback();
                    }
                    // the image we received is not the latest, we should
                    // request another render to try to get the latest image.
                    if (res.stale === true) {
                        renderOnIdle();
                    }
                });
            }
        }

        // ----
        function mouseInteraction(action, evt) {
            // stop default event handling by the browser.
            evt.preventDefault();

            /**
             * @class request.InteractionEvent
             * Container Object used to encapsulate MouseEvent status
             * formated in an handy manner for ParaView.
             * 
             *     {
             *       view         : 23452345, // View proxy globalId
             *       action       : "down",   // Enum["down", "up", "move"]
             *       charCode     : "",       // In key press will hold the char value
             *       altKey       : false,    // Is alt Key down ?
             *       ctrlKey      : false,    // Is ctrl Key down ?
             *       shiftKey     : false,    // Is shift Key down ?
             *       metaKey      : false,    // Is meta Key down ?
             *       buttonLeft   : false,    // Is button Left down ?
             *       buttonMiddle : false,    // Is button Middle down ?
             *       buttonRight  : false,    // Is button Right down ?
             *     }
             */
            var paraview_event = {
                /**
                 * @member request.InteractionEvent
                 * @property {Number}  view Proxy global ID
                 */
                view: Number(config.view),
                /**
                 * @member request.InteractionEvent
                 * @property {String}  action
                 * Type of mouse action and can only be one of:
                 * 
                 * - down
                 * - up
                 * - move
                 */
                action: action,
                /**
                 * @member request.InteractionEvent
                 * @property {String}  charCode
                 */
                charCode: evt.charCode,
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} altKey
                 */
                altKey: evt.altKey,
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} ctrlKey
                 */
                ctrlKey: evt.ctrlKey,
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} shiftKey
                 */
                shiftKey: evt.shiftKey,
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} metaKey
                 */
                metaKey: evt.metaKey,
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} buttonLeft
                 */
                buttonLeft: (current_button === 1 ? true : false),
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} buttonMiddle
                 */
                buttonMiddle: (current_button === 2 ? true : false),
                /**
                 * @member request.InteractionEvent
                 * @property {Boolean} buttonRight
                 */
                buttonRight: (current_button === 3 ? true : false)
            },
            elem_position = $(evt.delegateTarget).offset(),
            pointer = {
                x : (evt.pageX - elem_position.left),
                y : (evt.pageY - elem_position.top)
            };

            paraview_event.x = pointer.x / viewport.width();
            paraview_event.y = 1.0 - (pointer.y / viewport.height());
            if (eatMouseEvent(paraview_event)) {
                return;
            }

            action_pending = true;
            session.call("pv:mouseInteraction", paraview_event).then(function (res) {
                if (res) {
                    action_pending = false;
                    render();
                }
            });
        }

        // internal function to render stats.
        function renderStatistics() {
            if (statistics) {
                ctx2d.font = "bold 12px sans-serif";
                //ctx2d.fillStyle = "white";
                ctx2d.fillStyle = "black";
                ctx2d.fillRect(10, 10, 240, 100);
                //ctx2d.fillStyle = "black";
                ctx2d.fillStyle = "white";
                ctx2d.fillText("Frame Rate: " + statistics.frameRate().toFixed(2), 15, 25);
                ctx2d.fillText("Average Frame Rate: " + statistics.averageFrameRate().toFixed(2),
                    15, 40);
                ctx2d.fillText("Round trip: " + statistics.roundTrip() + " ms - Max: " + statistics.maxRoundTrip() + " ms",
                    15, 55);
                ctx2d.fillText("Server work time: " + statistics.serverWorkTime() + " ms - Max: " + statistics.maxServerWorkTime() + " ms",
                    15, 70);
                ctx2d.fillText("Minimum Frame Rate: " + statistics.minFrameRate().toFixed(2),
                    15, 85);
                ctx2d.fillText("Loading time: " + statistics.trueLoadTime(),
                    15, 100);
            }
        }

        // Choose if rendering is happening in Canvas or image
        if (config.useCanvas) {
            viewport = canvas;

            // If canvas not supported add image in bg
            //canvas.append(bgImage);

            // When image ready draw on canvas
            bgImage.onload = function () {
                /**
                 * @member pv.Viewport
                 * @event stop-loading
                 */
                viewport.trigger("stop-loading");
                canvas.attr("width", bgImage.width);
                canvas.attr("height", bgImage.height);
                ctx2d.drawImage(bgImage, 0, 0, bgImage.width, bgImage.height);
                renderStatistics();
            };
        } else {
            viewport = $(bgImage).attr("alt", "ParaView (JavaScript) Renderer");
        }
        
        // Extend touch event to mockup normal mouse event
        function addTouchSupport(event) {
            if (event.which === 0) {
                // Touch event
                event = $.extend(event, {
                    which: 1,
                    charCode: '',
                    altKey: false,
                    shiftKey: false,
                    metaKey: false,
                    ctrlKey: false
                });
            }
            return event;
        }

        // Attach mouse listeners if needed
        if (config.enableInteractions) {
            viewport.bind("contextmenu click", function (evt) {
                evt.preventDefault();
            });
            
            viewport.bind(headEvent + "mousedown", function (evt) {
                evt = addTouchSupport(evt);
                current_button = evt.which;
                quality = config.interactiveQuality;
                mouseInteraction("down", evt);
            });
            viewport.bind(headEvent + "mouseup", function (evt) {
                evt = addTouchSupport(evt);
                current_button = null;
                mouseInteraction("up", evt);
                quality = config.stillQuality;
            });
            viewport.bind(headEvent + "mousemove", function (evt) {
                evt = addTouchSupport(evt);
                mouseInteraction("move", evt);
            });
        }

        return {
            /**
             * @class pv.Viewport
             * This Object let you attach a remote viewport into your web page
             * and forward remotely the mouse interaction while keeping the
             * content of the viewport up-to-date.
             */
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
                    container.append(viewport);
                    render();
                }
            },
            /**
             * Remove viewport from DOM element
             */
            unbind: function () {
                var parentElement = viewport.parent();
                if (parentElement) {
                    parentElement.attr("__pv_viewport__", false);
                    viewport.remove();
                }
            },
            /**
             * Trigger a render on the server side and update the image locally.
             *
             *      view.render(function() { console.log("rendering done"); });
             *
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            render: function (ondone) {
                render(ondone);
            },
            /**
             * Reset the camera for the given view
             * 
             * @member pv.Viewport
             * @param {Function} ondone Function to call after rendering is complete.
             */
            resetCamera: function (ondone) {
                return session.call("pv:resetCamera", Number(config.view)).then(function () {
                    render(ondone);
                });
            },
            /**
             * Provides access to the HTMLElement used for rendering.
             *
             * @member pv.Viewport
             * @return {Object} Canvas or Image element used for rendering.
             */
            getHTMLElement : function () {
                return viewport;
            },

            /**
             * Display the statics collected by the object in the view.
             *
             * @member pv.Viewport
             * @param {pv.ViewportStatistics|null} stats
             * The ViewportStatistics object to use to show the statistics in
             * this view. To stop showing the statistics, simply call this
             * function with null or on arguments.
             */
            showStatistics : function (stats) {
                if (stats !== null) {
                    statistics = stats;
                } else {
                    statistics = null;
                }
            }
        };
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
    module.createViewport = function (session, option) {
        return createViewport(session, option);
    };
}(window, jQuery));
