# Starlight
**Starlight** is a toolkit for modding The Legend of Zelda: Tears of the Kingdom. It includes:

-   **AINB Editor:** This visual node editor allows you to effortlessly modify the game's logic data, including AI behaviors and sequences.
-   **Map Editor:** Edit actors directly within a scene using the map editor.
-   **Actor Editor:** Simplify map editing by creating and customizing actors with this built-in tool. It's designed for basic static map actors, not for comprehensive actor creation.

## Installation

Download and unzip the `Starlight.vX.X.X.zip` file from the [latest release](https://github.com/MrMystery-Official/Starlight-Dev/releases/latest).

### Prerequisites

- RomFS dump of [Tears of the Kingdom](https://zelda.nintendo.com/tears-of-the-kingdom/)
- Combined model decompress folder

### How to get combined model folder

Firstly, follow instructions on [mc_decompressor](https://gamebanana.com/tools/13236) mod page.

Once you have `mc_output0` and `mc_output1` folders, copy the contents of both into one singular folder, and store it in a safe place.

This will be the folder used for setup.

## Setup

Open `MapEditor.exe`. Windows may give you a notice about unwanted application, you can bypass this by opening properties of the file and checking Unblock.

You will be greeted with a window like the following:

![Screenshot displaying the Starlight interface. The editor shows panels for the outliner, a blank map view, properties, and a console. The Map View tab is active.](https://github.com/user-attachments/assets/551238ee-dab8-48a7-be38-d403285a3c12)

Firstly, you need to setup file paths. Click **Settings** in the menu bar to open the settings dialog.

![Settings dialog. Options are displayed for Paths, specifically RomFS Path and Model Path. They are red and needing to be set.](https://github.com/user-attachments/assets/d066ddca-f4b0-4580-92d9-4f3fab6433f2)

Fill in the **RomFS Path** with the absolute path of your dump folder, and the **Model Path** with the absolute path of your combined model folder.

When set successfully, they will turn green like the image following.

![Settings dialog. The options displayed for Paths are now green and filled in with text.](https://github.com/user-attachments/assets/fe3f0f34-d0d4-40f5-b222-2eb4f7db0e77)

Click **Close** to save your settings. The tool freezing for a second is normal.

Now, relaunch Starlight to load all the necessary assets.

You're done and ready to use Starlight. The Starlight Discord Server or the [Zelda TotK Modding Hub](https://discord.gg/nEFVf8A) are good resources to look to get started using Starlight.

## Support

-   **For bug reports or issues:** Please contact me via Discord(mrmystery0778) or open a GitHub issue.
-   **For general support:** Use the support channel in the Starlight Discord server: https://discord.gg/ug2pkkNq
-   **Using Starlight in your mods:** If you use Starlight extensively in your mod creation, please credit this tool in your acknowledgments.
