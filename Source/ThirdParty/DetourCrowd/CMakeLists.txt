# Copyright (c) 2008-2023 the Urho3D project
# License: MIT

# Define target name
set (TARGET_NAME DetourCrowd)

# Define source files
define_source_files (GLOB_CPP_PATTERNS Source/*.cpp GLOB_H_PATTERNS Include/*.h)

# Define dependency libs
set (INCLUDE_DIRS Include ../Detour/Include)

# Setup target
setup_library ()

# Install headers for building the Urho3D library
install_header_files (DIRECTORY Include/ DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/DetourCrowd FILES_MATCHING PATTERN *.h BUILD_TREE_ONLY)  # Note: the trailing slash is significant
