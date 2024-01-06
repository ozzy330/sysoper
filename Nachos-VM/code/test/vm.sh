#! /bin/bash

folder="vm"

cd "$(dirname "${BASH_SOURCE}")"
cd ../$folder/ && make depend && make  && cd ../test
echo -e "\n\nIniciando Shell en NachOS:\n"
../$folder/nachos ${@}
