# Type Safe Message Parser

TODO 

## Build Instructions

All defined presets have the following scheme:

| Preset stage  | scheme                    | description                                                              |
|---------------|---------------------------|--------------------------------------------------------------------------|
| build         | **build-**\<PRESET_NAME\> | This stage is used for compiling the project                             |
| configuration | \<PRESET_NAME\>           | This stage is used for configure the project with defined compiler setup |
| test          | **test-**\<PRESET_NAME\>  | This stage is used to run all test registered for ctest                  |

### Configure your build

To configure the project and write makefiles, you could use `cmake` with a bunch of command line options.
The easier option is to run cmake interactively:

#### **Configure via cmake preset**:

Check the preset which can be applied to your build system by typing:

    cmake --list-presets

The output looks like this:

    Available configure presets:

    "unixlike-gcc-14-debug"       - GCC 14 Debug
    "unixlike-gcc-14-release"     - GCC 14 Release
    "unixlike-clang-17-debug"     - Clang 17 Debug
    "unixlike-clang-17-release"   - Clang 17 Release
    "unixlike-clang-18-debug"     - Clang 18 Debug
    "unixlike-clang-18-release"   - Clang 18 Release
    "unixlike-clang-19-debug"     - Clang 19 Debug
    "unixlike-clang-19-release"   - Clang 19 Release

Choose a configuration which is suitable and use following command for example.

    cmake --preset unixlike-clang-17-debug

### Build
Once you have selected all the options you would like to use, you can build the
project (all targets):

    cmake --preset <PRESET_NAME>

For example:
    
    cmake --preset build-unixlike-clang-17-debug

### Test
Run all test using preset and ctest:

    cmake --preset <PRESET_NAME>

For example:

    cmake --preset test-unixlike-clang-17-debug

## Testing

- See [Catch2 tutorial](https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md)
