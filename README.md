# imx8-scu-backdoor

if (backdoor_connected) {
        /* backdoor established, check and execute instructions */
        backdoor_execute();
    } else {
        /* backdoor not established, search for client and update connected bool */
        backdoor_connected = backdoor_init();
    }
