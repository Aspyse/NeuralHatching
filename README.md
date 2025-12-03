<h1 align="center">
  Neural Hatching: Computing Real-time Cross Fields Using Deep Convolutional Networks
</h1>

<h5 align="center">
 By Derek Royce Burias
</h5>

<p align="center">
 <img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/Aspyse/NeuralHatching">
 <a href="https://opensource.org/license/mit"><img src="https://img.shields.io/github/license/Aspyse/NeuralHatching"></a>
</p>

<p align="center">Real-time neural method for calculating cross fields and rendering crosshatch-style strokes.</p>


## Video Demo

https://github.com/user-attachments/assets/573d3bbb-b8a8-4ef8-b994-e7fcb6d6235b

## Python Notebooks

[Streamline visualization (Colab notebook viewer link)](https://colab.research.google.com/drive/1VXV40wjvwrJY0u0FO7uaqlZU9q9eaiBZ?usp=sharing)

[Model training (Colab notebook viewer link)](https://colab.research.google.com/drive/1Jo42dFrP3hp772G_5KVQE9SbsEmQYZij?usp=sharing)

## Dataset

The model is trained using screen-space normals, depth, and principal curvature data captured from models in the MPZ14, Thingi10K, and ShapeNetCore datasets.

The resulting dataset is made available here: [Google Drive](https://drive.google.com/file/d/1al9NW5k26OTgY2jzM3dHi1Otv2Zp-Voi/view?usp=sharing)


## Setup

### 1. Clone the repository

```sh
git clone https://github.com/Aspyse/NeuralHatching.git
cd NeuralHatching
```


### 2. Configure and build

The repository includes CMake configuration presets. In a command prompt, run the configuration using:
```sh
cmake --preset x64-release
```
Then build using:
```sh
cmake --build out\build\x64-release
```


### 3. Run executable

In a command prompt, run:
```sh
.\out\build\x64-release\Neuralhatching.exe
```


## Libraries Used

- [miniPLY](https://github.com/vilya/miniply)
- [DirectX Tool Kit](https://github.com/microsoft/DirectXTK)
- [OpenGL Mathematics (GLM)](https://github.com/g-truc/glm)
- [Eigen](https://github.com/PX4/eigen)
- [LBFGSpp](https://github.com/yixuan/LBFGSpp)


## Models Used

- [Thingi10K](https://github.com/Thingi10K/Thingi10K)
- [MPZ14](https://vcg-legacy.isti.cnr.it/Publications/2014/MPZ14/)
- [ShapeNetCore](https://huggingface.co/datasets/ShapeNet/ShapeNetCore)
