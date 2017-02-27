SET BUILD_CONFIG=Release
SET BUILD_GENERATOR=Visual Studio 14 2015 Win64
SET BUILD_DEFINITIONS=
git submodule update --init
spatial build --log_level=debug
