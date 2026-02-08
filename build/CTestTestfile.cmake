# CMake generated Testfile for 
# Source directory: /home/duong/MediaPlayerApp
# Build directory: /home/duong/MediaPlayerApp/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(RunTests "/home/duong/MediaPlayerApp/build/RunTests")
set_tests_properties(RunTests PROPERTIES  _BACKTRACE_TRIPLES "/home/duong/MediaPlayerApp/CMakeLists.txt;151;add_test;/home/duong/MediaPlayerApp/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
