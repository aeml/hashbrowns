from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class HashbrownsConan(ConanFile):
    name = "hashbrowns"
    version = "1.0.0"
    license = "GPL-3.0-only"
    url = "https://github.com/aeml/hashbrowns"
    description = "C++ data structure benchmarking suite"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*", "docs/*", "tests/*"
    generators = "CMakeToolchain", "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def build(self):
        cm = CMake(self)
        cm.configure()
        cm.build()

    def package(self):
        cm = CMake(self)
        cm.install()

    def package_info(self):
        # Expose targets for consumers
        self.cpp_info.libs = ["hashbrowns_benchmark", "hashbrowns_structures", "hashbrowns_core"]
