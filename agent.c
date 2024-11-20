#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <libaudit.h>
#include <zmq.h>

void loop(const char* agent_name, void* connection) {
    while (1) {
        char event_buffer[MAX_AUDIT_MESSAGE_LENGTH];
        if (fgets(event_buffer, MAX_AUDIT_MESSAGE_LENGTH, stdin) != NULL) {
            size_t len = strlen(event_buffer);
            char topic[256];
            snprintf(topic, sizeof(topic), "%s:%s", agent_name, "audit");

            zmq_send(connection, topic, strlen(topic), ZMQ_SNDMORE);
            zmq_send(connection, event_buffer, len, 0);
        } else {
            if (feof(stdin)) {
                printf("End of input stream.\n");
                break;
            } else {
                perror("Failed to read from stdin");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s agent_name [options]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char* agent_name = argv[1];

    argc--;
    argv[1] = argv[0]; // Shift the program name down to where the agent name was
    argv++;            // Now argv[0] will still point to the program name for error reporting

    int c            = 0;
    int index        = 0;
    char* ip_str     = NULL;
    char* port_str   = NULL;

    static struct option long_options[] = {
        {"ip",      required_argument, 0,  1 },
        {"port",    required_argument, 0,  2 },
        {"help",    no_argument,       0, 'h'},
        {0,         0,                 0,  0 }
    };

    while ((c = getopt_long(argc, argv, "cf:h", long_options, &index)) != -1) {
        switch (c) {
            case 1:  // IP address
                ip_str = optarg;
                break;
            case 2:  // Port
                if (optarg) {
                    port_str = optarg;
                } else {
                    fprintf(stderr, "%s: option '--port' requires an argument\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                printf("Help: Use --ip <ip address>, --port <port>\n");
                exit(EXIT_SUCCESS);
            case '?':
                // getopt_long already printed an error message.
                break;
            default:
                printf("Unexpected argument %c\n", c);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        printf("Additional non-option arguments:\n");
        while (optind < argc) {
            printf("%s\n", argv[optind++]);
        }
        printf("Help: Use -c or --capture, -f or --file <filename>, --ip <ip address>, --port <port>\n");
        exit(EXIT_FAILURE);
    }


    if (!ip_str || !port_str) {
        fprintf(stderr, "IP and port must be specified.\n");
        exit(EXIT_FAILURE);
    }

    int port = atoi(port_str);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }

    void* context    = zmq_ctx_new();
    void* connection = zmq_socket(context, ZMQ_PUSH);
    char  dest_addr[512];
    sprintf(dest_addr, "tcp://%s:%d", ip_str, port);

    int rc = zmq_connect(connection, dest_addr);
    if (rc != 0) {
        perror("Failed to connect");
        printf("Trying to connect to %s. Got Error code: %d\n", dest_addr, errno);
        zmq_close(connection);
        zmq_ctx_destroy(context);
        exit(EXIT_FAILURE);
    }

    loop(agent_name, connection);

    zmq_close(connection);
    zmq_ctx_destroy(context);


    return 0;
}
