    .global asset_Font
    .global asset_FontSize
    .section .rodata
asset_Font:
    .incbin "../assets/Res/Montserrat.ttf"
1:
asset_FontSize:
    .int 1b - asset_Font

    .global asset_Languages
    .global asset_LanguagesSize
    .section .rodata
asset_Languages:
    .incbin "../assets/Res/Languages.csv"
1:
asset_LanguagesSize:
    .int 1b - asset_Languages

    .global asset_Galaxy
    .global asset_GalaxySize
    .section .rodata
asset_Galaxy:
    .incbin "../assets/Res/Galaxy.png"
1:
asset_GalaxySize:
    .int 1b - asset_Galaxy

    .global asset_Tools
    .global asset_ToolsSize
    .section .rodata
asset_Tools:
    .incbin "../assets/Res/Tools.png"
1:
asset_ToolsSize:
    .int 1b - asset_Tools

    .global asset_Setup
    .global asset_SetupSize
    .section .rodata
asset_Setup:
    .incbin "../assets/Res/Setup.png"
1:
asset_SetupSize:
    .int 1b - asset_Setup

    .global asset_TextIcons
    .global asset_TextIconsSize
    .section .rodata
asset_TextIcons:
    .incbin "../assets/Res/TextIcons.png"
1:
asset_TextIconsSize:
    .int 1b - asset_TextIcons

    .global asset_ActionIcons
    .global asset_ActionIconsSize
    .section .rodata
asset_ActionIcons:
    .incbin "../assets/Res/ActionIcons.png"
1:
asset_ActionIconsSize:
    .int 1b - asset_ActionIcons

    .global asset_MenuIcons
    .global asset_MenuIconsSize
    .section .rodata
asset_MenuIcons:
    .incbin "../assets/Res/MenuIcons.png"
1:
asset_MenuIconsSize:
    .int 1b - asset_MenuIcons
