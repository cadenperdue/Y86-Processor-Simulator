#/bin/bash

export std="c++11"
if [ "$1" != "" ]; then
    scl enable devtoolset-3 '/bin/bash --rcfile <(echo "make '$1'; exit")'
else
    scl enable devtoolset-3 '/bin/bash --rcfile <(echo "make; exit")'
fi
