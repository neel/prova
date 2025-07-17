vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mlpack/ensmallen
    REF 2.21.1
    SHA512 1e86fc28a58694057262a8d036af8080be084c889f7b659b77a08fd4e0957d0f03d8866e47b682a1868b5ac2198cca85c591a334b284096659a123196de95a66
)

vcpkg_replace_string(
    "${SOURCE_PATH}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.3.2)"
    "cmake_minimum_required(VERSION 3.5)"
)

vcpkg_replace_string(
    "${SOURCE_PATH}/CMakeLists.txt"
    "Armadillo::Armadillo"
    "armadillo"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    PREFER_NINJA
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ensmallen CONFIG_PATH lib/cmake/ensmallen)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(INSTALL "${SOURCE_PATH}/LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
