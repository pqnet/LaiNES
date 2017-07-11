from os import environ

VariantDir('build/src', 'src', duplicate=0)
VariantDir('build/lib', 'lib', duplicate=0)
flags = ['-g', '-march=native', '-std=c++14', '-Wc++14-compat' ]

env = Environment(ENV       = environ,
                  CXX       = 'clang++',
                  CPPFLAGS  = ['-Wno-unused-value'],
                  CXXFLAGS  = flags,
                  LINKFLAGS = flags,
                  CPPPATH   = ['#simpleini', '#lib/include', '#src/include'],
                  LIBS      = ['SDL2', 'SDL2_image', 'SDL2_ttf'])

env.Program('laines', Glob('build/*/*.cpp') + Glob('build/*/*/*.cpp'))
