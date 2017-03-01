# See https://github.com/Valloric/YouCompleteMe.
import os

def MakeAbsolute(path):
  this_directory = os.path.dirname(os.path.abspath(__file__))
  return os.path.join(this_directory, path)

flags = [
  # Warning flags.
  '-Wall', '-Wextra', '-Werror', '-pedantic',
  # C++ flags. The '-x' flag is a special one for YCM which indicates the source language.
  '-std=c++11', '-x', 'c++',
  # Library includes.
  '-isystem', MakeAbsolute('dependencies/glew-cmake/include'),
  '-isystem', MakeAbsolute('dependencies/glm'),
  '-isystem', MakeAbsolute('dependencies/SFML/include'),
  '-isystem', MakeAbsolute('build/dependencies/worker_sdk/include'),
  '-isystem', MakeAbsolute('build/generated'),
  # Project includes.
  '-I', MakeAbsolute('.'),
]

def FlagsForFile(filename, **kwargs):
  return {
    'flags': flags,
    'do_cache': True,
  }
