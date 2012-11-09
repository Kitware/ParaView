Do not edit the book html pages or images manually.
The book content is automatically generated.

Instead make your changes online at:
  http://paraview.org/Wiki/ParaView/Users_Guide/Table_Of_Contents

Download each page that you want to export in ODT format inside a directory ${ODT_DIR}
Then
$ git clone git://kwsource.kitwarein.com/miscellaneousprojectsuda/odt_to_html_converter.git odt2html
$ cd odt2html
$ ./convert.sh ${ODT_DIR} ${paraview_version}

Then replace the content of the Book directory by the one generated inside 
> odt2html/Doc/${paraview_version}/Book
