#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

#define HUGE_TLB_SIZE (0x200000)
#define INPUT_SIZE (4096)
#define INPUT_INSTRUCTION_LEN (6)
#define ADDR_LEN (11)
#define INOUT_SIZE_LEN (32)

struct CommandStruct {
  char inst;
  int64_t addr;
  int64_t size;
  char input[INPUT_SIZE];
};

static struct CommandStruct *cmd;

void hex_dump(void *addr, unsigned int size) {
  char ascii[17];
  unsigned int i, j;
  ascii[16] = '\0';
  printf("%p  ", addr);
  for (i = 0; i < size; ++i) {
    printf("%02X ", ((unsigned char *)addr)[i]);
    if (((unsigned char *)addr)[i] >= ' ' &&
        ((unsigned char *)addr)[i] <= '~') {
      ascii[i % 16] = ((unsigned char *)addr)[i];
    } else {
      ascii[i % 16] = '.';
    }
    if ((i + 1) % 8 == 0 || i + 1 == size) {
      printf(" ");
      if ((i + 1) % 16 == 0) {
        printf("|  %s \n", ascii);
      } else if (i + 1 == size) {
        ascii[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          printf(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          printf("   ");
        }
        printf("|%s|\n", ascii);
      }
    }
  }
}

void sendCommand() {
  int clientSocket;
  struct sockaddr_in serverAddr;

  if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation error");
    exit(EXIT_FAILURE);
  }

  memset(&serverAddr, 0, sizeof(serverAddr));

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SERVER_PORT);

  if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
    perror("Invalid server address");
    exit(EXIT_FAILURE);
  }

  if (connect(clientSocket, (struct sockaddr *)&serverAddr,
              sizeof(serverAddr)) < 0) {
    perror("Connection error");
    exit(EXIT_FAILURE);
  }

  while (1) {
    char inputLine[HUGE_TLB_SIZE];
    char instruction[INPUT_INSTRUCTION_LEN];
    char address[ADDR_LEN];
    char size[INOUT_SIZE_LEN];

    printf("$ ");
    if (fgets(inputLine, sizeof(inputLine), stdin) != NULL) {
      sscanf(inputLine, "%s %s %s %[^\n]s", instruction, address, size,
             cmd->input);

      cmd->inst = instruction[0];
      if (cmd->inst != 'r' && cmd->inst != 'w') {
        printf("Error reading valid instruction.\n");
        continue;
      }

      cmd->addr = strtol(address, NULL, 16);
      cmd->size = strtol(size, NULL, 0);

      ssize_t bytesSent =
          send(clientSocket, (void *)cmd, sizeof(struct CommandStruct), 0);
      if (bytesSent < 0) {
        perror("Send error");
        exit(EXIT_FAILURE);
      }

      // Receive a response
      char response[cmd->size];
      ssize_t bytesRead = recv(clientSocket, response, sizeof(response), 0);

      if (bytesRead < 0) {
        perror("Receive error");
        exit(EXIT_FAILURE);
      } else if (bytesRead == 0) {
        printf("Server closed the connection\n");
        break;
      } else {
        hex_dump(cmd->input, cmd->size);
      }
    } else {
      printf("Error reading input.\n");
    }
  }
}

int main(void) {
  cmd = (struct CommandStruct *)malloc(sizeof(struct CommandStruct));
  if (cmd == NULL) {
    fprintf(stderr, "Failed to allocate memory for command struct.\n");
    return 1;
  }
  sendCommand();
  free((void *)cmd);
  return 0;
}

