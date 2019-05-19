# example - multigroup

## start

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

You can type in ```exit``` to terminate the node.

For simplicity, membership change is not supported in this example.

Use ```set``` to store a KV pair based on the hash value of key.

```shell
set [key] [value]
```

Type in key to display the stored value.

```shell
[key]
```

Use ```display``` to show all KV pairs in the specified cluster.

```shell
display [clusterID]
```

## multigroup

This example starts two Raft cluster to support a hash sharding KV store.

