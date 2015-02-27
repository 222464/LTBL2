Let There Be Light 2
====================

Let There Be Light 2 is a 2D dynamic shadowing/lighting system that uses SFML.    
The original author of Let There Be Light 2 is Eric Laukien. I restructured and refactored parts of the code to make it look more organized, and make better use of CMake.

At the moment, it has only been tested in Ubuntu 14.10 (64-bit) and GCC.    

### Compiling and Installing
Let There Be Light 2 depends on SFML 2.x. Also, to compile and install it, CMake and a C++ compiler is required.

#### Ubuntu
    sudo apt-get install build-essential git cmake libsfml2-dev
    git clone https://github.com/sfballais123/LTBL2.git
    cd LTBL2
    mkdir build && cd build
    cmake ..
    make
    sudo make install # Installs to /usr/local/ by default 

### Contributing
There are still improvements needed in LTBL2. You can contribute to the project to improve it.    
    
Stick with the coding style of the library as possible to avoid confusion between contributors. Fork this repo to your own account and work on it from there to avoid code conflicts in the main account. You may also create a branch in your fork. This is optional but it will be very helpful when working with the code.