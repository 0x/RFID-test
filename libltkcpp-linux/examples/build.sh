#!/bin/bash -eu

echo ================================================================================
echo Building Examples for x86_64
echo ================================================================================

# NOTE : to build the examples, make sure you have the libxml2-dev package installed

SCRIPT_PATH=$(dirname $(realpath $0))
OUTPUT_PATH=${SCRIPT_PATH}/output
LTK_INSTALL_PATH=${SCRIPT_PATH}/..

rm -rf ${OUTPUT_PATH}

cmake -G"Unix Makefiles" ${SCRIPT_PATH} -B"${OUTPUT_PATH}" \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DLTK_INCLUDE_DIR=${LTK_INSTALL_PATH} \
    -DLTK_LIB=${LTK_INSTALL_PATH}/libltkcpp_x86_64.a \
    -DLTKIMPINJ_LIB=${LTK_INSTALL_PATH}/libltkcppimpinj_x86_64.a \
    -DOPENSSL_LIBS=libssl.so\;libcrypto.so\
    -DLIBXML2_LIB=xml2

cmake --build "${OUTPUT_PATH}"
