
(define 
  (fix-pv inFileBase outFileBase)
    (let* 
      (
      ; Strokes
      (tTobStroke (vector 666.5 1233.5 666.5 1939.5))
      (tTobSrc (vector 667.0 1233.0))
      (lTorStroke (vector 98.5 1534.5 1726.5 1534.5))
      (lTorSrc (vector 98.0 1533.0))
      ; files
      (inFile inFileBase)
      (outFile outFileBase)
      (image (car (gimp-file-load RUN-NONINTERACTIVE inFile inFile)))
      (dable (car (gimp-image-get-active-layer image)))
      )

      ; apply top to bottom stroke
      (gimp-context-set-brush "Circle (01)")
      (gimp-clone dable dable IMAGE-CLONE (vector-ref tTobSrc 0) (vector-ref tTobSrc 1) 4 tTobStroke)
      ; apply left to right stroke
      (gimp-context-set-brush "Circle (01)")
      (gimp-clone dable dable IMAGE-CLONE (vector-ref lTorSrc 0) (vector-ref lTorSrc 1) 4 lTorStroke)

      (gimp-file-save RUN-NONINTERACTIVE image dable outFile outFile)
      (gimp-image-delete image)

      (display (string-append inFile "->" outFile))

   )
)

