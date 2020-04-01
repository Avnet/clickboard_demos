cmake \
-G "Ninja" \
-DCMAKE_TOOLCHAIN_FILE="/opt/azurespheresdk/CMakeFiles/AzureSphereToolchain.cmake" \
-DAZURE_SPHERE_TARGET_API_SET="4" \
-DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY="../Hardware/mt3620_rdb/" \
-DAZURE_SPHERE_TARGET_HARDWARE_DEFINITION="sample_hardware.json" \
--no-warn-unused-cli \
-DCMAKE_BUILD_TYPE="Debug" \
-DCMAKE_MAKE_PROGRAM="ninja" \
..
