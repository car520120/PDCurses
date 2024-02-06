
target('pdcurses')
    set_kind('shared')
    add_files('pdcurses/*.c')
    add_includedirs('.',{public=true})
    add_headerfiles('*.h')
    if is_plat("windows") then
        add_defines('PDC_WIDE', 'PDC_FORCE_UTF8')
        add_links("user32", "advapi32");
        add_files("wincon/*.c")
        add_headerfiles('*.h')
        add_includedirs('wincon',{public=true})
    end