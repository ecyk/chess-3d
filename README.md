# chess-3d

A simple 3d chess game.

- U to undo last move
- R to restart game
- C to reset camera angle
- F to toggle fullscreen

![alt text](/images/screenshot.png)

Board and piece models are taken from [here](https://polyhaven.com/a/chess_set) (CC0).

## Building

```sh
git clone https://github.com/ecyk/chess-3d.git &&
cd chess-3d &&
mkdir build &&
cd build &&
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .. &&
make
```

GLFW and GLM libraries must be available on your system. They can be easily installed using vcpkg. Then, add the
following line to your `cmake` command:

`-DCMAKE_TOOLCHAIN_FILE=$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake`

## Resources

- [Learn OpenGL](https://learnopengl.com)
- [Chess Programming Wiki](https://www.chessprogramming.org)
