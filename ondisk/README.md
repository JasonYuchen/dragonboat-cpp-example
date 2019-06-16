# example - ondisk

## about

More features like LeaderTransfer, MembershipChange, Multigroup are in progress.

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

Use ```put``` to store a KV pair and ```get``` to fetch a KV pair:

```shell
put key value
get key
```

Any error message will be displayed on the terminal, e.g. ```Not Found``` for getting a nonexistent key.

You can type in ```exit``` to terminate the node.

## details about on-disk state machine