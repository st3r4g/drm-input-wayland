project('compositor', 'c', default_options : 'c_std=c11')
add_project_arguments('-Wall', '-Wextra', '-Wno-unused-parameter', language : 'c')
add_project_link_arguments('-lm', language : 'c')

b = 'backend/'
p = 'protocols/'
u = 'util/'
w = 'wl/'
x = 'xdg/'

src = [b+'egl.c', b+'input.c', b+'screen.c']
src += p+'xdg-shell-unstable-v6-code.c'
src += [u+'algebra.c', u+'log.c']
src += [w+'compositor.c', w+'data_device_manager.c', w+'keyboard.c', w+'output.c', w+'region.c', w+'seat.c', w+'surface.c']
src += x+'xdg_shell.c'
src += ['main.c', 'renderer.c']

inc = include_directories('include')

dep = [dependency('egl')]
dep += dependency('gbm')
dep += dependency('glesv2')
dep += dependency('libdrm')
dep += dependency('libudev')
dep += dependency('wayland-server')
dep += dependency('xkbcommon')

executable('compositor', src, dependencies : dep, include_directories : inc)
