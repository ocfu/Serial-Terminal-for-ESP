//
//  serial_esp.cpp
//  xESP Tool serial
//
//  Created by ocfu on 01.10.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h> 
#include <sys/stat.h> 
#include <getopt.h>   
#include <pthread.h> 


#define PATTERN "--------------- CUT HERE FOR EXCEPTION DECODER ---------------"
#define PATTERN_LENGTH (sizeof(PATTERN) - 1)  // pattern length without '\0' termination

volatile int keepRunning = 1;
int logging = 0; // indicates logging to files
FILE *log_file = NULL;
int enable_dump_logging = 0; // indicates dump to files

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void intHandler(int dummy) {
   keepRunning = 0;
}

#pragma GCC diagnostic pop

// convert baudrate parameter to a constant
speed_t getBaudrate(int baudrate) {
   switch(baudrate) {
      case 1200: return B1200;
      case 2400: return B2400;
      case 4800: return B4800;
      case 9600: return B9600;
      case 19200: return B19200;
      case 38400: return B38400;
      case 57600: return B57600;
      case 115200: return B115200;
      case 230400: return B230400;
      default:
         fprintf(stderr, "Unsupported baudrate: %d\n", baudrate);
         exit(EXIT_FAILURE);
   }
}

void enable_raw_mode() {
   struct termios tty;
   
   // get current terminal settings for later restore
   if (tcgetattr(STDIN_FILENO, &tty) == -1) {
      perror("tcgetattr");
      exit(EXIT_FAILURE);
   }
   
   // switch the terminal to non-canon mode without echo
   tty.c_lflag &= ~(ICANON | ECHO); 
   tty.c_cc[VMIN] = 1;             // min. 1 char per read
   tty.c_cc[VTIME] = 0;            // no timeout
   
   // apply new terminal settings
   if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1) {
      perror("tcsetattr");
      exit(EXIT_FAILURE);
   }
}

void disable_raw_mode() {
   struct termios tty;
   
   // restor terminal settings
   if (tcgetattr(STDIN_FILENO, &tty) == -1) {
      perror("tcgetattr");
      exit(EXIT_FAILURE);
   }
   
   tty.c_lflag |= (ICANON | ECHO); 
   
   if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1) {
      perror("tcsetattr");
      exit(EXIT_FAILURE);
   }
}

// function to get data from stdin and send to the serial port
void *write_to_serial(void *arg) {
   int serial_port = *(int *)arg;
   char input_buf[256];
   
   while (keepRunning) {
      ssize_t num_bytes = read(STDIN_FILENO, input_buf, sizeof(input_buf));
      if (num_bytes > 0) {
         ssize_t bytes_written = write(serial_port, input_buf, num_bytes);
         if (bytes_written < 0) {
            perror("Error writing to serial port");
            break;
         }
      } else if (num_bytes < 0) {
         perror("Error reading from stdin");
         break;
      }
   }
   
   return NULL;
}

// function to capture the ESP exception pattern in received data from the serial port
int match_pattern(const char *buf, int buf_len, int *match_len) {
   static int pattern_pos = 0;
   
   for (int i = 0; i < buf_len; i++) {
      if (buf[i] == PATTERN[pattern_pos]) {
         pattern_pos++;
         if (pattern_pos == PATTERN_LENGTH) {
            *match_len = i + 1; 
            pattern_pos = 0;
            return 1;  // pattern found
         }
      } else {
         pattern_pos = 0;
      }
   }
   *match_len = 0;
   return 0;
}

int file_exists(const char *filename) {
   struct stat buffer;
   return (stat(filename, &buffer) == 0);
}

// function to create an index file name and copy existiong `dump.log`
void handle_existing_logfile(const char* filename) {
   char new_filename[64];
   int index = 1;
   
   if (file_exists(filename)) {
      // find the next index file name
      do {
         snprintf(new_filename, sizeof(new_filename), "%s.%d", filename, index++);
      } while (file_exists(new_filename));
      
      // copy the `dump.log` content
      FILE *src = fopen(filename, "r");
      FILE *dest = fopen(new_filename, "w");
      if (src && dest) {
         char buffer[256];
         size_t bytes;
         while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dest);
         }
         fclose(src);
         fclose(dest);
         printf("Existing log file copied to %s\n", new_filename);
      } else {
         if (src) fclose(src);
         if (dest) fclose(dest);
         perror("Error copying log file");
         exit(EXIT_FAILURE);
      }
   }
}

void usage_exit(char* name, int e) {
   fprintf(stderr, "Usage: %s [-d] [-b <baudrate>] <serial_port>\n", name);
   fprintf(stderr, "Options:\n");
   fprintf(stderr, " -d: write a detected dump into dump files (dump.log, dump_1.log, dump_2.log...).\n");
   fprintf(stderr, " -b: set baudrate, default: 115200.\n");
   exit(e);
}

int main(int argc, char *argv[]) {
   int opt;
   char *serial_port_name = NULL;
   int baudrate = 115200;
   
   while ((opt = getopt(argc, argv, "db:")) != -1) {
      switch (opt) {
         case 'd':
            enable_dump_logging = 1;
            break;
         case 'b':
            baudrate = atoi(optarg); 
            break;
         default:
            usage_exit(argv[0], EXIT_FAILURE);
      }
   }
   
   if (optind != argc - 1) {
      usage_exit(argv[0], EXIT_FAILURE);
   }
   
   serial_port_name = argv[optind];

   int serial_port = open(serial_port_name, O_RDWR);
   
   if (serial_port < 0) {
      printf("Error %i from open: %s\n", errno, strerror(errno));
      return 1;
   }
   
   // Set serial port parameters
   struct termios tty;
   if(tcgetattr(serial_port, &tty) != 0) {
      printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
      return 1;
   }
   
   tty.c_cflag &= ~PARENB; // No parity
   tty.c_cflag &= ~CSTOPB; // One stop bit
   tty.c_cflag &= ~CSIZE;
   tty.c_cflag |= CS8;     // 8 bits per byte
   tty.c_cflag &= ~CRTSCTS;// No hardware flow control
   tty.c_cflag |= CREAD | CLOCAL; // Enable reading and ignore control lines
   
   tty.c_lflag &= ~ICANON; // Disable canonical mode
   tty.c_lflag &= ~ECHO;   // Disable echo
   tty.c_lflag &= ~ECHOE;  // Disable erasure
   tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT, SUSP
   tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off software flow control
   
   tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes
   
   tty.c_cc[VMIN] = 1;
   tty.c_cc[VTIME] = 0;
   
   // Set baud rate
   speed_t baud = getBaudrate(baudrate);
   cfsetispeed(&tty, baud);
   cfsetospeed(&tty, baud);
   
   if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
      printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
      return 1;
   }
   
   //signal(SIGINT, intHandler);

   // set stdin to non-canon mode
   enable_raw_mode();

   
   // create the write thread from stdin to the serial port
   pthread_t writer_thread;
   if (pthread_create(&writer_thread, NULL, write_to_serial, &serial_port) != 0) {
      perror("Error creating thread");
      close(serial_port);
      disable_raw_mode();
      return 1;
   }

   char read_buf[256];
   memset(&read_buf, '\0', sizeof(read_buf));
   
   // Loop to read data
   while (keepRunning) {
      int num_bytes = (int)read(serial_port, &read_buf, sizeof(read_buf));
      
      if (num_bytes < 0) {
         printf("Error reading: %s\n", strerror(errno));
         break;
      }
      
      int match_len = 0;
      // verify dump pattern match, if dump loggin is enabled
      if (enable_dump_logging && match_pattern(read_buf, num_bytes, &match_len)) {
         if (logging == 0) {
            // pattern matches, start logging the dump
            printf("Pattern detected. Starting to log...\n");
            
            char filename[64];
            snprintf(filename, sizeof(filename), "dump.log");
            
            handle_existing_logfile(filename);

            log_file = fopen(filename, "w");
            if (log_file == NULL) {
               printf("Error opening file for writing.\n");
               break;
            }
            printf("Logging to file: %s\n", filename);
            logging = 1;
         } else {
            // pattern machtes (again), stop logging
            printf("Pattern detected again. Stopping log.\n");
            if (log_file != NULL) {
               fclose(log_file);
               log_file = NULL;
            }
            logging = 0;
         }
         
         // handle remaining data received
         if (match_len < num_bytes) {
            int remaining_bytes = num_bytes - match_len;
            if (logging && log_file != NULL) {
               fwrite(read_buf + match_len, sizeof(char), remaining_bytes, log_file);
               fflush(log_file);
            }
            for (int i = match_len; i < num_bytes; i++) {
               if (isprint(read_buf[i]) || read_buf[i] == 0x0d || read_buf[i] == 0x0a) {
                  printf("%c", read_buf[i]);
               } else {
                  printf("[%02X]", (unsigned char)read_buf[i]);  // print non-printable characters as hex value
               }
            }
            fflush(stdout);
         }
         continue;
      }
      
      // log data, if enabled
      if (logging && log_file != NULL) {
         fwrite(read_buf, sizeof(char), num_bytes, log_file);
         fflush(log_file);
      }
      
      // Output each character on stdout
      for (int i = 0; i < num_bytes; i++) {
            printf("%c", read_buf[i]); 
      }
      fflush(stdout);
   }
   
   // Thread beenden
   pthread_cancel(writer_thread);
   pthread_join(writer_thread, NULL);
   disable_raw_mode();

   close(serial_port);
   printf("\nSerial port closed. Program terminated.\n");
   return 0;
}
