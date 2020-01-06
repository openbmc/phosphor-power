gen_intf_hpp = custom_target(
    'server.hpp',
    capture : true,
    input : '../Manager.interface.yaml',
    output : 'server.hpp',
    command : [
        sdbuspp,
        '-r', meson.source_root(),
        'interface',
        'server-header',
        'xyz.openbmc_project.Power.Regulators.Manager'
    ]
)

gen_intf_cpp = custom_target(
    'server.cpp',
    capture : true,
    input : '../Manager.interface.yaml',
    output : 'server.cpp',
    command : [
        sdbuspp,
        '-r', meson.source_root(),
        'interface',
        'server-cpp',
        'xyz.openbmc_project.Power.Regulators.Manager'
    ]
)
