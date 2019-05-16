# dragonboat-cpp-example

## About

This repo contains C++ examples for dragonboat.

The original repo of dragonboat examples can be found [here](https://github.com/lni/dragonboat-example)

Details can be found in each folder.

1. helloworld
2. multi-group(in progress)

## Prerequisite

- GCC with C++11 support
- cmake
- dragonboat C++ binding

Instructions can be found [here](https://github.com/lni/dragonboat)

## Build

```shell
cmake -DEXAMPLE=helloworld
make
```

## Run

Start three instances on the same machine in three different terminals:

```shell
./dragonboat_cpp_example 1
```

```shell
./dragonboat_cpp_example 2
```

```shell
./dragonboat_cpp_example 3
```