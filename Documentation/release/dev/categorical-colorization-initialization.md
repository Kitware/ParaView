# Annotations automatically initialized when categorical colorization is enabled

ParaView will now attempt to automatically initialize colors and annotations when the 'Interpret Values as Categories' option is enabled for a color map. It will determine the categories from the currently selected source in the Pipeline Browser. If unsuccessful, ParaView will report an error.
