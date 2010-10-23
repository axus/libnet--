# Build using Makefile.MinGW
#
# To build project:
#       build-msys.sh
# To clean project:
#       build-msys.sh clean

make $@
cd test_client
make $@
cd ..
cd test_http
make $@
cd ..