#!/bin/bash
# Make a development build of ITK-SNAP the handler for itksnap-* links on macOS,
# so that clicking a link launches the build under the debugger.
#
#   debug_url_handler.sh <build-dir>   wrap <build-dir>/ITK-SNAP in
#                                      <build-dir>/ITK-SNAP-dbg.app and make it
#                                      the handler
#   debug_url_handler.sh --restore     hand the links back to the installed app
#   debug_url_handler.sh --status      show which app opens each kind of link
#
# The wrapper app links to the build's executable, so rebuilds are picked up.
# Debug a link with: lldb -n ITK-SNAP -w   (then click the link)
set -e

SRC_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PLIST_SRC="$SRC_DIR/Utilities/MacOS/BundleResources/Info.plist"
LSREG=/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister
DEBUG_ID=org.itksnap.debug
RELEASE_ID=org.itksnap

# The schemes listed in the bundle's Info.plist
SCHEMES=$(/usr/libexec/PlistBuddy -c "Print :CFBundleURLTypes" "$PLIST_SRC" \
  | grep -o 'itksnap-[a-z]*' | sort -u)

set_handler()
{
  local swift_list
  swift_list=$(printf '"%s",' $SCHEMES)
  swift -e "import Foundation
import CoreServices
for s in [${swift_list%,}] {
  let rc = LSSetDefaultHandlerForURLScheme(s as CFString, \"$1\" as CFString)
  print(\"\\(s) -> $1\", rc == 0 ? \"\" : \"(error \\(rc))\")
}"
}

show_status()
{
  local swift_list
  swift_list=$(printf '"%s",' $SCHEMES)
  swift -e "import AppKit
for s in [${swift_list%,}] {
  let app = NSWorkspace.shared.urlForApplication(toOpen: URL(string: s + \"://x/y\")!)
  print(s, \"->\", app?.path ?? \"none\")
}"
}

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <build-dir> | --restore | --status" >&2
  exit 1
fi

if [[ $1 == --status ]]; then
  show_status
  exit 0
fi

if [[ $1 == --restore ]]; then
  set_handler $RELEASE_ID
  exit 0
fi

BUILD_DIR="$(cd "$1" && pwd)"
EXE="$BUILD_DIR/ITK-SNAP"
APP="$BUILD_DIR/ITK-SNAP-dbg.app"
if [[ ! -x $EXE ]]; then
  echo "No ITK-SNAP executable in $BUILD_DIR" >&2
  exit 1
fi

mkdir -p "$APP/Contents/MacOS"
cp "$PLIST_SRC" "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy \
  -c "Set :CFBundleIdentifier $DEBUG_ID" \
  -c "Set :CFBundleExecutable ITK-SNAP" \
  -c "Set :CFBundleName ITK-SNAP (debug)" \
  "$APP/Contents/Info.plist"
ln -sf "$EXE" "$APP/Contents/MacOS/ITK-SNAP"

"$LSREG" -f "$APP"
set_handler $DEBUG_ID
