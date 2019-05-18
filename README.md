# dragonboat-cpp-example

## About

This repo contains C++ examples for dragonboat.

The original repo of dragonboat examples can be found [here](https://github.com/lni/dragonboat-example)

Details can be found in each folder.

1. helloworld
2. multi-group
3. *concurrent statemachine
4. *on-disk statemachine

* both C++ binding and examples are in progress, issues are welcome both here and [dragonboat](https://github.com/lni/dragonboat)

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
./dragonboat_cpp_example -nodeid 1
```

```shell
./dragonboat_cpp_example -nodeid 2
```

```shell
./dragonboat_cpp_example -nodeid 3
```