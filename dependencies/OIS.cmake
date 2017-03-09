# Custom OIS build file.
project(OIS)

set(OIS_COMMON_SOURCES
  OIS/src/OISObject.cpp
  OIS/src/OISException.cpp
  OIS/src/OISEffect.cpp
  OIS/src/OISJoyStick.cpp
  OIS/src/OISInputManager.cpp
  OIS/src/OISForceFeedback.cpp
  OIS/src/OISKeyboard.cpp)

set(OIS_WIN_SOURCES
  OIS/src/win32/Win32InputManager.cpp
  OIS/src/win32/Win32ForceFeedback.cpp
  OIS/src/win32/Win32Mouse.cpp
  OIS/src/win32/Win32KeyBoard.cpp
  OIS/src/win32/Win32JoyStick.cpp)

set(OIS_MAC_SOURCES
  OIS/src/mac/MacJoyStick.cpp
  OIS/src/mac/MacHelpers.cpp
  OIS/src/mac/MacHIDManager.cpp
  OIS/src/mac/MacMouse.cpp
  OIS/src/mac/MacKeyboard.cpp
  OIS/src/mac/MacInputManager.cpp)

set(OIS_LINUX_SOURCES
  OIS/src/linux/EventHelpers.cpp
  OIS/src/linux/LinuxMouse.cpp
  OIS/src/linux/LinuxJoyStickEvents.cpp
  OIS/src/linux/LinuxInputManager.cpp
  OIS/src/linux/LinuxForceFeedback.cpp
  OIS/src/linux/LinuxKeyboard.cpp)

if(MSVC)
  set(OIS_SOURCES ${OIS_COMMON_SOURCES} ${OIS_WIN_SOURCES})
  set(OIS_DEFINITIONS OIS_WIN32_PLATFORM)
elseif(APPLE)
  set(OIS_SOURCES ${OIS_COMMON_SOURCES} ${OIS_MAC_SOURCES})
  set(OIS_DEFINITIONS OIS_APPLE_PLATFORM)
else()
  set(OIS_SOURCES ${OIS_COMMON_SOURCES} ${OIS_LINUX_SOURCES})
  set(OIS_DEFINITIONS OIS_LINUX_PLATFORM)
endif()

add_library(ois STATIC ${OIS_SOURCES})
target_include_directories(ois SYSTEM PUBLIC OIS/includes)
target_compile_definitions(ois PUBLIC ${OIS_DEFINITIONS})
