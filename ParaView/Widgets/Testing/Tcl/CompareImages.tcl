proc IncrementFileName { validImage count } {
    set res ""
    regsub {\.png} $validImage _${count}.png res
    return $res
}

proc CompareImage { view } {
   global argc argv
   set validImageFound 0
   for {set i  1} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-A"} {
         linsert auto_path 0 [lindex $argv [expr $i +1]]
      }
      if {[lindex $argv $i] == "-V"} {
         set validImageFound 1
         set validImage "[lindex $argv [expr $i + 1]]"
      }
   }
   
   set threshold -1
   
   # current directory
   if {$validImageFound == 0} {
      return;
   }
   vtkWindowToImageFilter rt_w2if
   rt_w2if SetInput [$view GetVTKWindow]
   if {$threshold == -1} {
      set threshold 10
   }
      
   # does the valid image exist ?
   if {[file exists ${validImage}] == 0 } {
      if {[catch {set channel [open ${validImage} w]}] == 0 } {
         close $channel
         vtkPNGWriter rt_pngw
         rt_pngw SetFileName $validImage
         rt_pngw SetInput [rt_w2if GetOutput]
         rt_pngw Write
         rt_pngw Delete
      } else {
         puts "Unable to find valid image:${validImage}"
         rt_w2if Delete
         return
      }
   }
   
   vtkPNGReader rt_png
   rt_png SetFileName $validImage
   vtkImageDifference rt_id
   
   rt_id SetInput [rt_w2if GetOutput]
   rt_id SetImage [rt_png GetOutput]
   rt_id Update
   set imageError [rt_id GetThresholdedError]
   set minError [rt_id GetThresholdedError]
   set bestImage $validImage
   rt_w2if Delete 
   
   set count 0
   set errIndex 0
   if {$minError > $threshold} {
      set count 1
      set testFailed 1
      set errIndex -1
      while 1 {
         set newFileName [IncrementFileName $validImage $count]
         if {[catch {set channel [open $newFileName r]}]} {
            break
         }
         close $channel
         rt_png SetFileName $newFileName
         rt_png Update
         rt_id Update
         set altError [rt_id GetThresholdedError]
         if { $altError <= $threshold } { 
            # Test passed with the alternate image
            set errIndex $count
            set testFailed 0
            set minError $altError
            set imageError $altError
            set bestImage $newFileName
            break
         } else {
            # Test failed but is it better than any image we saw so far?
            if { $altError < $minError } {
               set errIndex $count
               set minError $altError
               set imageError $altError
               set bestImage $newFileName
            }
         }
         incr count 1
      }
      
      if { $testFailed } {
         rt_png SetFileName $bestImage
         
         rt_png Update
         rt_id Update
         
         if {[catch {set channel [open $validImage.diff.png w]}] == 0 } {
            close $channel
            
            # write out the difference image in full resolution
            vtkPNGWriter rt_pngw2
            rt_pngw2 SetFileName $validImage.diff.png
            rt_pngw2 SetInput [rt_id GetOutput]
            rt_pngw2 Write 
            rt_pngw2 Delete

            # write out the difference image scaled and gamma adjusted
            # for the dashboard
            set rt_size [[rt_png GetOutput] GetDimensions]
            if { [lindex $rt_size 1] <= 250.0} {
               set rt_magfactor 1.0
            } else {
               set rt_magfactor [expr 250.0 / [lindex $rt_size 1]]
            }
            
            vtkImageResample rt_shrink
            rt_shrink SetInput [rt_id GetOutput]
            rt_shrink InterpolateOn
            rt_shrink SetAxisMagnificationFactor 0 $rt_magfactor 
            rt_shrink SetAxisMagnificationFactor 1 $rt_magfactor 
            
            vtkImageShiftScale rt_gamma
            rt_gamma SetInput [rt_shrink GetOutput]
            rt_gamma SetShift 0
            rt_gamma SetScale 10
            
            vtkJPEGWriter rt_jpegw_dashboard
            rt_jpegw_dashboard SetFileName $validImage.diff.small.jpg
            rt_jpegw_dashboard SetInput [rt_gamma GetOutput]
            rt_jpegw_dashboard SetQuality 85
            rt_jpegw_dashboard Write
            
            # write out the image that was generated
            rt_shrink SetInput [rt_id GetInput]
            rt_jpegw_dashboard SetInput [rt_shrink GetOutput]
            rt_jpegw_dashboard SetFileName $validImage.test.small.jpg
            rt_jpegw_dashboard Write
            
            # write out the valid image that matched
            rt_shrink SetInput [rt_id GetImage]
            rt_jpegw_dashboard SetInput [rt_shrink GetOutput]
            rt_jpegw_dashboard SetFileName $validImage.small.jpg
            rt_jpegw_dashboard Write

            rt_shrink Delete
            rt_jpegw_dashboard Delete
            rt_gamma Delete
         }
         puts "Failed Image Test with error: $imageError"
         
         puts -nonewline "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">"
         puts -nonewline "$imageError"
         puts "</DartMeasurement>"
         
         if { $errIndex <= 0} {
            puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>"
         } else {
            puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">"
            puts -nonewline "$errIndex"
            puts "</DartMeasurement>"
         }
         
         puts -nonewline "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">"
         puts -nonewline "$validImage.test.small.jpg"
         puts "</DartMeasurementFile>"
         
         puts -nonewline "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">"
         puts -nonewline "$validImage.diff.small.jpg"
         puts "</DartMeasurementFile>"
         
         puts -nonewline "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">"
         puts -nonewline "$validImage.small.jpg"
         puts "</DartMeasurementFile>"
         
         rt_id Delete
         rt_png Delete
         return
      }
   }
     
   # output the image error even if a test passed
   puts -nonewline "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">"
   puts -nonewline "$imageError"
   puts "</DartMeasurement>"
   
   if { $errIndex <= 0} {
      puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>"
   } else {
      puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">"
      puts -nonewline "$errIndex"
      puts "</DartMeasurement>"
   }
   rt_id Delete
   rt_png Delete
}
