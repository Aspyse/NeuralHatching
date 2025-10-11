<h1 align="center">
  Neural Hatching
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

To be uploaded


## Setup

### 1. Clone the repository

```sh
git clone https://github.com/Aspyse/NeuralHatching.git
cd HalftoneDemo
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

