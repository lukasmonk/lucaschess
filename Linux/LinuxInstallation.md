Linux installation from sources
-------------------------------

1) Download github distribution

2) Install dependencies:
    - sudo apt-get install python-pip
    - sudo apt-get install python-dev
    - sudo apt-get install python-qt4
    - sudo apt-get install python-pyaudio
    - sudo pip install psutil
    - sudo pip install pygal
    - sudo pip install chardet
    - sudo apt-get install python-chess
    - sudo pip install Pillow
    - sudo pip install PhotoHash
    - sudo pip install Cython
    - sudo pip install scandir

    
3) chmod +x -R Engines/Linux64/ - to ensure all engines have exec-permissions

4) 64 bits
    - cd LCEngine/irina
    - ./xmk_linux.sh
    - cd .. 
    - ./xcython_linux.sh


Tested in Mate 14.04


Robert Gamble
