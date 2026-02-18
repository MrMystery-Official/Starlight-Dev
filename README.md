# Starlight

Starlight is a comprehensive, standalone toolkit designed for **The Legend of Zelda: Tears of the Kingdom** modding. It streamlines complex technical workflows, providing creators with the tools necessary to modify world data, AI logic, and physics structures without relying on heavy external dependencies.


## Key Features

* **Standalone Collision Generation:** Generate physics collision data directly within Starlight, eliminating the need for convoluted third-party pipelines or an external emulator.
* **Navmesh Generation:** Create and bake navigation meshes for NPCs and entities to ensure seamless movement within modified environments (single scene only).
* **Integrated Map Editor:**
    * **Terrain Viewing:** Real-time visualization of world terrain.
    * **Terrain Editing:** Modify heightmaps and terrain structures directly within the map environment.
* **AINB Editor:** A dedicated interface for editing the game’s AI logic files.
    * *Note:* The **Auto Layout** feature for node organization is currently in active development. While functional, manual adjustment may still be required for complex graphs.


## Getting Started

Starlight can be obtained either by building the project from the source code or by downloading pre-compiled binaries.
Please make sure to restart Starlight *after* you have specified the paths for the first time.

### Pre-compiled Binaries

For those who prefer to skip the build process or wish to support the ongoing development of Starlight, pre-compiled binaries are available for supporters on [Patreon](https://patreon.com/MrMystery846).

### Building from Source (tested on Windows and MacOS)

To compile Starlight on your local machine, follow these steps:

1.  Clone the repository to your local environment.
2.  Navigate to the project folder.
3.  Run the provided build script:
    ```batch
    build-release.bat
    ```
4.  Once the process completes, the executable will be located in the project's root directory (`/rootDir`).