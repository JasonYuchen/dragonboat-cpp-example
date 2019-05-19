# example - helloworld

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

Any input message will be replicated to all three nodes.

You can type in ```exit``` to terminate the node.

## availability

Kill one node and then input messages in the rest terminals. The messages are replicated to the majority of nodes, thus the Raft cluster is still available.

Now restart the killed node, the dragonboat make sure it is restored and updated.

## membership change

1. input ```add localhost:63004 4```
2. start a new node in a new terminal ```./dragonboat_cpp_example -nodeid 4 -addr localhost:63004 -join```
3. remove a existing node ```remove 4```