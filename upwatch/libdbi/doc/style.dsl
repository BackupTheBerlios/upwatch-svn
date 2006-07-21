<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY html-ss
  PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA dsssl>
<!ENTITY print-ss
  PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA dsssl>
]>

<style-sheet>
<style-specification id="print" use="print-stylesheet">
<style-specification-body> 

(define %paper-type% "A4")
(declare-characteristic heading-level
  "UNREGISTERED::James Clark//Characteristic::heading-level" 2)
(define %section-autolabel% #t)

</style-specification-body>
</style-specification>
<style-specification id="html" use="html-stylesheet">
<style-specification-body> 

;; customize the html stylesheet
;;(define %section-autolabel% #t)

</style-specification-body>
</style-specification>
<external-specification id="print-stylesheet" document="print-ss">
<external-specification id="html-stylesheet" document="html-ss">
</style-sheet>


