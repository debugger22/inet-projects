/*
* Please make sure that you enter server's MAC address in dest[ETH_ALEN]
* before executing the program.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>

union ethernetframe{
    struct{
      struct ethhdr    header;
      unsigned char    data[ETH_DATA_LEN];
    } field;
    unsigned char    buffer[ETH_FRAME_LEN];
};

int main(int argc, char **argv) {
    if (argc < 2){
      printf("Please provide the data to send in argument\n");
      return -1;
    }

    char *iface = "eth0";
    unsigned char dest[ETH_ALEN]
             = { 0x10, 0xbf, 0x48, 0x0d, 0xe7, 0x10 }; // { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    unsigned short proto = 0x88b5; // Ethernet payload type
    unsigned char *data = argv[1];  // Reading data from command line argument
    unsigned short data_len = strlen(data);

    // Creating packet socket
    int s;
    if ((s = socket(AF_PACKET, SOCK_RAW, htons(proto))) < 0) {
        printf("Error: could not open socket\n");
        return -1;
    }

    // Looking up the interface index
    struct ifreq buffer;
    int index;
    memset(&buffer, 0x00, sizeof(buffer));
    strncpy(buffer.ifr_name, iface, IFNAMSIZ);
    if (ioctl(s, SIOCGIFINDEX, &buffer) < 0) {
        printf("Error: could not get interface index\n");
        close(s);
        return -1;
    }
    index = buffer.ifr_ifindex;

    // Looking up source MAC address
    unsigned char source[ETH_ALEN];
    if (ioctl(s, SIOCGIFHWADDR, &buffer) < 0) {
        printf("Error: could not get interface address\n");
        close(s);
        return -1;
    }
    memcpy((void*)source, (void*)(buffer.ifr_hwaddr.sa_data),
           ETH_ALEN);


    // Filling in the packet fields
    union ethernetframe frame;
    memcpy(frame.field.header.h_dest, dest, ETH_ALEN);
    memcpy(frame.field.header.h_source, source, ETH_ALEN);
    frame.field.header.h_proto = htons(proto);
    memcpy(frame.field.data, data, data_len);

    unsigned int frame_len = data_len + ETH_HLEN;

    // Filling in the sockaddr_ll struct
    struct sockaddr_ll saddrll;
    memset((void*)&saddrll, 0, sizeof(saddrll));
    saddrll.sll_family = PF_PACKET;
    saddrll.sll_ifindex = index;
    saddrll.sll_halen = ETH_ALEN;
    memcpy((void*)(saddrll.sll_addr), (void*)dest, ETH_ALEN);

    // Sending the packet
    if (sendto(s, frame.buffer, frame_len, 0,
               (struct sockaddr*)&saddrll, sizeof(saddrll)) > 0){
      printf("Data sent successfully!\n");
    }else{
      printf("Error, could not send data.\n");
    }

    // Closing socket
    close(s);


    // Initiating receiver socket
    short int sock_desc=0, count=0;
    if ((sock_desc = socket(AF_PACKET, SOCK_RAW, htons(proto))) < 0)
          printf("\nError creating socket\n");
    else{
          printf("\nListening...\n");
          struct sockaddr_ll sock_addrll;
          unsigned char frame_buffer[ETH_FRAME_LEN];
          int data_rcvd=0;
          socklen_t sock_addrll_len = (socklen_t)sizeof(sock_addrll);
          data_rcvd = recvfrom(
                      sock_desc,
                      frame_buffer,
                      ETH_FRAME_LEN,
                      0x00,
                      (struct sockaddr*) &sock_addrll,
                      &sock_addrll_len
                  );
          printf("\nData received: %s\n",frame_buffer+sizeof(struct ethhdr));
    }
    return 0;
}
