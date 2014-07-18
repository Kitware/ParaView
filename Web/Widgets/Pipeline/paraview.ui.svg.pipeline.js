/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-svg-pipeline'
 *
 * @class jQuery.paraview.ui.Pipeline
 */
(function (GLOBAL, $) {

    // =======================================================================
    // ==== Defaults constant values =========================================
    // =======================================================================

    var TEMPLATE_BRANCH = "<path d='MAA,BB LAA,CC QAA,DD,EE,FF TGG,HH' stroke='COLOR' stroke-width='STROKE' fill='none' />",
        TEMPLATE_SVG_HEAD = "<svg width='100%' height='HEIGHT'>",
        TEMPLATE_SVG_TAIL = '</svg>',
        TEMPLATE_ENTRY = "<circle pv-proxy-id='ID' cx='CX' cy='CY' r='6' stroke='STROKE_COLOR' stroke-width='2' fill='FILL_COLOR' /><text pv-proxy-id='ID' x='TX' y='TY' fill='TCOLOR' font-weight='WEIGHT' >TXT</text>",
        TEMPLATE_ACTIVE_LINE = "<rect fill='#999' pv-proxy-id='ID' height='20' width='1000' x='-50' y='Y'/>",
        COLOR_PALETTE = ["#e1002a", "#417dc0", "#1d9a57", "#e9bc2f", "#9b3880"],
        buffer = null,
        error = function(e){console.log(e);};

    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.ui.Pipeline
     * @method pipelineSVG
     * @param {Object} options
     *
     *  Usage:
     *
     *      $('.pipeline-container-div').pipelineBrowser({
     *          session: null,
     *          offset: 15,
     *          grid_spacing: [15, 25],
     *          stroke: 4,
     *          radius: 6,
     *          proxy_list: [],
     *          leafs: [],
     *          active_proxy: '0',
     *          active_view: -1,
     *          palette: COLOR_PALETTE
     *      });
     *
     *  Events: (DEPRECATED updateProxyVisibility updateActiveProxy)
     *
     *    { type: 'pipeline-visibility-change', visible: proxy.visible, id: proxy.id, name: proxy.name }
     *
     *    { type: 'pipeline-data-change', active: active_proxy_id, view: view_id, sources: proxy_list }
     *
     *  Listen on events:
     *
     *      Use session to get data  : 'pipeline-reload' => will trigger 'pipeline-changed' when done
     *      Use local data           : 'pipeline-invalidate' => will trigger 'pipeline-changed' when done
     */
    $.fn.pipelineSVG = function(options) {
        // Handle data with default values
        var opts = $.extend({},$.fn.pipelineSVG.defaults, options);

        return this.each(function() {
            var me = $(this).empty().addClass('pipelineSVG'),
            session = opts.session;

            // Initialize global html buffer
            if (buffer === null) {
                buffer = createBuffer();
            }
            buffer.clear();

            // Update DOM
            me.data('pipeline', opts);
            processProxyList(me);
            updatePipelineBuffer(me);

            // Initialize pipelineBrowser (Visibility + listeners)
            initializeListener(me);

            // Attach RPC method if possible
            attachSessionController(me);
        });
    };

    $.fn.pipelineSVG.defaults = {
        session: null,
        offset: 15,
        grid_spacing: [15, 25],
        stroke: 4,
        radius: 6,
        proxy_list: [],
        leafs: [],
        active_proxy: '0',
        active_view: -1,
        palette: COLOR_PALETTE
    };

    // =======================================================================
    // ==== Events triggered on the pipelineBrowser ==========================
    // =======================================================================

    function fireToggleProxyVisibility(container, proxy_id) {
        var list = container.data('pipeline').proxy_list,
            proxy = getProxy(list, proxy_id);

        // Update data model
        proxy.visible = !proxy.visible;

        container.trigger({
            type: 'pipeline-visibility-change',
            visible: proxy.visible,
            id: proxy.id,
            name: proxy.name
        });
    }

    // ------------------------------------------------------------------------

    function firePipelineChange(container) {
        var data = container.data('pipeline');
        container.trigger({
            type: 'pipeline-data-change',
            active: data.active_proxy,
            view: data.active_view,
            sources: data.proxy_list
        });
    }

    // ------------------------------------------------------------------------

    function updateActiveProxy(container, proxy_id) {
        var change = (container.data('pipeline').active_proxy !== proxy_id);
        if(change) {
            container.data('pipeline').active_proxy = proxy_id;
        }
        return change;
    }

    // =======================================================================
    // ==== Data Model Access ================================================
    // =======================================================================

    function getProxy(list, id) {
        var count = list.length;
        while(count--) {
            if(list[count].id === id) {
                return list[count];
            }
        }
        return null;
    }

    // -----------------------------------------------------------

    function processProxyList(container) {
        var hierarchy = {},
            proxyMap = {},
            leafs = [],
            count = 0,
            globalPosition = 0,
            list = container.data('pipeline').proxy_list;

        function registerProxy(proxy) {
            if(!hierarchy.hasOwnProperty(proxy.parent)) {
                hierarchy[proxy.parent] = [];
            }
            if(!hierarchy.hasOwnProperty(proxy.id)) {
                hierarchy[proxy.id] = [];
            }

            hierarchy[proxy.parent].push(proxy.id);
            proxyMap[proxy.id] = proxy;
        }

        function sortIds(a,b) {
            return Number(b) - Number(a);
        }

        function sortProxy(a,b) {
            return a.position - b.position;
        }

        function assignBranchId(proxyId, branchId) {
            var children = hierarchy[proxyId],
                nbChildren = children.length;

            // Assign branch
            proxyMap[proxyId].branch = branchId;
            proxyMap[proxyId].position = globalPosition++;
            proxyMap[proxyId].leaf = (nbChildren === 0);

            // Handle children branches
            if(nbChildren === 0) {
                leafs.push(proxyId);
            } else {
                children.sort(sortIds);
                for(var idx = 0; idx < nbChildren; ++idx) {
                    assignBranchId(children[idx], branchId + (nbChildren - idx - 1));
                }
            }
        }

        // Fill internal structure
        count = list.length;
        while(count--) {
           registerProxy(list[count]);
        }

        // Assign branches + position to proxies
        if(hierarchy.hasOwnProperty('0')) {
            var branchId = 0,
                rootNodes = hierarchy["0"],
                nbRootNodes = rootNodes.length;

            rootNodes.sort(sortIds);
            for(var idx = 0; idx < nbRootNodes; ++idx) {
                assignBranchId(rootNodes[idx], 0);
            }

            // Sort proxy in the list
            list.sort(sortProxy);
        }

        container.data('pipeline').leafs = leafs;
    }

    // =======================================================================
    // ==== HTML code generators =============================================
    // =======================================================================

    function updatePipelineBuffer(container) {
        var data = container.data('pipeline'),
            offset = data.offset,
            deltaX = data.grid_spacing[0],
            deltaY = data.grid_spacing[1],
            stroke = data.stroke,
            radius = data.radius,
            count = 0,
            palette = data.palette,
            paletteSize = palette.length,

            list        = data.proxy_list,
            activeProxy = getProxy(list, data.active_proxy),
            leafs       = data.leafs;

            // Reset buffer
            buffer.clear();

            // Add SVG container
            buffer.append(TEMPLATE_SVG_HEAD.replace(/HEIGHT/g, (2*offset) + ((list.length-1) * deltaY)));

            // Add active line proxy
            buffer.append(TEMPLATE_ACTIVE_LINE.replace(/Y/g, (activeProxy ? (offset - 10 + deltaY * (activeProxy.position)) : -50)).replace(/ID/g, (activeProxy ? activeProxy.id : "0")));

            // Add branches
            count = leafs.length;
            while(count--) {
                var startX, startY,
                    endY,
                    finalX, finalY,
                    leafProxy = getProxy(list, leafs[count]),
                    currentProxy = leafProxy;

                    if(leafProxy.parent === '0') {
                        continue;
                    }

                    startX = offset + (leafProxy.branch) * deltaX;
                    startY = offset + leafProxy.position * deltaY;

                    while(currentProxy && currentProxy.branch === leafProxy.branch) {
                        currentProxy = getProxy(list, currentProxy.parent);
                        if(currentProxy) {
                            endY = offset + currentProxy.position * deltaY;
                        }

                    }
                    if(currentProxy) {
                        finalX = offset + (currentProxy.branch) * deltaX;
                        finalY = offset + currentProxy.position * deltaY;
                    } else {
                        finalX = startX;
                        finalY = endY;
                    }

                    buffer.append(TEMPLATE_BRANCH
                        .replace(/AA/g, startX)
                        .replace(/BB/g, startY)
                        .replace(/CC/g, (endY + deltaY) )
                        .replace(/DD/g, (endY + (deltaY / 2)))
                        .replace(/EE/g, (startX+finalX)/2 )
                        .replace(/FF/g, (endY+(deltaY/2)))
                        .replace(/GG/g, finalX)
                        .replace(/HH/g, finalY)
                        .replace(/COLOR/g, palette[ leafProxy.branch % paletteSize])
                        .replace(/STROKE/g, 4)
                    );
                }

            // Add proxies
            count = list.length;
            while(count--) {
                var proxy = list[count],
                    color = palette[ proxy.branch % paletteSize],
                    visible = proxy.visible,
                    active = (activeProxy === proxy),
                    cx = offset + (deltaX * proxy.branch),
                    cy = offset + (deltaY * proxy.position),
                    strokeColor = active ? 'black' : color,
                    fillColor = visible ? color : 'white',
                    txt = proxy.name,
                    tx = cx + deltaX,
                    ty = cy + 5,
                    weight = active ? 'bold' : '500',
                    tcolor = active ? 'white' : 'black';

                buffer.append(TEMPLATE_ENTRY
                    .replace(/TXT/g, txt)
                    .replace(/CX/g, cx)
                    .replace(/CY/g, cy)
                    .replace(/STROKE_COLOR/g, strokeColor)
                    .replace(/FILL_COLOR/g, fillColor)
                    .replace(/ID/g, proxy.id)
                    .replace(/TX/g, tx)
                    .replace(/TY/g, ty)
                    .replace(/TCOLOR/g, tcolor)
                    .replace(/WEIGHT/g, weight)
                );
            }

            // close SVG container
            buffer.append(TEMPLATE_SVG_TAIL);

            // Update DOM
            container[0].innerHTML = buffer.toString();
    }

    // =======================================================================
    // ==== Helper internal functions ========================================
    // =======================================================================

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

    // =======================================================================
    // ==== Controller methods (event > actions) =============================
    // =======================================================================

    function attachSessionController(container) {
        var dataObject = container.data('pipeline');
        // Is it a pipelineBrowser
        if(!dataObject.hasOwnProperty('session')) {
            // No job for us...
            return;
        }

        // Get session
        var session = dataObject.session;
        if(session === null || session === undefined) {
            // No job for us...
            return;
        }

        container.bind('pipeline-visibility-change', function(event){
            var proxy = getProxy(dataObject.proxy_list, event.id);
            session.call("pv.proxy.manager.update", [[{id: proxy.rep, name: 'Visibility', value: (event.visible ? 1 : 0) }]]).then(function(){
                container.trigger('pipeline-reload');
            }, error);
        });

        container.bind('pipeline-reload', function(event){
            session.call('pv.proxy.manager.list', []).then(function(data) {
                container.data('pipeline').proxy_list = data.sources;
                container.data('pipeline').active_view = data.view;
                processProxyList(container);
                updatePipelineBuffer(container);
                firePipelineChange(container);
            }, function(err){
                console.log('get pipeline error');
                console.log(err);
            });
        }).trigger('pipeline-reload');
    }

    // =======================================================================
    // ==== UI Listeners =====================================================
    // =======================================================================

    function initializeListener(container) {
        var data = container.data('pipeline'),
            offset = data.offset,
            deltaX = data.grid_spacing[0],
            deltaY = data.grid_spacing[1];

        container.on('click', function(event){
            var activeProxyId = event.target.getAttribute('pv-proxy-id'),
                changeDetected = false;

            // If circle then toggle visibility as well
            if(event.target.nodeName === 'circle') {
                updateActiveProxy(container, activeProxyId);
                fireToggleProxyVisibility(container, activeProxyId);
                changeDetected = true;
            } else if (event.target.nodeName === 'svg'){
                var index = Math.floor((event.clientY - event.target.parentNode.getBoundingClientRect().top - offset + (deltaY/2)) / deltaY);
                if(index > -1 && index < data.proxy_list.length) {
                    activeProxyId = data.proxy_list[index].id;
                }
            }
            // In case we don't have a valid proxy set '0' as active
            if (activeProxyId){
                changeDetected = updateActiveProxy(container, activeProxyId) || changeDetected;
            } else {
                changeDetected = updateActiveProxy(container, '0') || changeDetected;
            }

            // Update Content
            updatePipelineBuffer(container);

            if(changeDetected) {
                firePipelineChange(container);
            }
        });

        // Update data model and UI when an invalidate event get triggered
        container.bind('pipeline-invalidate', function(){
            processProxyList(container);
            updatePipelineBuffer(container);
            firePipelineChange(container);
        });
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-svg-pipeline');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
