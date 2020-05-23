#!/bin/bash
#===============================================================================
#
#          FILE:  install-script.sh
#
#         USAGE:  ./install-script.sh
#
#   DESCRIPTION:  Script used for installation of the iMonitor application
#
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR:   (),
#       COMPANY:
#       VERSION:  1.0
#       CREATED:  05/05/2020 04:28:01 PM IST
#      REVISION:  ---
#===============================================================================

tar -xzf htop2.tar.gz -C imonitor/
cd imonitor
./autogen.sh && ./configure && make
