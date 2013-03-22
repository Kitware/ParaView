jColumnListView

Author:  Alexander Khizha <khizhaster@gmail.com>
License: BSD

Description
Simple Finder-like control that can be used instead of <select> to display hierarchical data in columns view. Supports multiselect, separate selecting and checking for items and has labels area to display selected items. Creates an <input> element for each checked item, so you can grab an array of elements on the server. Uses a separate CSS with class names prefixed with 'cvl-'.

Demo site
http://knell.ho.ua/jcvl/

Issues and Wishes
Please, feel free to contact me by e-mail (can be found in .js file) or create an issue in case you found some errors or want some features to be implemented.

Version 0.2
Splitters added for column. Now it's possible to set min and max width of columns, enable useSplitters flag and change column width with mouse dragging. Also leftMode is available, in this mode only left column will be modified.

Version 0.2.3
Added function setValues(). It allows to set up checked items with given values. This function will search for items, makes it checked and sets labels.

Version 0.2.4
Implemented single check mode. Set 'singleCheck' parameter to true and only one item can be check at one time.

Version 0.3
Implemented auto-scroll. It works in two ways:

If you click item with children and new column will not fit view area then view will be scrolled to show new column completely.
If you click item without children or empty space of column and current column does not fit view area then view will be scrolled to show current column completely.
Fixed removing of check marks for children items.

Version 0.3.1
Implemented support of values for items. Now control can read 'itemValue' attribute for <li> items in list. See updated example below. If attribute 'itemValue' is not present then text will take as value.
Removed dependency on comma (,) item's text or value. Now it's possible to set text for item with commas, and for values too. Also, it will be useful for similar items in different categories. Now you can set different values for them.
Version 0.4 (0.3.2)
Implemented 'leafMode'. If this mode enabled control will store only leaf elements (that have not any children items). Version 0.3.2 promoted to 0.4 (Today is 04.04 :)).

Version 0.4.1
From version 0.4.1 it's possible to set up format of item's text. There are three parameters to do it.

textFormat. It supports two meta tags: %cvl-text% and %cvl-children-counter%. First tag will be replaced with item's text. Second one will be replaced with children counter, obvious, heh? By default this parameter has value %cvl-text% and item will have only text. But you can add any text that you want, for example: 'Item %cvl-text% has %cvl-children-counter% item(s)'.
childrenCounterFormat. This string defines a format for children counter. This parameter supports only one meta tag %cvl-count% which will be replaced with number of children of current item. For example, you can set this format '[%cvl-count%]' and textFormat to '%cvl-children-counter% %cvl-text%' and you will get items like '[3] First Item', '[7] Another Item' or '[5] Third Item'.
emptyChildrenCounter. Flag is used when item has no children. If this parameter is true then %cvl-children-counter% tag will be rendered when children number is 0. Otherwise it will be removed. For example, suppose textFormat is '%cvl-children-counter% %cvl-text%' and childrenCounterFormat is '[ %cvl-count% ]' and our item has no children. So, if emptyChildrenCounter is true then you will get '[ 0 ] Item Text' and 'Item Text' otherwise.
You can change parameters of each column item and of whole column separately. Imagine you have created control and have variable 'cl':

// Single item
cl.getColumnList().getColumn(0).getItem(2).setChildrenCounterFormat('{%cvl-count%}');
cl.getColumnList().getColumn(1).getItem(1).setChildrenCounterFormat('=%cvl-count%=');

// Whole columns
cl.getColumnList().getColumn(0).setChildrenCounterFormat('[%cvl-count%]');
cl.getColumnList().getColumn(0).setTextFormat('%cvl-children-counter% %cvl-text%');
cl.getColumnList().getColumn(0).setEmptyChildrenCounter(true);
cl.getColumnList().getColumn(1).setChildrenCounterFormat('{ %cvl-count% }');
cl.getColumnList().getColumn(1).setTextFormat('%cvl-text% %cvl-children-counter%');
And, of course, you can change tags! It stored in global object jCVL_ColumnItemTags and by default it looks like:

var jCVL_ColumnItemTags = {
    'text':             '%cvl-text%',
    'childrenCounter':  '%cvl-children-counter%',
    'childrenNumber':   '%cvl-count%'
};
So, you can simply do the following:

jCVL_ColumnItemTags.text = '$my-text-tag$';
jCVL_ColumnItemTags.childrenCounter = '$my-kids-counter$';
columnItem.setTextFormat('Item: $my-text-tag$ $my-kids-counter$');
// Items will be update automatically
Note. Take in mind that global tag's object is used by all jColumnListView controls, so your changes will affect all control instances and probably you will need to update formats everywhere.

Version 0.4.2
Implemented visual indicator for items with children. There are two parameters childIndicator and childIndicatorTextFormat. First parameter specifies to show or not children indicator. Second parameter defines a format for text in indicator. If second parameter is null text will not be rendered. This format supports only one tag %cvl-count%. See new screenshots below.
Added two CSS classes: cvl-column-item-indicator and cvl-column-item-indicator-selected. Use these classes to customize your indicator. Structure of text label with indicator element:
<span class="cvl-column-item-label">
    <span>Motherboards</span>
    <div class="cvl-column-item-indicator"></div>
</span>
Functions for operate on text formats (0.4.1) and children indicator (0.4.2) implemented for Item, Column and ColumnList. You can easily change look of control:
// List
cl.getColumnList().setChildIndicator(false)
// Column
cl.getColumnList().getColumn(0).setChildIndicator(true)
cl.getColumnList().getColumn(0).setChildIndicatorTextFormat('A')
// Item
cl.getColumnList().getColumn(0).getItem(2).setChildIndicatorTextFormat('(%cvl-count%)')
cl.getColumnList().getColumn(0).getItem(3).setChildIndicatorTextFormat('[%cvl-count%]')
Version 0.5.0
Basic AJAX support. Added ajaxSource option to configure AJAX request and setFromURL() to load data in list by URL. ajaxSource objects contains of following parameters:
url - URL to get data. By default is null. See notes below.
method - 'GET' or 'POST'.
dataType - Type of data from URL. See notes below.
waiterClass - class name for waiter element (e.g., load spinner). See CSS file for example.
onSuccess - callback to be called when data retrieved successfuly and after this data setted to list.
onFailure - callback to be called when some errors occured during request.
Both of callbacks has the same signature:
  function onSuccess(reqObj, respStatus, respData)
  function onFailure(reqObj, respStatus, errObj)
where reqObj is an XMLHttpRequest, respStatus is a text status, respData is a data object with required type, errObj is an error object. (See jQuery.ajax() for more details about dataType and callbacks).
New setFromURL() function of jCVL_ColumnListView object. When you create a list view object you can leave ajaxSource.url field empty (null). Control will be constructed without any data, but required parameters for AJAX will be ready for use (method, callbacks, etc). Later you just call to setFromURL(data_url) and your control will be filled with data.
Notes. Now only whole list update is supported. Request must returns simple HTML fragment with <UL> list like example below. Control will parse this list and set data. Separate URLs and URL parameters for columns and items, different data types (e.g. JSON, text, etc) will be supported in future versions of 0.5.x branch.
