#include "main/backdoor.h"
#include "main/main.h"

#define HUGE_TLB_SIZE (0x200000)
#define INPUT_SIZE (4096)

#define MEMORY_START (0xd4800000)
#define MEMORY_END (0x80000000)

#define READ_INSTRUCTION ("r")
#define WRITE_INSTRUCTION ("w")

const char *MAGIC_STR = "backdoor";
const char *CONNECTION_STR = "connected";

struct CommandStruct {
  char inst;
  int64_t addr;
  int64_t size;
  char input[INPUT_SIZE];
};

static struct CommandStruct *cmd;

/* Function to initialize the backdoor communication */
bool backdoor_init(void) {
  /* Reset connection pointer to start of memory */
  void *connection_ptr = (void *)MEMORY_START;
  /* Look through memory to find the backdoor client */
  while (connection_ptr > (void *)MEMORY_END) {
    /* Backdoor client has a magic string to identify itself */
    if (!memcmp(connection_ptr, MAGIC_STR, sizeof(MAGIC_STR))) {
      board_print(2,
                  "Connection with backdoor client established at addr: %p!\n",
                  connection_ptr);
      /* Notify client of established connection */
      memcpy(connection_ptr, CONNECTION_STR, sizeof(CONNECTION_STR));
      /* set CommandStruct */
      cmd = connection_ptr;
      return true;
    }
    /* Go to next HugeTLB address. If HugeTLB is not enabled, this needs to be
     * changed and finding the client will take a lot longer */
    connection_ptr = (unsigned char *)connection_ptr - HUGE_TLB_SIZE;
  }
  return false;
}

/* Function to execute backdoor commands */
void backdoor_execute(void) {
  /* Write instruction */
  if (!memcmp(cmd, WRITE_INSTRUCTION, 1)) {
    /* Copy the input to the target address */
    memcpy((char *)(uintptr_t)cmd->addr, cmd->input, cmd->size);
  }
  /* Read instruction */
  else if (!memcmp(cmd, READ_INSTRUCTION, 1)) {
    /* Copy memory of target address to address set by client*/
    memcpy(cmd->input, (char *)(uintptr_t)cmd->addr, cmd->size);
  }
  /* reset instruction */
  cmd = 0;
}
