/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * @class paraview.ui.PipelineBrowser
 */
(function (GLOBAL, $) {

    var SlideSpeed = 250,
    DEFAULT_PIPELINE = {
        name: "kitware.com",
        type: 'server',
        children: [{
            proxy_id: 234,
            name: "Cow.vtu",
            type: 'dataset',
            representation: 'surface',
            showScalarBar: false,
            pointData: ['a','b','c'],
            cellData: ['aa','bb','cc'],
            activeData: 'p:a',
            children: [{
                proxy_id: 235,
                name: "Iso",
                type: 'contour',
                representation: 'surface',
                showScalarBar: false,
                pointData: ['a','b','c'],
                cellData: ['aa','bb','cc'],
                activeData: 'p:a',
                children: [{
                    proxy_id: 236,
                    name: "Clip",
                    type: 'clip'
                }]
            }, {
                proxy_id: 237,
                name: "Slice",
                type: 'slice',
                representation: 'surface',
                showScalarBar: true,
                pointData: ['a','b','c'],
                cellData: ['aa','bb','cc'],
                activeData: 'p:a'
            }, {
                proxy_id: 238,
                name: "Stream Tracer",
                type: 'stream',
                representation: 'surface',
                showScalarBar: false,
                pointData: ['a','b','c'],
                cellData: ['aa','bb','cc'],
                activeData: 'p:a'
            }, {
                proxy_id: 239,
                name: "Threshold",
                type: 'threshold',
                representation: 'surface',
                showScalarBar: false,
                pointData: ['a','b','c'],
                cellData: ['aa','bb','cc'],
                activeData: 'p:a'
            }, {
                proxy_id: 240,
                name: "Random filter",
                type: 'filter',
                representation: 'surface',
                showScalarBar: false,
                pointData: ['ac'],
                cellData: ['t','y'],
                activeData: 'c:t'
            }]
        }, {
            proxy_id: 240,
            name: "Random source",
            type: 'dataset',
            representation: 'surface',
            showScalarBar: false,
            pointData: ['z','d','r'],
            cellData: ['d'],
            activeData: 'p:z'
        }]
    },
    DEFAULT_SOURCES = [{
        name: 'Cone',
        type: 'dataset',
        category: 'source'
    },{
        name: 'Clip',
        type: 'clip',
        category: 'filter'
    },{
        name: 'Slice',
        type: 'slice',
        category: 'filter'
    },{
        name: 'Contour',
        type: 'contour',
        category: 'filter'
    },{
        name: 'Threshold',
        type: 'threshold',
        category: 'filter'
    },{
        name: 'Stream Tracer',
        type: 'stream',
        category: 'filter'
    },{
        name: 'Warp',
        type: 'filter',
        category: 'filter'
    }],
    DEFAULT_FILES =  [{
        name: 'can.ex2',
        path: '/can.ex2'
    },{
        name: 'cow.vtu',
        path: '/cow.vtu'
    },{
        name: 'VTKData',
        path: '/VTKData',
        children: [{
            name: 'cow.vtu',
            path: '/VTKData/cow.vtu'
        },{
            name: 'sphere.vtu',
            path: '/VTKData/sphere.vtu'
        }]
    },{
        name: 'ParaViewData',
        path: '/ParaViewData',
        children: [{
            name: 'asfdg.vtu',
            path: '/ParaViewData/asfdg.vtu'
        },{
            name: 'can.ex2',
            path: '/ParaViewData/can.ex2'
        },{
            name: 'AnotherDir',
            path: '/ParaViewData/AnotherDir',
            children: [{
                name: 'aaaaa.vtu',
                path: '/ParaViewData/AnotherDir/aaaaa.vtu'
            },{
                name: 'bbbbb.ex2',
                path: '/ParaViewData/AnotherDir/bbbbb.ex2'
            }]
        }]
    }], buffer = null;


    // ======================= Private helper methods =======================

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

    // =============== Search given proxy in the provided tree ===============

    function searchProxy(id, listOfProxy) {
        var result = [], i;
        for(i in listOfProxy) {
            var node = listOfProxy[i];
            if(node.hasOwnProperty("proxy_id") && node.proxy_id === id) {
                return [node];
            }
            if(node.hasOwnProperty("children")) {
                result = result.concat(searchProxy(id, node.children));
            }
        }
        return result;
    }

    function getPipelineContainer(obj) {
        var container = obj, i = 0;
        while(container.data('pipeline') === undefined && ++i < 100) {
            container = container.parent();
        }
        return container;
    }


    // ======== addFilePanelToBuffer: Fill HTML to global buffer object ========

    function addFilePanelToBuffer(fileList, panelClassName, parentClassName) {
        if(fileList === null || fileList === undefined) {
            return;
        }

        var childrenList = [], i;
        buffer.append("<ul class='file-panel ");
        buffer.append(panelClassName);
        buffer.append("'>");
        if(parentClassName) {
            buffer.append("<li class='parent menu-link' link='");
            buffer.append(parentClassName);
            buffer.append("'>..</li>");
        }
        for(i in fileList) {
            if(fileList[i].hasOwnProperty("children") && fileList[i].children.length > 0) {
                buffer.append("<li class='child menu-link' link='");
                var obj = {
                    id: fileList[i].path.replace(/\//g, "_"),
                    children: fileList[i].children
                };
                childrenList.push(obj);
                buffer.append(obj.id);
                buffer.append("'>");
            } else {
                buffer.append("<li class='open-file' path='");
                buffer.append(fileList[i].path);
                buffer.append("'>");
            }
            buffer.append(fileList[i].name);
            buffer.append("</li>");
        }

        buffer.append("</ul>");

        // Add all child panels
        for(i in childrenList) {
            addFilePanelToBuffer(childrenList[i].children, childrenList[i].id, panelClassName);
        }
    }

    // ======== addSourcesToBuffer: Fill HTML to global buffer object ========

    function addSourcesToBuffer(sourceList) {
        if(sourceList === undefined || sourceList === null) {
            return;
        }

        // Build proxy line
        var i, item;

        buffer.append("<ul>");
        for(i in sourceList) {
            item = sourceList[i];
            buffer.append("<li class='action ");
            buffer.append(item.type);
            buffer.append("' category='");
            buffer.append(item.category);
            buffer.append("'>");
            buffer.append(item.name);
            buffer.append("</li>");
        }
        buffer.append("</ul>");
    }

    // === hasChild(proxy): Return true if proxy.children.lenght > 0 and exist

    function hasChild(proxy) {
        return (proxy && proxy.hasOwnProperty("children") && proxy.children.length > 0);
    }

    // ======== addPipelineToBuffer: Fill HTML to global buffer object ========

    function addPipelineToBuffer(title, data) {
        // Build header
        buffer.append("<div class='pipeline-header'><span class='title'>");
        buffer.append(title);
        buffer.append("</span><div class='action files'></div><div class='action add'></div><div class='action delete'></div></div>");
        // Build pipeline
        buffer.append("<div class='pipeline-tree'>");
        addProxyToBuffer(data.pipeline, hasChild(data.pipeline), true);
        buffer.append("</div>");

        // Build file selector
        buffer.append("<div class='pipeline-files'>");
        addFilePanelToBuffer(data.files, "ROOT", null);
        buffer.append("</div>");

        // Build source/filter selector
        buffer.append("<div class='pipeline-add'>");
        addSourcesToBuffer(data.sources);
        buffer.append("</div>");
    }


    // ==== addProxyToBuffer: Add a proxy with its children to the buffer ====

    function addProxyToBuffer(proxy, isLastChild, isRoot) {
        if(proxy === undefined || proxy === null) {
            return;
        }
        buffer.append("<li class='");
        buffer.append(proxy.type);
        if(isLastChild) {
            buffer.append(" lastChild");
        } else if (isRoot === false && proxy.hasOwnProperty("children") && proxy.children.length > 0) {
            buffer.append(" open");
        }
        buffer.append("'><div class='proxy proxy-id-");
        var proxyId = (proxy.hasOwnProperty('proxy_id') ? proxy.proxy_id : "0");
        buffer.append(proxyId);
        buffer.append("' proxy_id='");
        buffer.append(proxyId);
        buffer.append("'>");
        addProxySummaryToBuffer(proxy);
        buffer.append("</div>");
        if(proxy.hasOwnProperty("children")) {
            buffer.append("<ul>");
            var idx, len = Number(proxy.children.length) - 1;
            for(idx in proxy.children) {
                var child = proxy.children[idx];
                addProxyToBuffer(child, idx === len, false);
            }
            buffer.append("</ul>");
        }
        buffer.append("</li>");
    }

    // ==== addProxySummaryToBuffer: Add Proxy control line to the buffer ====

    function addProxySummaryToBuffer(proxy) {
        if(!proxy.hasOwnProperty('proxy_id')) {
            buffer.append(proxy.name);
            return;
        }

        // Add default empty valid field if missing
        // TODO... $.extend(...)

        // Build proxy line
        var i, currentValue;
        buffer.append(proxy.name);
        buffer.append("<span class='proxy-control'>");
        buffer.append("<div class='action representation ");
        buffer.append(proxy.representation);
        buffer.append("'></div><select class='data-array'>");
        for(i in proxy.pointData) {
            currentValue = 'p:' + proxy.pointData[i];
            buffer.append("<option value='p:");
            buffer.append(proxy.pointData[i]);
            if(currentValue === proxy.activeData) {
                buffer.append("' selected='selected' >");
            } else {
                buffer.append("'>");
            }
            buffer.append(proxy.pointData[i]);
            buffer.append("</option>");
        }
        for(i in proxy.cellData) {
            currentValue = 'c:' + proxy.cellData[i];
            buffer.append("<option value='c:");
            buffer.append(proxy.cellData[i]);
            if(currentValue === proxy.activeData) {
                buffer.append("' selected='selected' >");
            } else {
                buffer.append("'>");
            }
            buffer.append(proxy.cellData[i]);
            buffer.append("</option>");
        }
        buffer.append("</select>");
        buffer.append("<div class='action scalarbar ");
        buffer.append((proxy.showScalarBar) ? "active" : "");
        buffer.append("'></div>");
        buffer.append("</span>");
    }

    // ============= Initialize Pipeline (Listener + Visibility) =============

    function initialize (container) {
        // Main panel containers
        var pipelineView = $('.pipeline-tree', container),
        algoView = $('.pipeline-add', container),
        fileView = $('.pipeline-files', container);

        // Update visibility
        algoView.hide();
        fileView.hide();

        // ===================================================================
        // Delete should behave like:
        // (a) In Pipeline view
        //     (aa) if proxy selected and delete not disable:
        //          => trigger event to delete proxy
        //     (ab) if no proxy selected: nothing
        //     (ac) if the selection/no-selection can not be deleted the icon
        //          should be disable.
        // (b) In any other view mode (File or Source) that should switch back
        //     to the Pipeline view.
        // ===================================================================

        $(".delete",container).click(function(){
            // (a)
            if(pipelineView.is(":visible")) {
                var proxy = container.getProxy();
                if(proxy && !$(this).hasClass("disable")) {
                    // (aa) + (ab)
                    fireDeleteProxy(container, proxy);
                }
            } else {
                returnToPipelineBrowser(pipelineView, algoView, fileView);
            }
        });

        // ===================================================================
        // Add should behave like:
        //
        // (c) If the button is not disable
        //    (ca) In Pipeline view
        //        (caa) if active proxy => Show source list with the filter category
        //        (cab) if no proxy selected => Show only the sources in source list
        //    (cb) In any other view mode (File or Pipeline) that should switch
        //         back to the Pipeline view.
        // ===================================================================

        $(".add",container).click(function(){
            if(!$(this).hasClass("disable")) {
                // (c)
                if(pipelineView.is(":visible")) {
                    // (ca)
                    var proxy = container.getProxy();
                    if(proxy) {
                        // (caa)
                        $("li[category=filter]",container).show();
                    } else {
                        // (cab)
                        $("li[category=filter]",container).hide();
                    }
                    pipelineView.hide('slide', SlideSpeed, function(){
                        algoView.show('slide', SlideSpeed);
                    });
                } else {
                    // (cb)
                    returnToPipelineBrowser(pipelineView, algoView, fileView);
                }
            }
        });

        // ===================================================================
        // Files should behave like:
        //
        // (d) If the button is not disable
        //    (da) In Pipeline view
        //        (daa) Show the file list at root
        //    (db) In any other view mode (File or Pipeline) that should switch
        //         back to the Pipeline view.
        // ===================================================================

        $(".files",container).click(function(){
            if(!$(this).hasClass("disable")) {
                // (d)
                if(pipelineView.is(":visible")) {
                    // (da)
                    $('.file-panel', container).hide();
                    $('.file-panel.ROOT', container).show();
                    pipelineView.hide('slide', SlideSpeed, function(){
                        fileView.show('slide', SlideSpeed);
                    });
                } else {
                    // (db)
                    returnToPipelineBrowser(pipelineView, algoView, fileView);
                }
            }
        });

        // ===================================================================
        // Inside File browser view
        // ===================================================================

        $(".open-file", container).click(function() {
            returnToPipelineBrowser(pipelineView, algoView, fileView);
            fireOpenFile(container, $(this).attr('path'));
        });

        $(".menu-link", container).click(function() {
            var me = $(this);
            var menuToShow = $("." + me.attr('link'));
            me.parent().hide('slide', 250, function(){
                menuToShow.show('slide',250);
            });
        });

        // ===================================================================
        // Inside Source/Algo view
        // ===================================================================

        $(".pipeline-add .action", container).click(function(){
            returnToPipelineBrowser(pipelineView, algoView, fileView);
            var me = $(this);
            var proxy = container.getProxy();
            var parentId = (me.attr('category') === 'filter' && proxy) ? proxy.proxy_id : null;
            fireAddSource(container, me.text(), parentId);
        });

        // ===================================================================
        // Inside Pipeline view
        // ===================================================================

        $(".scalarbar", container).click( function() {
            var me = $(this).toggleClass("active");
            var proxyId = me.parent().parent().attr("proxy_id");
            var proxy = container.getProxy(proxyId);
            if(proxy) {
                proxy.showScalarBar = me.hasClass("active");
                fireProxyChange(container, proxy, 'showScalarBar');
            }
        });

        $(".representation", container).click(selectRepresentation);

        $('.data-array', container).change(function(){
            var me = $(this),
            id = me.parent().parent().attr("proxy_id"),
            proxy = container.getProxy(id);
            if(proxy) {
                proxy.activeData = me.val();
                fireProxyChange(container, proxy, 'activeData');
            }
        });

        $(".proxy", container).click(function(){
            // Handle style classes
            $(".proxy").removeClass('active');
            $(".proxy-control").removeClass('active');
            $('.proxy-control', this).addClass('active').parent().addClass('active');

            // Handle active proxy
            var id = $(this).attr('proxy_id'),
            proxy = container.getProxy(id),
            activeProxy = container.getProxy();

            if(proxy !== activeProxy){
                container.data('active_proxy', proxy);
                fireProxySelected(container, proxy);
            }

            // Handle delete disable attribute (ac)
            if(proxy === null || hasChild(proxy)) {
                $('.pipeline-header .delete', container).addClass("disable");
            } else {
                $('.pipeline-header .delete', container).removeClass("disable");
            }
        });
    }

    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member paraview.ui.PipelineBrowser
     * @method pipelineBrowser
     * @param {pv.PipelineBrowserConfig} options
     *
     * Usage:
     *      $('.pipeline-container-div').pipelineBrowser({
     *          session: sessionObj,
     *          pipeline: pipeline,
     *          sources: sourceList,
     *          files: fileList
     *      });
     */
    $.fn.pipelineBrowser = function(options) {
        return this.each(function(options) {
            var me = $(this).empty().addClass('pipelineBrowser'), opts;

            // Initialize global html buffer
            if (buffer === null) {
                buffer = createBuffer();
            }
            buffer.clear();

            // Handle data with default values
            opts = $.extend({},$.fn.pipelineBrowser.defaults, options);

            // Fill buffer with pipeline HTML
            addPipelineToBuffer("Pipeline", opts);

            // Update DOM
            me.data('pipeline', opts);
            me[0].innerHTML = buffer.toString();

            // Initialize pipelineBrowser (Visibility + listeners)
            initialize(me);
        });
    };

    /**
     * @class pv.PipelineBrowserConfig
     * Configuration object used to create a Pipeline Browser Widget.
     *
     *     DEFAULT_VALUES = {
     *       session: null,
     *       pipeline: DEFAULT_PIPELINE,
     *       sources: DEFAULT_SOURCES,
     *       files: DEFAULT_FILES
     *     }
     */
    $.fn.pipelineBrowser.defaults = {
        /**
         * @member pv.PipelineBrowserConfig
         * @property {pv.Session} session
         * Session used to be attached with the given pipeline.
         */
        session: null,
        /**
         * @member pv.PipelineBrowserConfig
         * @property {reply.Pipeline} pipeline
         * Pipeline used to initialized the widget.
         */
        pipeline: DEFAULT_PIPELINE,
        /**
         * @member pv.PipelineBrowserConfig
         * @property {pv.Algorithm} sources[]
         * List of source and filters available for the pipeline.
         */
        sources: DEFAULT_SOURCES,
        /**
         * @member pv.PipelineBrowserConfig
         * @property {reply.FileList[]} files
         * List of files and directory accessible to the pipeline browser.
         */
        files: DEFAULT_FILES
    };

    /**
     * Method used to retreive a proxy from the Pipeline browser.
     * If the proxyId is null/undefined the selected proxy will be returned.
     *
     * @member paraview.ui.PipelineBrowser
     * @method getProxy
     * @param {Number|undefined|null} proxyId
     * @return {pv.Proxy} proxy that have the given id or null if not found.
     *
     * Usage:
     *      var selectedProxy = $('.pipeline-container-div').getProxy();
     *      var proxy = $('.pipeline-container-div').getProxy(1234);
     */

    $.fn.getProxy = function(proxyId) {
        if(proxyId === null || proxyId === undefined) {
            return $(this).data('active_proxy');
        }
        var data = $(this).data('pipeline'), id = Number(proxyId);
        var result = searchProxy(id, [data.pipeline]);
        if(result.length === 1) {
            return result[0];
        }
        return null;
    };

    // ======================= Listener helper functions =======================

    function returnToPipelineBrowser(pipelineView, algoView, fileView) {
        if(algoView.is(":visible")) {
            // (b)
            algoView.hide('slide', SlideSpeed, function(){
                pipelineView.show('slide', SlideSpeed, function(){
                    // init what need to be init under the cover
                    });
            });
        } else if(fileView.is(":visible")) {
            // (b)
            fileView.hide('slide', SlideSpeed, function(){
                pipelineView.show('slide', SlideSpeed, function(){
                    // init what need to be init under the cover
                    $('.file-panel', fileView).hide();
                });
            });
        }
    }

    // =========================================================================

    function selectRepresentation() {
        $(this).removeClass('hide outline wireframe surface surface_edge').unbind('click').bind('click', chooseRepresentation);
    }

    // =========================================================================

    function chooseRepresentation(e) {
        var me = $(this),
        i = 0, container = getPipelineContainer(me),
        pos = (e.pageX - me.offset().left)/20,
        className = ['hide','outline','wireframe','surface','surface_edge'][Math.floor(pos)],
        id = me.addClass(className).unbind('click').bind('click', selectRepresentation).parent().parent().attr("proxy_id"),
        proxy = container.getProxy(id);

        // Update proxy field
        proxy.representation = className;

        fireProxyChange(container, container.getProxy(id), 'representation');
    }


    // ========================== Event fire methods ==========================

    /**
     * Event that get triggered when a Proxy get deleted.
     * @member paraview.ui.PipelineBrowser
     * @event deleteProxy
     * @param {pv.Proxy} proxy
     * Proxy that is getting deleted.
     */
    function fireDeleteProxy(container, proxy) {
        container.trigger({
            type: 'deleteProxy',
            proxy_id: proxy.proxy_id,
            proxy: proxy
        });
    }

    /**
     * Event that get triggered when a request for a new file open is made.
     * @member paraview.ui.PipelineBrowser
     * @event openFile
     * @param {String} path
     * File path that is requested to be open.
     */
    function fireOpenFile(container, filePath) {
        container.trigger({
            type: 'openFile',
            path: filePath
        });
    }

    /**
     * Event that get triggered when a source or a filter is getting added.
     * @member paraview.ui.PipelineBrowser
     * @event addSource
     * @param {String} name
     * Name of the SourceProxy to be created.
     * @param {Number} parent
     * Global Id of the parent Proxy if any, null otherwise.
     */
    function fireAddSource(container, algoName, parent) {
        container.trigger({
            type: 'addSource',
            name: algoName,
            parent: parent
        });
    }

    /**
     * Event triggered when a Proxy has changed.
     *
     * @member paraview.ui.PipelineBrowser
     * @event proxyModified
     * @param {pv.Proxy} proxy
     * Proxy that has been modified.
     * @param {String} field
     * Name of the field that has been modified.
     */
    function fireProxyChange(container, proxy, fieldName) {
        container.trigger({
            type: 'proxyModified',
            proxy: proxy,
            field: fieldName
        });
    }

    /**
     * Event triggered when selection change in the Pipeline browser.
     * @member paraview.ui.PipelineBrowser
     * @event proxySelected
     * @param {pv.Proxy} proxy
     * Proxy that get selected in the Pipeline. Can be null if no selection.
     */
    function fireProxySelected(container, proxy) {
        container.trigger({
            type: 'proxySelected',
            proxy: proxy
        });
    }

}(window, jQuery));