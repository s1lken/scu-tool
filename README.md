# SCU Tool

The SCU Tool is designed to provide backdoor functionality to the System Controller Unit on the NXP i.MX8.
This coprocessor runs a wide-range of security critical functions and, among other exploits, has access to the entirety of the systems' memory.
If taken over, the SCU can be used to read and write memory at runtime on behalf of another application.

## Content
The SCU Tool is a collection of files that can be used to

1. nest a backdoor into the SCFW (`backdoor.c`)
2. connect to that backdoor through a userspace application on the i.MX8 (`imx_negotiator.c`)
3. communicate with that backdoor through a remote application (`remote_application.c`)

## Implementation of backdoor
The firmware running on the i.MX8 SCU is mostly closed source, yet in the porting kit a couple of source code files are exposed.
The porting kit happily signs any firmware that was generated, which gives us the possibility of implanting a backdoor into the firmware.
We do this by identifying a suitable hook function in `board.c` that is 

- called periodically: We need a reliable time frame we can perform our executions in.
- called frequently: We want the executions to be performed quickly after sending them to the SCU.
- not take long to complete: We do not want to starve the system.

We identified `board_get_control` as our best option.
From this function we can check if a connection has been established, and if so, execute commands on every call of the function.

```C
if (backdoor_connected) {
        /* backdoor established, check and execute instructions */
        backdoor_execute();
    } else {
        /* backdoor not established, search for client and update connected bool */
        backdoor_connected = backdoor_init();
    }
```

## Installation

1. Place the backdoor.c function into a appropriate directory inside the SCFW porting kit (be aware of the board you are using).
2. Find a suitable hook function in `board.c` and place the above code at the top of that function.
3. Build the SCFW and the U-Boot image (see vendors reference).
4. Build `imx-negotiator.c` and modify the rootfs to run the application on startup.
5. Modify the rootfs to allow for HUGE_TLB (`/proc/sys/vm/nr_hugepages`), which is used by the backdoor to easier find the negotiator.
6. Build `remote_application.c` on remote computer in same network
7. Flash board, run it, run `remote_application` binary.

## Usage

At the current state, SCU Tool supports the following commands:

1. read: copy a certain size of memory to the negotiator, which sends it to the remote application.

```shell
$ read [address] [size]
```

2. write: write a certain size of input into a specified address.

```shell
$ write [address] [size] [input]
```

## TODO
- [ ] Increase input/output buffer
- [ ] Access NIC from SCU directly to get rid of negotiator