# SkyManager: Skyrim Mod Manager for Nintendo Switch

SkyManager is a simple mod manager for Skyrim on the Nintendo Switch, using the Skyrim.ccc method currently used by the community.

## Requirements

You must have a Nintendo Switch running [Atmosphere CFW](https://github.com/Atmosphere-NX/Atmosphere) and a copy of Skyrim updated to the latest version. 

If you have the base edition of Skyrim (i.e. without the anniversary edition upgrade) SkyManager works out of the box. If you have the Anniversary Edition, please copy config.ini from this
repository to /config/skymanager/config.ini and make the modifications specified in the file **before** running SkyManager.


## Usage

Place your Skyrim NX mods in your Skyrim LayeredFS Data folder (for the base edition, this is /atmosphere/contents/01000A10041EA000/romfs/Data). Please note that mods for other versions of Skyrim require conversion; please see [this guide](https://rentry.co/SkyrimNXModding#i-want-to-convert-a-pc-mod-to-skyrim-nintendo-switch) for instructions.

## Building

You should have DevkitARM and [dirent](https://github.com/tronkko/dirent) in your development environment. A copy of the excellent [inih](https://github.com/benhoyt/inih) is included in the files.



