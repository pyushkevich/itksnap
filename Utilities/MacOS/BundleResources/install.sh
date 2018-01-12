#!/bin/bash

# Color formatting
RED='\033[0;31m'
YELLOW='\033[0;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

function emph()
{
  echo "\033[0;4m${@}\033[0m"
}

function prompt_yesno()
{
  indent=${1?}
  message=${2?}
  defresp=${3?}
  while true; do
    printf "${YELLOW}${indent}${message}${NC}"
    read  -p "" response
    printf "$NC"
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]; then
      yesno=1
      break
    elif [[ "$response" =~ ^([nN][oO]|[nN])+$ ]]; then
      yesno=0
      break
    elif [[ ! $response ]]; then
      yesno=$defresp
      break
    else
      echo "${indent}Please type 'y' or 'n'"
    fi
  done
}

function install_binary()
{
  cmd=${1?}
  name=${2?}
  src=${3?}
  trg=${4?}

  echo "Installing ${name} '${cmd}' to ${trg}"
  if [[ -f $trg/$cmd || -L $trg/$cmd ]]; then
    if [[ $trg/$cmd -nt $src/$cmd ]]; then
      echo "  A newer version of '$cmd' is already installed in $trg."
      prompt_yesno "    " "Do you want to override it [y/N]" 0
      if [[ $yesno -eq 0 ]]; then
        return
      fi
    fi
  fi

  ln -sf $src/$cmd $trg/$cmd
}

# Determine the location of the script itself
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do 
  # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  # if $SOURCE was a relative symlink, we need to resolve it relative 
  # to the path where the symlink file was located
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" 
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

# The location of the bin files
BINDIR=$(dirname $DIR)/bin

# The destination directory
DESTDIR=${1:-"/usr/local/bin"}

# Say what we are doing
echo -e "${YELLOW}${BOLD}ITK-SNAP Command Line Tool Installer${NC}"
echo -e "This script will install links to command-line programs included with \"
        "ITK-SNAP to $(emph $DESTDIR)"
prompt_yesno "" "Do you wish to continue? [Y/n] " 1

# Handle the launcher
if [[ ! -f $BINDIR/itksnap ]]; then
  echo -e "${RED}ERROR:${NC} missing ITK-SNAP launcher $(emph $BINDIR/itksnap)"
  exit -1
fi

# Copy the launcher to /usr/local/bin
echo -e "Installing the ITK-SNAP command-line launcher $(emph itksnap) to $(emph $DESTDIR)"
cp -a $BINDIR/itksnap $DESTDIR

# Install the workspace tool
install_binary itksnap-wt "ITK-SNAP workspace tool" $BINDIR $DESTDIR

# Prompt whether to install Convert3D
echo "ITK-SNAP is packaged with Convert3D, a command-line tool"
echo "for image filtering, image arithmetic, and many useful"
echo "image processing commands (http://itksnap.org/c3d)"
prompt_yesno "Install links to Convert3D commands in $DESTDIR? [Y/n] "

if [[ $yesno -eq 1 ]]; then
  install_binary c3d "Convert3D tool" $BINDIR $DESTDIR
  install_binary c2d "Convert3D tool for 2D images" $BINDIR $DESTDIR
  install_binary c4d "Convert3D tool for 4D images" $BINDIR $DESTDIR
  install_binary c3d_affine_tool "Convert3D tool for affine transforms" $BINDIR $DESTDIR
fi
