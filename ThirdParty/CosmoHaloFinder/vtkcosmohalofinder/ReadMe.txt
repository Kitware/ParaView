Files copied from cosmotools with the following shell code:

cosmoToolsDir=/path/to/cosmotools

cp $cosmoToolsDir/algorithms/halofinder/*.{cxx,h} .
cp $cosmoToolsDir/common/CosmoTools*.h .

for file in `ls *.{cxx,h}`; do
  expand -t 8 $file | sed -e :a -e '/./,$!d;/^\n*$/{$d;N;};/\n$/ba' - > ${file}.bak
  mv ${file}.bak $file
done
