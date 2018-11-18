#!/bin/sh



sudo apt-get -y install python-pip
sudo apt-get -y install python-dev
sudo apt-get -y install python-qt4
sudo apt-get -y install python-pyaudio
sudo apt-get -y install python-setuptools
sudo apt-get  -y install pychess
sudo -H pip  install python-chess
pip install --upgrade pip
sudo -H pip install psutil
sudo -H pip install pygal
sudo -H pip install chardet
sudo -H pip install Pillow
sudo -H pip install PhotoHash
sudo -H pip install Cython
sudo -H pip install scandir

# For reference, after building turn on the executable bit for the
# following files:
#
# file `find Linux64/ ` | grep -i elf | cut -f 1 -d : | grep -v "\.o"
