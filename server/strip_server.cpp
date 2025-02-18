#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <csignal> 
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>

#define GROUP "224.1.1.1"
#define PORT 7780
#define MSG_LEN 10
#define MAX_SEND_RETRIES 30

FILE *cava;
int sock;
struct sockaddr_in addr;

void clearLed() {
  if (sock > 0) {
    printf("Clean up led\n");
    sendto(sock, "c", 1, 0, (struct sockaddr*) &addr, sizeof(addr));
  }
}

void stopCava() {
  if (cava != NULL) {
    pclose(cava);
  }
}

void signalHandler(int signal) {
  stopCava();
  clearLed();

  printf("Bye bye <3 (signal=%d)\n", signal);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Path to cava config is requred!\n");
    return 1;
  }

  signal(SIGINT, signalHandler);
  signal(SIGQUIT, signalHandler);
  signal(SIGABRT, signalHandler);
  signal(SIGTERM, signalHandler);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(GROUP);
  addr.sin_port = htons(PORT);

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    perror("Failed to create socket");
    return 1;
  }

  u_int yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
    perror("Reusing ADDR failed");
    return 1;
  }

  char cmd[9 + strlen(argv[1])];
  sprintf(cmd, "cava -p %s", argv[1]);
  cava = popen(cmd, "r");
  if (cava == NULL) {
    perror("Failed to pipe in cava");
    return 1;
  }

  std::cout << "Sending cava data" << std::endl;

  char data[MSG_LEN];
  int nbytes = 0, retries = 0;
  while(fgets(data, MSG_LEN, cava) != NULL) {
    nbytes = sendto(sock, data, MSG_LEN, 0, (struct sockaddr*) &addr, sizeof(addr));
    if (nbytes < 0) {
      if (retries > MAX_SEND_RETRIES) {
        perror("Failed to send data");
        return 1;
      }

      retries++;
      sleep(1);
    } else if (retries > 0) {
      retries = 0;
    }
  }

  return 0;
}