    .global _asset_Font
    .global _asset_FontSize
    .section __TEXT,__const
_asset_Font:
    .incbin "../assets/Res/Montserrat.ttf"
1:
_asset_FontSize:
    .int 1b - _asset_Font

    .global _asset_Languages
    .global _asset_LanguagesSize
    .section __TEXT,__const
_asset_Languages:
    .incbin "../assets/Res/Languages.csv"
1:
_asset_LanguagesSize:
    .int 1b - _asset_Languages

    .global _asset_Galaxy
    .global _asset_GalaxySize
    .section __TEXT,__const
_asset_Galaxy:
    .incbin "../assets/Res/Galaxy.png"
1:
_asset_GalaxySize:
    .int 1b - _asset_Galaxy

    .global _asset_Tools
    .global _asset_ToolsSize
    .section __TEXT,__const
_asset_Tools:
    .incbin "../assets/Res/Tools.png"
1:
_asset_ToolsSize:
    .int 1b - _asset_Tools

    .global _asset_Setup
    .global _asset_SetupSize
    .section __TEXT,__const
_asset_Setup:
    .incbin "../assets/Res/Setup.png"
1:
_asset_SetupSize:
    .int 1b - _asset_Setup

    .global _asset_TextIcons
    .global _asset_TextIconsSize
    .section __TEXT,__const
_asset_TextIcons:
    .incbin "../assets/Res/TextIcons.png"
1:
_asset_TextIconsSize:
    .int 1b - _asset_TextIcons

    .global _asset_ActionIcons
    .global _asset_ActionIconsSize
    .section __TEXT,__const
_asset_ActionIcons:
    .incbin "../assets/Res/ActionIcons.png"
1:
_asset_ActionIconsSize:
    .int 1b - _asset_ActionIcons

    .global _asset_MenuIcons
    .global _asset_MenuIconsSize
    .section __TEXT,__const
_asset_MenuIcons:
    .incbin "../assets/Res/MenuIcons.png"
1:
_asset_MenuIconsSize:
    .int 1b - _asset_MenuIcons
