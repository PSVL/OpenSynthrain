# OpenSynthrain
OpenSynthrain is a batch data synthesis tool with a built in preview. 
It was originally written to generate data in support of our paper on adherant raindrop removal:

Adherent Raindrop Removal with Self-Supervised Attention Maps and Spatio-Temporal Generative Adversarial Networks
2019 IEEE/CVF International Conference On Computer Vision Workshop (ICCVW)2019
Stefano Alletto, Casey Carlin, Luca Rigazio, Yasunori Ishii, Sotaro Tsukizawa

## Usage
Data input and output directories are defined in `config.json` located in the working directory. A default config file will be created automatically on first run.

Images should be prepared in the following manner:
### When non-sequential color data will be used
Place images into one folder, e.g. 
  - `\path\to\data\`

In the configuration set 
  - `"input_folder"` to `\path\to\data\` 
  - `'use_sequences`' to `[""]`

### When sequences of color data will be used
Place image sequences in sub folders, e.g. 
  - `\path\to\data\seq1\`
  - `\path\to\data\seq2\`

In the configuration set 
  - `"input_folder"` to `\path\to\data\` 
  - `'use_sequences`' to ["seq1", "seq2"]

Use `use_sequences` to limit your data generation to one or more sequences from a larger data set.

### When using depth and/or flow data with sequences
Using the sequences configuration, further divide data type into subfolders:
  - `\path\to\data\seq1\color`
  - `\path\to\data\seq1\depth`
  - `\path\to\data\seq1\flow`

As noted in the known issues, some raindrop paramers can be adjusted at run time but they will not be saved. 

## Building
OpenSynthrain targets 64-bit OSes only, but it can be made to work on 32-bit if CUDA support is removed.

### Windows
Install the following packages with vcpckg
```
vcpkg install glew:x64-windows libjpeg-turbo:x64-windows libpng:x64-windows sdl2-image:x64-windows sdl2-image[libjpeg-turbo]:x64-windows sdl2:x64-windows zlib:x64-windows
```
You will also need to install the CUDA toolkit (version 9 or greater)
If compilation fails because of CUDA, you may need to edit 
C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v9.2\include\crt\host_config.h line 131 to read:
`#if _MSC_VER < 1600`

### Ubuntu Linux
Install the following packages:
```
libglew
libsdl2-dev
libsdl2-image-dev
````
You will also need to install the CUDA toolkit (version 9 or greater) using your preferred method.

## Known Issues
  * Source images larger than the half the size of the Window will case the resulting "rainy" image to be partially or completely drawn off screen.
  * Many raindrop parameters are hardcoded and/or not saved to disk after being modified.