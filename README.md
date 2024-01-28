## Custom implementation of `std::any`

### Notes

- The implementation works with `-no-rtti` compilation flag (take a look in [CMakeLists.txt](./CMakeLists.txt) file). It relies mostly on how the actual `std::any` works.
- Some tests were added, see [src/test.cpp](./src/test.cpp).

### Building
To build and run the tests do the following:

- `git clone https://github.com/dmitrii-artuhov/cub-cpp-ta-assessment.git && cd cub-cpp-ta-assessment`
- `mkdir build && cd build`
- `cmake .. && make`

There will be `tests` target generate in the created build folder, run it: `./tests`.

