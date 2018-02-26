#!/usr/bin/env bash
# execute as an user who is member of the sudoers, i.e. can elevate to root
sudo apt-get install python-pip
# added next line to make sure to have most current pip version
pip install --upgrade pip
sudo apt-get install python-dev
sudo apt-get install python-qt4
sudo apt-get install python-pyaudio
# added install of setuptools, is required for psutils, python-chess and PhotoHash
pip install setuptools
pip install psutil
pip install pygal
pip install chardet
pip install python-chess
pip install Pillow
pip install PhotoHash
pip install Cython

# !! modify path to your installation directory of lucaschess!!
cd /usr/share/lucaschess

# give read rights to group and others
sudo chmod -R 755 *
# give execution right to all executable files, especially chess engines
sudo chmod -R +x *
# give all rights to ./UsrData in order for all users to share settings
sudo chmod 777 UsrData

# build Irina engine etc, refresh existing libirina.so and LCEngine.so
cd LCEngine/irina
./xmk_linux.sh
cd ..
./xcython_linux.sh