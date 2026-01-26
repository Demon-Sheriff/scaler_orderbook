# sppe assignment

minimal hft market data system with shared memory and tcp transport.

## structure

```
sppe_assignment/
├── common/
│   ├── market_message.h   # pod struct for market data
│   ├── ring_buffer.h      # lock-free spsc queue
│   └── tsc_clock.h        # high-precision timestamp
├── publisher/             # generates and publishes data (cpu 0)
├── consumer_shm/          # reads from shared memory (cpu 1)
├── consumer_tcp/          # reads from tcp socket (cpu 2)
├── setup_env.sh           # system tuning script
└── CMakeLists.txt
```

## dependencies

- boost (asio)
- fmt

## build

```bash
mkdir build && cd build
cmake .. && make
```

## run

```bash
# terminal 1
./publisher

# terminal 2
./consumer_shm

# terminal 3
./consumer_tcp
```

## system tuning (optional)

```bash
sudo ./setup_env.sh
```
