/*
 * Implementation of IPC layer for communication between ther server & client.
 *
 * Author: Rob Lyerly
 * Date: 8/8/2015
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

/* Unix domain socket headers */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "message.h"
#include "ipc.h"
#include "retvals.h"

///////////////////////////////////////////////////////////////////////////////
// Struct definitions
///////////////////////////////////////////////////////////////////////////////

struct _server_channel
{
	int listen_fd;
	struct sockaddr_un addr;
};

struct _client_channel
{
	int conn_fd;
	struct sockaddr_un addr;
	socklen_t addr_size;
};

///////////////////////////////////////////////////////////////////////////////
// Internal API
///////////////////////////////////////////////////////////////////////////////

static inline int open_socket()
{
	int fd;
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd == -1)
	{
#ifdef _VERBOSE
		perror("Could not open socket");
#endif
	}
	return fd;
}

static inline socklen_t setup_sockaddr(struct sockaddr_un* addr,
																			 const char* fname)
{
	// TODO check copied string size
	addr->sun_family = AF_UNIX;
	strncpy(addr->sun_path, fname, sizeof(addr->sun_path));
	return sizeof(addr->sun_family) + strlen(addr->sun_path);
}

static inline int send_message(int fd, const struct message* msg)
{
	ssize_t send_size;

	send_size = send(fd, msg, sizeof(struct message), 0);
	if(send_size == -1)
	{
#ifdef _VERBOSE
		perror("Could not send message");
#endif
		return IPC_SEND_ERR;
	}

	if(send_size != sizeof(struct message))
	{
#ifdef _VERBOSE
		fprintf(stderr, "Sent incorrect number of bytes (%lu vs %ld)\n",
						sizeof(struct message), send_size);
#endif
		return IPC_SEND_ERR;
	}

	return SUCCESS;
}

static inline int receive_message(int fd, struct message* msg)
{
	ssize_t receive_size;

	receive_size = recv(fd, msg, sizeof(struct message), 0);
	if(receive_size == -1)
	{
#ifdef _VERBOSE
		perror("Could not receive message");
#endif
		return IPC_RECV_ERR;
	}

	if(receive_size != sizeof(struct message))
	{
#ifdef _VERBOSE
		fprintf(stderr, "Received incorrect message size (%lu vs %ld)\n",
						sizeof(struct message), receive_size);
#endif
		return IPC_RECV_ERR;
	}

	return SUCCESS;
}

static inline int close_socket(int socket_fd)
{
	int retval = close(socket_fd);
#ifdef _VERBOSE
	if(retval)
		perror("Could not close socket");
#endif
	return retval;
}

static inline int close_ipc_file(const struct sockaddr_un* addr)
{
	int retval = remove(addr->sun_path);
#ifdef _VERBOSE
	if(retval)
		perror("Could not remove socket file");
#endif
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
// Server API
///////////////////////////////////////////////////////////////////////////////

/*
 * Open server's IPC channel.  Opens a socket on which to listen for
 * connections and binds it to the specified file.
 *
 * @param socket_fname file for which to bind the socket for listening
 * @return a server IPC channel or NULL if something went wrong
 */
server_channel open_server_channel(const char* socket_fname)
{
	server_channel chan;
	socklen_t sock_size;

	chan = (server_channel)malloc(sizeof(struct _server_channel));
	if(!chan) return NULL;

	// Get socket file descriptor
	chan->listen_fd = open_socket();
	if(chan->listen_fd == -1)
	{
		free(chan);
		return NULL;
	}

	// Bind socket file descriptor to file
	sock_size = setup_sockaddr(&chan->addr, socket_fname);
	if(bind(chan->listen_fd, (struct sockaddr*)&chan->addr, sock_size))
	{
#ifdef _VERBOSE
		perror("Could not bind socket to file");
#endif
		close_socket(chan->listen_fd);
		chan->listen_fd = -1;
		free(chan);
		return NULL;
	}

	return chan;
}

/*
 * Close & free server's IPC channel.
 *
 * @param chan server channel handle to close
 * @return 0 if successful, or an error code otherwise
 */
int close_server_channel(server_channel chan)
{
	int retval;

	if(!chan) return BAD_IPC_CHANNEL;

	retval = close_socket(chan->listen_fd);
	retval |= close_ipc_file(&chan->addr);

	free(chan);

	if(retval) return IPC_CLEANUP_ERR;
	else return SUCCESS;
}

/*
 * Listen on the Unix socket for a connection.  Return a connection struct
 * when client contacts server with a request.
 *
 * @param chan server channel handle on which to listen for connections
 * @side_effect conn struct containing file-descriptor & received message
 * @return 0 if successful, or an error code otherwise
 */
int server_listen(server_channel chan, struct connection* conn)
{
	struct sockaddr_un remote_addr;
	socklen_t remote_size = sizeof(remote_addr);

	if(!chan) return BAD_IPC_CHANNEL;
	if(!conn) return IPC_RECV_ERR;

	//Wait for a connection
	if(listen(chan->listen_fd, SERVER_QUEUE_SIZE) == -1)
	{
#ifdef _VERBOSE
		perror("Server could not listen on socket");
#endif
		return IPC_RECV_ERR;
	}

	//Accept the incoming connection, saving the file descriptor
	conn->fd = accept(chan->listen_fd,
										(struct sockaddr*)&remote_addr,
										&remote_size);
	if(conn->fd == -1)
	{
#ifdef _VERBOSE
		perror("Server could not accept connection");
#endif
		return IPC_RECV_ERR;
	}

	return receive_message(conn->fd, &conn->msg);
}

/*
 * Send a message over the specified connection.
 *
 * @param conn a struct encapsulating the data to send.  Note that the user is
 *             expected to have previously opened this connection & they have
 *             filled the message contained therein with a response.
 */
int server_send(const struct connection* conn)
{
	if(!conn || conn->fd == -1) return IPC_SEND_ERR;
	return send_message(conn->fd, &conn->msg);
}

/*
 * Closes a connection held open by the server.
 *
 * @param conn a struct encapsulating a connection to close.
 */
int server_close_connection(struct connection* conn)
{
	if(!conn || conn->fd == -1) return IPC_CLEANUP_ERR;
	int retval = close_socket(conn->fd);
	conn->fd = -1;
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
// Client API
///////////////////////////////////////////////////////////////////////////////

/*
 * Open a channel for communicating with the server.
 *
 * @param socket_fname Unix socket filename
 * @return an IPC channel to the server, or NULL if something went wrong
 */
client_channel open_client_channel(const char* socket_fname)
{
	client_channel chan;

	chan = (client_channel)malloc(sizeof(struct _client_channel));
	if(!chan) return NULL;

	// Get socket file descriptor
	chan->conn_fd = open_socket();
	if(chan->conn_fd == -1)
	{
		free(chan);
		return NULL;
	}

	chan->addr_size = setup_sockaddr(&chan->addr, socket_fname);
	if(connect(chan->conn_fd,
						 (struct sockaddr*)&chan->addr,
						 chan->addr_size) == -1)
	{
#ifdef _VERBOSE
		perror("Could not connect to server");
#endif
		close_socket(chan->conn_fd);
		free(chan);
		return NULL;
	}

	return chan;
}

/*
 * Re-open a connection to the server.
 *
 * @param chan a client channel that has been previously opened & closed.
 * @return 0 if the channel was successfully re-opened, or an error code
 * 				 otherwise
 */
int reopen_client_conn(client_channel chan)
{
	if(!chan || chan->addr.sun_family != AF_UNIX) return BAD_IPC_CHANNEL;

	// Get a new socket file descriptor
	chan->conn_fd = open_socket();
	if(chan->conn_fd == -1) return IPC_SETUP_ERR;

	// Re-connect to socket file
	if(connect(chan->conn_fd,
						 (struct sockaddr*)&chan->addr,
						 chan->addr_size) == -1)
	{
#ifdef _VERBOSE
		perror("Could not re-connect to server");
#endif
		close_socket(chan->conn_fd);
		chan->conn_fd = -1;
		return IPC_SETUP_ERR;
	}

	return SUCCESS;
}

/*
 * Close a connection to the server.  Can be later re-opened using
 * reopen_client_conn().
 *
 * @param chan a client channel handle
 * @return 0 if successfully closed or an error code otherwise
 */
int close_client_connection(client_channel chan)
{
	int retval;

	if(!chan) return BAD_IPC_CHANNEL;

	retval = close_socket(chan->conn_fd);
	chan->conn_fd = -1;

	if(retval) return IPC_CLEANUP_ERR;
	else return SUCCESS;
}

/*
 * Close & free client's IPC channel.
 *
 * @param chan a client channel handle
 * @return 0 if successfully closed or an error code otherwise
 */
int close_client_channel(client_channel chan)
{
	int retval = 0;

	if(!chan) return BAD_IPC_CHANNEL;

	if(chan->conn_fd != -1)
		retval = close_client_connection(chan);
	free(chan);

	if(retval) return IPC_CLEANUP_ERR;
	else return SUCCESS;
}

/*
 * Send a message to server.
 *
 * @param chan client IPC channel
 * @param msg message to send to server
 * @return 0 if successfully sent or an error code otherwise
 */
int client_send(client_channel chan, struct message* msg)
{
	if(!chan) return BAD_IPC_CHANNEL;
	if(!msg) return IPC_SEND_ERR;
	return send_message(chan->conn_fd, msg);
}

/*
 * Receive a message from the server.
 *
 * @param chan client IPC channel
 * @param msg storage for received message
 * @return 0 if successfully received or an error code otherwise
 */
int client_receive(client_channel chan, struct message* msg)
{
	if(!chan) return BAD_IPC_CHANNEL;
	if(!msg) return IPC_RECV_ERR;
	return receive_message(chan->conn_fd, msg);
}

