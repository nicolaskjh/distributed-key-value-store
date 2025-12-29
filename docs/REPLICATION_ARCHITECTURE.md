# Master-Replica Replication Architecture

## Overview

Asynchronous master-replica replication system for horizontal read scaling.

## Architecture

```
                    ┌─────────────┐
                    │   CLIENT    │
                    └──────┬──────┘
                           │
                    WRITES │ READS
                           │
                    ┌──────▼──────┐
                    │   MASTER    │ ◄─── Accepts ALL writes
                    │  (Port 50051)│
                    └──────┬──────┘
                           │
            Async Replication (Fire & Forget)
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
    ┌────▼────┐       ┌────▼────┐      ┌────▼────┐
    │ REPLICA │       │ REPLICA │      │ REPLICA │
    │   #1    │       │   #2    │      │   #3    │
    │(50052)  │       │(50053)  │      │(50054)  │
    └─────────┘       └─────────┘      └─────────┘
         ▲                 ▲                 ▲
         │                 │                 │
         └─────────────────┴─────────────────┘
                    READ requests
```

## Node Roles

### Master Node
- Accepts all write operations (SET, DELETE, EXPIRE)
- Applies operations locally and logs to AOF
- Assigns monotonically increasing sequence IDs
- Replicates operations asynchronously to all replicas
- Responds to client immediately without waiting for replicas
- Can serve read requests

### Replica Nodes
- Read-only from client perspective
- Only accept ReplicationCommand RPCs from master
- Apply operations in sequence ID order
- Serve read requests to distribute load
- Maintain independent persistence (RDB + AOF)

## Write Flow

```
Client → Master.Set("key", "value")
           ↓
        [Store locally]
           ↓
        [Return OK]  ←─── Client unblocked
           ↓
    [Replicate async] ───┐
                         ├──→ Replica1
                         ├──→ Replica2
                         └──→ Replica3
```

1. Client sends write to master
2. Master applies operation locally
3. Master responds to client immediately
4. Master sends ReplicationCommand to all replicas asynchronously
5. Replicas apply operations independently

## Read Flow

- Clients can read from master (latest data) or replicas (eventual consistency)
- Replicas may lag behind master by network latency + processing time

## Replication Protocol

### ReplicationCommand Message
```protobuf
message ReplicationCommand {
  CommandType type = 1;      // SET, DELETE, or EXPIRE
  string key = 2;
  string value = 3;
  int32 seconds = 4;
  int64 sequence_id = 5;     // Monotonically increasing
}
```

### Sequence IDs
- Master assigns using atomic counter
- Ensures operations applied in same order on all nodes
- Prevents reordering of operations

### Preventing Replication Loops
- Master uses `Storage::Set()` which triggers replication
- Replicas use `Storage::SetFromReplication()` which does not trigger replication
- Role check before replicating: `if (IsMaster())`

## Data Consistency

**Eventual Consistency:** All replicas converge to same state given no new writes. Replicas may temporarily return stale data during replication lag.

## Configuration

Start master:
```bash
./kvstore_server --master --address 0.0.0.0:50051 --replicas localhost:50052,localhost:50053
```

Start replica:
```bash
./kvstore_server --replica --address 0.0.0.0:50052 --master-address localhost:50051
```

