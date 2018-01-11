/*
 * IPC Layer API - interface to the IPC layer for both client & server.
 *
 * Author: Rob Lyerly
 * Date: 8/8/2015
 */

#ifndef _IPC_H
#define _IPC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Socket definitions */
#define UNIX_DOMAIN_SOCK_TEXT "sockets"
#define SOCKET_FILE "/var/run/aira-lb.sock"
#define SERVER_QUEUE_SIZE 128

typedef struct _client_channel* client_channel;
typedef struct _server_channel* server_channel;

/* Server interface */
server_channel open_server_channel(const char* socket_fname);
int close_server_channel(server_channel chan);
int server_listen(const server_channel chan, struct connection* conn);
int server_send(const struct connection* conn);
int server_close_connection(struct connection* conn);

/* Client interface */
client_channel open_client_channel(const char* socket_fname);
int reopen_client_conn(client_channel chan);
int close_client_connection(client_channel chan);
int close_client_channel(client_channel chan);
int client_send(client_channel chan, struct message* msg);
int client_receive(client_channel chan, struct message* msg);

#ifdef __cplusplus
}
#endif

#endif /* _IPC_H */

