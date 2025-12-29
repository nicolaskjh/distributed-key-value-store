# Test Suite

Comprehensive test scripts for the distributed key-value store.

## Overview

The test suite consists of:
- **Shell scripts** - Automated test scenarios (`.sh` files)
- **Test clients** - C++ executables used by the scripts (`.cpp` files)

## Test Scripts

### Run All Tests

```bash
./test_all.sh
```

Runs all test scripts sequentially and provides a summary.

### Individual Test Scripts

1. **test_basic_operations.sh** - Tests core operations
   - SET, GET, DELETE, CONTAINS operations
   - Basic client-server communication
   - Single node functionality

2. **test_ttl_expiration.sh** - Tests time-to-live functionality
   - EXPIRE command
   - TTL query
   - Automatic expiration
   - Time-based cleanup

3. **test_persistence.sh** - Tests data durability
   - RDB snapshot creation (60 second interval)
   - AOF log writing
   - Server restart and recovery
   - Data integrity after restart

4. **test_replication.sh** - Tests distributed replication
   - Master-replica setup
   - Write to master
   - Read from replicas
   - Eventual consistency verification

5. **test_concurrent_clients.sh** - Tests concurrent access
   - Multiple simultaneous clients
   - Thread-safe operations
   - Data consistency under load

## Test Client Executables

The shell scripts use these C++ test clients (built automatically with the project):

- **kvstore_client** - Full-featured interactive test client
  - Tests all operations: SET, GET, DELETE, CONTAINS, EXPIRE, TTL
  - Used by most test scripts
  - Source: `client_test.cpp`

- **read_test** - Simple read-only client
  - Quick GET operations to verify data
  - Used for replication verification
  - Source: `read_test.cpp`

- **snapshot_test** - Writes test data for persistence testing
  - Creates sample dataset
  - Used by persistence tests
  - Source: `snapshot_test.cpp`

- **verify_persistence** - Verifies data after server restart
  - Checks data recovery from RDB/AOF
  - Used by persistence tests
  - Source: `verify_persistence.cpp`

## Running Tests

### From the tests directory:

```bash
cd tests

# Run all tests
./test_all.sh

# Run individual tests
./test_basic_operations.sh       # Core operations
./test_ttl_expiration.sh          # TTL and expiration
./test_persistence.sh             # Persistence and recovery (~70s)
./test_replication.sh             # Master-replica replication
./test_concurrent_clients.sh      # Concurrent access
```

## Prerequisites

1. **Build the project** (this creates all test executables):

```bash
cd /path/to/distributed-key-value-store
mkdir -p build && cd build
cmake ..
make -j8
```

2. **Make scripts executable** (if needed):

```bash
cd ../tests
chmod +x *.sh
```

## Notes

- Scripts automatically clean up processes and temporary files
- Each test runs in isolation with fresh data
- Replication test creates temporary `replica1/` and `replica2/` directories
- Persistence test waits 65 seconds for RDB snapshot (configured at 60s interval)
