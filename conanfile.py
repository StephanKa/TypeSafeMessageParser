from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class TypeSafeMessageParserConan(ConanFile):
    name = 'typesafemessageparser'
    version = '0.0.1'
    description = 'Header-only, compile-time type-safe message parser for C++23'
    license = 'MIT'
    package_type = 'header-library'
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'CMakeDeps'
    exports_sources = 'src/include/*'
    default_options = {'spdlog/*:header_only': True}

    def requirements(self):
        self.requires('spdlog/1.17.0')
        self.requires('catch2/3.15.0')

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = 23
        tc.variables["CMAKE_CXX_FLAGS"] = "-Wno-nullability-extension"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy('*.h', src='src/include', dst='include')

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
