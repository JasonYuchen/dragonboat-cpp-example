# dragonboat-cpp-example

## About

**This repo shows the basic usages of dragonboat in C++ projects and is not suitable for benchmark**

This repo contains C++ examples for dragonboat.

The original repo of dragonboat examples can be found [here](https://github.com/lni/dragonboat-example)

Details can be found in each folder.

1. helloworld
2. multi-group
3. *concurrent statemachine
4. on-disk statemachine

* both C++ binding and examples are in progress, issues are welcome both here and [dragonboat](https://github.com/lni/dragonboat)
* for more details about v3.0 binding, see [v3.0pr](https://github.com/lni/dragonboat/pull/71)
* we also have a plan to implement a Python binding

## c++ binding v3.0 beta 

1. support all new features in nodehost.go with both sync/async version
2. support concurrent & on-disk statemachines with batchedUpdate method
3. prepareSnapshot & saveSnapshot take/return a ```void*``` to represent an arbitrary type
4. clients can specify both .so file name and factory name when starting cluster from a plugin
5. `StartCluster` now accepts a functional factory which allows caller to parse both C-style functions and lambda expressions

* v3.0 is almost completed and has been actively tested
* examples will be updated once v3.0 is ready

## Prerequisite

- GCC with C++11 support
- cmake
- dragonboat C++ binding

Instructions can be found [here](https://github.com/lni/dragonboat). 

The dragonboat C++ binding is based on CGo, thus its performance is worse than the original Go interface. Please refer to [Adventures in Cgo Performance](https://about.sourcegraph.com/go/gophercon-2018-adventures-in-cgo-performance) for more details.


## Build

```shell
cmake . -DEXAMPLE=helloworld
make
```

* example - ondisk is a RocksDB based key-value store thus RocksDB is required.

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