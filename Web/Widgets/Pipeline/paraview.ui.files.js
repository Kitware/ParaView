/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-files'
 *
 * @class jQuery.paraview.ui.Pipeline
 */
(function (GLOBAL, $) {
    var TEMPLATE_BASE = "<div class='row path'><ul class='breadcrumb'>PATH</ul></div><div class='row directories'><ul class='list-unstyled' style='width: 100%;margin-bottom:0;'>DIRS</ul></div><div class='row groups'><ul class='list-unstyled' style='width: 100%;margin-bottom:0;'>GROUPS</ul></div><div class='row files'><ul class='list-unstyled' style='width: 100%;margin-bottom:0;'>FILES</ul></div>",
        TEMPLATE_PATH_ITEM = "<li class='clickable' data-basepath='FULLPATH' data-file='NAME' data-action='path'>NAME</li>",
        TEMPLATE_LIST_ITEM = "<li class='clickable hover-higlight pv-side-padding' data-file='FILE' data-action='ACTION' data-basepath='BASEPATH'>ICON LABEL</li>",
        TEMPLATE_DIVIDER = "<span class='divider'>",
        TEMPLATE_OPTION = "<option value='VALUE' SELECTED>LABEL</option>";

    /**
     * Graphical component use to create a browsable list of files and directories.
     *
     * @member jQuery.paraview.ui.Pipeline
     * @method fileBrowser2
     * @param {Object} data
     *     Object that you receive from the ParaViewWebFileListing protocol
     *
     * Usage:
     *
     *      $('.file-browser').fileBrowser2( {...} );
     *
     * Events:
     *
     *     {
     *        type: 'file-action',
     *        action: action,   # ['directory', 'file' ] Click was on a directory (need to fetch content) or a file (need to load it)
     *        path: basePath,   # Base path in which the files belong to
     *        files: files      # ['fileName'] is simple file or directory or ['file1', 'file2'] when a group is clicked
     *     }
     *
     *
     */
    $.fn.fileBrowser2 = function(data) {
        return this.each(function() {
            var me = $(this).empty().addClass('pv-file-browser'),
                pathBuffer = [],
                dirsBuffer = [],
                groupsBuffer = [],
                filesBuffer = [],
                count = -1,
                basepath = data.path.join('/');

            // PATH
            count = data.path.length;
            for(var i = 0; i < count; ++i) {
                pathBuffer.push(TEMPLATE_PATH_ITEM
                    .replace(/FULLPATH/g, data.path.slice(0, i).join('/'))
                    .replace(/NAME/g, data.path[i]));
            }

            // DIRECTORY
            count = data.dirs.length;
            for(var i = 0; i < count; ++i) {
                var item = data.dirs[i];
                dirsBuffer.push(TEMPLATE_LIST_ITEM
                    .replace(/FILE/g, item)
                    .replace(/BASEPATH/g, basepath)
                    .replace(/ACTION/g, 'directory')
                    .replace(/ICON/g, '<span class="vtk-icon-folder-empty"></span>')
                    .replace(/LABEL/g, item));
            }
            // GROUP
            count = data.groups.length;
            for(var i = 0; i < count; ++i) {
                var item = data.groups[i];
                groupsBuffer.push(TEMPLATE_LIST_ITEM
                    .replace(/FILE/g, item.files)
                    .replace(/BASEPATH/g, basepath)
                    .replace(/ACTION/g, 'file')
                    .replace(/ICON/g, '<span class="vtk-icon-doc-text"></span>')
                    .replace(/LABEL/g, item.label));
            }
            // FILES
            count = data.files.length;
            for(var i = 0; i < count; ++i) {
                var item = data.files[i];
                filesBuffer.push(TEMPLATE_LIST_ITEM
                    .replace(/FILE/g, item.label)
                    .replace(/BASEPATH/g, data.path.join('/'))
                    .replace(/ACTION/g, 'file')
                    .replace(/ICON/g, '<span class="vtk-icon-doc"></span>')
                    .replace(/LABEL/g, item.label));
            }


            me[0].innerHTML = TEMPLATE_BASE
                .replace(/PATH/g, pathBuffer.join(''))
                .replace(/DIRS/g, dirsBuffer.join(''))
                .replace(/GROUPS/g, groupsBuffer.join(''))
                .replace(/FILES/g, filesBuffer.join(''));

            // Need to add listeners
            $('.clickable', me).bind('click', function(){
                var item = $(this),
                    basePath = item.attr('data-basepath'),
                    files = item.attr('data-file'),
                    action = item.attr('data-action');

                me.trigger({
                    type: 'file-action',
                    action: action,
                    path: basePath,
                    files: files
                });
            });
            // todo fixme
        });
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-files');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));