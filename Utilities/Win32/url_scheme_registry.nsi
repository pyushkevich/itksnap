; NSIS script to register itksnap:// URL scheme on Windows
; This script is included in the main NSIS installer

; Register URL scheme in registry
WriteRegStr HKCR "itksnap" "" "URL:ITK-SNAP Protocol"
WriteRegStr HKCR "itksnap" "URL Protocol" ""
WriteRegStr HKCR "itksnap\DefaultIcon" "" "$INSTDIR\bin\ITK-SNAP.exe,0"
WriteRegStr HKCR "itksnap\shell\open\command" "" '"$INSTDIR\bin\ITK-SNAP.exe" "%1"'
