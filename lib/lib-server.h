/*
copyright: Boaz segev, 2016
license: MIT

Feel free to copy, use and enjoy according to the license provided.
*/
#ifndef LIB_SERVER_H
#define LIB_SERVER_H

#define LIB_SERVER_VERSION "0.2.1"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* lib server is based off and requires the following libraries: */
#include "libreact.h"
#include "libasync.h"

/** \file
## Lib-Server - a dynamic protocol server library

lib-server builds off the struct Reactor introduced in lib-react.h and manages
everything that makes a server run, including the thread pool (using lib-async),
rocess forking, accepting new connections, setting up the initial protocol for
new connections, user space socket writing buffers (using `lib-buffer.h`), etc'.

It's awesome :-)

The lib-server API is accessed using the global `Server` object, which is only
global member of the struct ___Server__API____.

Use:

    #include "lib-server.h"
    #include <stdio.h>
    #include <string.h>

    ; // we don't have to, but printing stuff is nice...
    void on_open(server_pt server, int sockfd) {
      printf("A connection was accepted on socket #%d.\n", sockfd);
    }
    ; // server_pt is just a nicely typed pointer,
    void on_close(server_pt server, int sockfd) {
      printf("Socket #%d is now disconnected.\n", sockfd);
    }

    ; // a simple echo... this is the main callback
    void on_data(server_pt server, int sockfd) {
      char buff[1024]; // We'll assign a reading buffer on the stack
      ssize_t incoming = 0;
      ; // Read everything, this is edge triggered, `on_data` won't be called
      ; // again until all the data was read.
      while ((incoming = Server.read(server, sockfd, buff, 1024)) > 0) {
        ; // since the data is stack allocated, we'll write a copy
        ; // optionally, we could avoid a copy using Server.write_move
        Server.write(server, sockfd, buff, incoming);
        if (!memcmp(buff, "bye", 3)) {
          ; // closes the connection automatically AFTER the buffer was sent.
          Server.close(server, sockfd);
        }
      }
    }

    ; // running the server
    int main(void) {
      ; // We'll create the echo protocol object as the server's default
      struct Protocol protocol = {.on_open = on_open,
                                  .on_close = on_close,
                                  .on_data = on_data,
                                  .service = "echo"};
      ; // We'll use the macro start_server, because our settings are simple.
      ; // (this will call Server.listen(settings) with our settings)
      start_server(.protocol = &protocol, .timeout = 10, .threads = 8);
    }

*/

/**************************************************************************/ /**
* General info
*/

/* The following types are defined for the userspace of this library: */

struct Server;                    /** used internally. no public data exposed */
typedef struct Server* server_pt; /** a pointer to a server object */
struct ServerSettings;            /** sets up the server's behavior */
struct Protocol;                  /** controls connection events */

/**
The start_server(...) macro is a shortcut that allows to easily create a
ServerSettings structure and start the server is a simple manner.
*/
#define start_server(...) Server.listen((struct ServerSettings){__VA_ARGS__})

/**************************************************************************/ /**
* The Protocol

the Protocol struct defines the callbacks used for the connection and sets the
behaviour for the connection's protocol.
*/
struct Protocol {
  /** a string to identify the protocol's service (i.e. "http"). */
  char* service;
  /** called when a connection is opened */
  void (*on_open)(struct Server*, int sockfd);
  /** called when a data is available */
  void (*on_data)(struct Server*, int sockfd);
  /** called when the socket is ready to be written to. */
  void (*on_ready)(struct Server*, int sockfd);
  /** called when the server is shutting down,
   * but before closing the connection. */
  void (*on_shutdown)(struct Server*, int sockfd);
  /** called when the connection was closed */
  void (*on_close)(struct Server*, int sockfd);
  /** called when the connection's timeout was reached */
  void (*ping)(struct Server*, int sockfd);
};

/**************************************************************************/ /**
* The Server Settings

These settings will be used to setup server behaviour. missing settings will be
filled in with default values. only the `protocol` setting, which sets the
default protocol, is required.
*/
struct ServerSettings {
  /** the default protocol. */
  struct Protocol* protocol;
  /** the port to listen to. defaults to 3000. */
  char* port;
  /** the address to bind to. defaults to NULL (all localhost addresses) */
  char* address;
  /** called when the server starts, allowing for further initialization, such
  as timed event scheduling.
  this will be called seperately for every process. */
  void (*on_init)(struct Server* server);
  /** called when the server is done, to clean up any leftovers. */
  void (*on_finish)(struct Server* server);
  /** called whenever an event loop had cycled (a "tick"). */
  void (*on_tick)(struct Server* server);
  /** called if an event loop cycled with no pending events. */
  void (*on_idle)(struct Server* server);
  /** called each time a new worker thread is spawned (within the new thread).
   */
  void (*on_init_thread)(struct Server* server);
  /** a NULL terminated string for when the server is busy (defaults to NULL, a
   * simple disconnection with no message). */
  char* busy_msg;
  /** opaque user data. **/
  void* udata;
  /**
  ets the amount of threads to be created for the server's thread-pool.
  Defaults to 1 - the reactor and all callbacks will work using a single working
  thread, allowing for an evented single threaded design.
  */
  int threads;
  /** sets the amount of processes to be used (processes will be forked).
  Defaults to 1 working processes (no forking). */
  int processes;
  /** sets the timeout for new connections. defaults to 5 seconds. */
  unsigned char timeout;
};

/**************************************************************************/ /**
* The Server API
* (and helper functions)

The API and helper functions described here are accessed using the `Server`
object. i.e:

    Server.listen(struct ServerSettings { ... });

*/
extern const struct ___Server__API____ {
  /****************************************************************************
  * Server settings and objects
  */

  /** Returns the originating process pid */
  pid_t (*root_pid)(struct Server* server);
  /** Allows direct access to the reactor object. use with care. */
  struct Reactor* (*reactor)(struct Server* server);
  /** Allows direct access to the server's original settings. use with care. */
  struct ServerSettings* (*settings)(struct Server* server);
  /**
  Returns the adjusted capacity for any server instance on the system.

  The capacity is calculating by attempting to increase the system's open file
  limit to the maximum allowed, and then marginizing the result with respect to
  possible memory limits and possible need for file descriptors for response
  processing.
  */
  long (*capacity)(void);

  /****************************************************************************
  * Server actions
  */

  /**
  Listens to a server with the following server settings (which MUST include
  a default protocol).

  This method blocks the current thread until the server is stopped (either
  though a `srv_stop` function or when a SIGINT/SIGTERM is received).
  */
  int (*listen)(struct ServerSettings);
  /** stops a specific server, closing any open connections. */
  void (*stop)(struct Server* server);
  /** stops any and all server instances, closing any open connections. */
  void (*stop_all)(void);

  /****************************************************************************
  * Socket settings and data
  */

  /** Returns true if a specific connection's protected callback is running.

  Protected callbacks include only the `on_message` callback and tasks forwarded
  to the connection using the `td_task` or `each` functions.
  */
  unsigned char (*is_busy)(struct Server* server, int sockfd);
  /** Retrives the active protocol object for the requested file descriptor. */
  struct Protocol* (*get_protocol)(struct Server* server, int sockfd);
  /**
  Sets the active protocol object for the requested file descriptor.

  Returns -1 on error (i.e. connection closed), otherwise returns 0.
  */
  int (*set_protocol)(struct Server* server,
                      int sockfd,
                      struct Protocol* new_protocol);
  /** retrives an opaque pointer set by `set_udata` and associated with the
  connection.

  since no new connections are expected on fd == 0..2, it's possible to store
  global data in these locations. */
  void* (*get_udata)(struct Server* server, int sockfd);
  /** Sets the opaque pointer to be associated with the connection. returns the
  old pointer, if any. */
  void* (*set_udata)(struct Server* server, int sockfd, void* udata);
  /** Sets the timeout limit for the specified connectionl, in seconds, up to
  255 seconds (the maximum allowed timeout count). */
  void (*set_timeout)(struct Server* server, int sockfd, unsigned char timeout);

  /****************************************************************************
  * Socket actions
  */

  /** Attaches an existing connection (fd) to the server's reactor and protocol
  management system, so that the server can be used also to manage connection
  based resources asynchronously (i.e. database resources etc').
  */
  int (*attach)(struct Server* server, int sockfd, struct Protocol* protocol);
  /** Closes the connection.

  If any data is waiting to be written, close will
  return immediately and the connection will only be closed once all the data
  was sent. */
  void (*close)(struct Server* server, int sockfd);
  /** Hijack a socket (file descriptor) from the server, clearing up it's
  resources. The control of hte socket is totally relinquished.

  This method will block until all the data in the buffer is sent before
  releasing control of the socket. */
  int (*hijack)(struct Server* server, int sockfd);
  /** Counts the number of connections for the specified protocol (NULL = all
  protocols). */
  long (*count)(struct Server* server, char* service);
  /// "touches" a socket, reseting it's timeout counter.
  void (*touch)(struct Server* server, int sockfd);

  /****************************************************************************
  * Read and Write
  */

  /**
  Sets up the read/write hooks, allowing for transport layer extensions (i.e.
  SSL/TLS) or monitoring extensions.

  These hooks are only relevent when reading or writing from the socket using
  the server functions (i.e. `Server.read` and `Server.write`).

  These hooks are attached to the specified socket and they are cleared
  automatically once the connection is closed.

  **Writing hook**

  A writing hook will be used instead of the `write` function to send data to
  the socket. This allows this buffer to be used for special protocol extension
  or transport layers, such as SSL/TLS.

  A writing hook is a function that takes in a pointer to the server (the
  buffer's owner), the socket to which writing should be performed (fd), a
  pointer to the data to be written and the length of the data to be written:

  A writing hook should return the number of bytes actually sent from the data
  buffer (not the number of bytes sent through the socket, but the number of
  bytes that can be marked as sent).

  A writing hook should return -1 if the data couldn't be sent and processing
  should be stop (the connection was lost or suffered a fatal error).

  A writing hook should return 0 if no data was sent, but the connection should
  remain open or no fatal error occured.

  i.e.:

      ssize_t writing_hook(server_pt srv, int fd, void* data, size_t len) {
        int sent = write(fd, data, len);
        if (sent < 0 && (errno & (EWOULDBLOCK | EAGAIN | EINTR)))
          sent = 0;
        return sent;
      }

  **Reading hook**

  The reading hook, similar to the writing hook, should behave the same as
  `read` and accepts the same arguments as the `writing_hook`, except the
  `length` argument should reffer to the size of the buffer (or the amount of
  data to be read, if less then the size of the buffer).

  The return values are the same as the writing hook's return values, except
  the number of bytes returned refers to the number of bytes written to the
  buffer.

  i.e.

  ssize_t reading_hook(server_pt srv, int fd, void* buffer, size_t size) {
    ssize_t read = 0;
    if ((read = recv(fd, buffer, max_len, 0)) > 0) {
      return read;
    } else {
      if (read && (errno & (EWOULDBLOCK | EAGAIN)))
        return 0;
    }
    return -1;
  }

  */
  void (*rw_hooks)(
      server_pt srv,
      int sockfd,
      ssize_t (*writing_hook)(server_pt srv, int fd, void* data, size_t len),
      ssize_t (
          *reading_hook)(server_pt srv, int fd, void* buffer, size_t size));

  /** Reads up to `max_len` of data from a socket. the data is stored in the
  `buffer` and the number of bytes received is returned.

  Returns -1 if an error was raised and the connection was closed.

  Returns the number of bytes written to the buffer. Returns 0 if no data was
  available.
  */
  ssize_t (*read)(server_pt srv, int sockfd, void* buffer, size_t max_len);
  /** Copies & writes data to the socket, managing an asyncronous buffer.

  returns 0 on success. success means that the data is in a buffer waiting to
  be written. If the socket is forced to close at this point, the buffer will be
  destroyed (never sent).

  On error, returns -1. Returns 0 on success
  */
  ssize_t (*write)(server_pt srv, int sockfd, void* data, size_t len);
  /** Writes data to the socket, moving the data's pointer directly to the
  buffer.

  Once the data was written, `free` will be called to free the data's memory.

  On error, returns -1. Returns 0 on success
  */
  ssize_t (*write_move)(server_pt srv, int sockfd, void* data, size_t len);
  /** Copies & writes data to the socket, managing an asyncronous buffer.

  Each call to a `write` function considers it's data atomic (a single package).

  The `urgent` varient will send the data as soon as possible, without
  distrupting
  any data packages (data written using `write` will not be interrupted in the
  middle).

  On error, returns -1. Returns 0 on success
  */
  ssize_t (*write_urgent)(server_pt srv, int sockfd, void* data, size_t len);
  /** Writes data to the socket, moving the data's pointer directly to the
  buffer.

  Once the data was written, `free` will be called to free the data's memory.

  Each call to a `write` function considers it's data atomic (a single package).

  The `urgent` varient will send the data as soon as possible, without
  distrupting
  any data packages (data written using `write` will not be interrupted in the
  middle).

  On error, returns -1. Returns 0 on success
  */
  ssize_t (*write_move_urgent)(server_pt srv,
                               int sockfd,
                               void* data,
                               size_t len);
  /**
  Sends a whole file as if it were a single atomic packet.

  Once the file was sent, the `FILE *` will be closed using `fclose`.

  The file will be buffered to the socket chunk by chunk, so that memory
  consumption is capped at ~ 64Kb.
  */
  ssize_t (*sendfile)(server_pt srv, int sockfd, FILE* file);

  /****************************************************************************
  * Tasks + Async
  */

  /**
  Schedules a specific task to run asyncronously for each connection.
  a NULL service identifier == all connections (all protocols).

  If a connection was terminated before performing their sceduled tasks, the
  `fallback` task will be performed instead.

  It is recommended to perform any resource cleanup within the fallback function
  and call the fallback function from within the main task, but other designes
  are valid as well.
  */
  int (*each)(struct Server* server,
              char* service,
              void (*task)(struct Server* server, int fd, void* arg),
              void* arg,
              void (*fallback)(struct Server* server, int fd, void* arg));
  /** Schedules a specific task to run for each connection. The tasks will be
   * performed sequencially, in a blocking manner. The method will only return
   * once all the tasks were completed. A NULL service identifier == all
   * connections (all protocols).
   *
   * The task, although performed <b>on</b> each connection, will be performed
   * within the calling connection's lock, so be careful as for possible race
   * conditions.
  */
  int (*each_block)(struct Server* server,
                    char* service,
                    void (*task)(struct Server* server, int fd, void* arg),
                    void* arg);
  /** Schedules a specific task to run asyncronously for a specific connection.

  returns -1 on failure, 0 on success (success being scheduling the task).

  If a connection was terminated before performing their sceduled tasks, the
  `fallback` task will be performed instead.

  It is recommended to perform any resource cleanup within the fallback function
  and call the fallback function from within the main task, but other designes
  are valid as well.
  */
  int (*fd_task)(struct Server* server,
                 int sockfd,
                 void (*task)(struct Server* server, int fd, void* arg),
                 void* arg,
                 void (*fallback)(struct Server* server, int fd, void* arg));
  /** Runs an asynchronous task, IF threading is enabled (set the
  `threads` to 1 (the default) or greater).

  If threading is disabled, the current thread will perform the task and return.

  Returns -1 on error or 0
  on succeess.
  */
  int (*run_async)(struct Server* self, void task(void*), void* arg);
  /** Creates a system timer (at the cost of 1 file descriptor) and pushes the
  timer to the reactor. The task will NOT repeat. Returns -1 on error or the
  new file descriptor on succeess.

  NOTICE: Do NOT create timers from within the on_close callback, as this might
  block resources from being properly freed (if the timer and the on_close
  object share the same fd number).
  */
  int (*run_after)(struct Server* self,
                   long milliseconds,
                   void task(void*),
                   void* arg);
  /** Creates a system timer (at the cost of 1 file descriptor) and pushes the
  timer to the reactor. The task will repeat `repetitions` times. if
  `repetitions` is set to 0, task will repeat forever. Returns -1 on error
  or the new file descriptor on succeess.

  NOTICE: Do NOT create timers from within the on_close callback, as this might
  block resources from being properly freed (if the timer and the on_close
  object share the same fd number).
  */
  int (*run_every)(struct Server* self,
                   long milliseconds,
                   int repetitions,
                   void task(void*),
                   void* arg);
} Server;

#endif
