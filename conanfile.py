from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class HelloConan(ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'CMakeDeps', 'CMakeToolchain'
    default_options = {'spdlog/*:header_only': True}

    def requirements(self):
        self.requires('catch2/3.7.0')
        self.requires('spdlog/1.14.1')

    def configure(self):
        cmake = CMakeToolchain(self)
        cmake.user_presets_path = None

    def build(self):
        cmake = CMakeToolchain(self)
        cmake.configure()
        cmake.build()
