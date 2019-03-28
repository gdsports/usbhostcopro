#!/bin/bash
IDEVER="1.8.9"
WORKDIR="/tmp/autobuild_$$"
mkdir -p ${WORKDIR}
# Install Ardino IDE in work directory
if [ -f ~/Downloads/arduino-${IDEVER}-linux64.tar.xz ]
then
    tar xf ~/Downloads/arduino-${IDEVER}-linux64.tar.xz -C ${WORKDIR}
else
    wget -O arduino.tar.xz https://downloads.arduino.cc/arduino-${IDEVER}-linux64.tar.xz
    tar xf arduino.tar.xz -C ${WORKDIR}
    rm arduino.tar.xz
fi
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
# Install board package
arduino --pref "compiler.warning_level=default" --save-prefs
arduino --install-boards "arduino:samd"
arduino --pref "boardsmanager.additional.urls=https://adafruit.github.io/arduino-board-index/package_adafruit_index.json" --save-prefs
arduino --install-boards "adafruit:samd"
BOARD="arduino:samd:arduino_zero_edbg"
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
# Install MIDI library for USBH_MIDI examples
arduino --install-library "MIDI Library"
arduino --install-library "Adafruit DotStar"
cd $LIBDIR
# Install USB host library for SAMD
if [ -d ~/Sync/USB_Host_Library_SAMD ]
then
    ln -s ~/Sync/USB_Host_Library_SAMD .
else
    git clone https://github.com/gdsports/USB_Host_Library_SAMD
fi
# Build all examples for Adafruit Trinket M0
cd ${IDEDIR}/portable/sketchbook
if [ -d ~/Sync/usbhostcopro ]
then
	ln -s ~/Sync/usbhostcopro .
else
    git clone https://github.com/gdsports/usbhostcopro
fi
cd usbhostcopro
BOARD="adafruit:samd:adafruit_trinket_m0"
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
find . -name '*.ino' -print0 | xargs -0 -n 1 $CC >/tmp/m0_$$.txt 2>&1
# Export binary
# find . -name '*.bin'|gawk '{uf2=$0; sub(/.*\//, "", uf2); sub(/bin$/, "uf2", uf2); printf("uf2conv.py -c %s -o firmware/%s\n", $0, uf2)}'
