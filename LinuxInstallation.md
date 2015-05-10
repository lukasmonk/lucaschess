Linux installation from sources
-------------------------------

1) Download github distribution

2) Install dependencies:
    - sudo pip install psutil
    - sudo pip install pygal
    - sudo apt-get install python-pyaudio
    - sudo apt-get install sphinx3
    - sudo apt-get install swig
    - sudo pip install pocketsphinx
    - sudo pip install pygal
    - sudo pip install chardet

3) To fix the issue in Linux-64bits, OSError: EnginesLinux/winglet/winglet.so: wrong ELF class: ELFCLASS32
    - modify EnginesLinux/winglet/srclinux/makefile:

        - $(LINK_TARGET) : $(OBJS)
        - \- g++ -shared -O3 -o $@ $^
        - \+ g++ -shared -O3 -m64 -fPIC -o $@ $^


        - %.o : %.cpp
        - \- g++ -g -O3 -o $@ -c $<
        - \+ g++ -g -O3 -m64 -fPIC -o $@ -c $<


    - make clean
    - make all

4) chmod -R 777 *  at top-level project directory, (to ensure all engines have exec-permissions)


Tested in Ubuntu 14.04


Robert Gamble
