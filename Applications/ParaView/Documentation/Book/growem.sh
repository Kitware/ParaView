#script to resize openoffice sized images to something readable
foreach i (*.html)
  python imagesizer.py $i > bigger/$i
end
mv bigger/* .
