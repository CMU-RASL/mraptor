/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.35 $  $Author: brennan $  $Date: 2006/11/16 22:49:43 $
 *  
 * AUTHOR: Trey Smith
 *
 * @file    clientComm.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCclientComm_h
#define INCclientComm_h

#include <fstream>

#include "ipc.h"

// Really a gcc-3.2 thing...
#include "mrHashMap.h"
#include "RCL.h"
#include "mrIPCMessage.h"
#include "mrFSlot.h"

#ifdef REDHAT6_2
#include <sys/time.h>
#endif

namespace microraptor {

typedef void (*Response_Handler)(const std::string& which_daemon,
				 const std::string& message,
				 void *client_data);
typedef void (*Connect_Handler)(const std::string& which_daemon,
				void *client_data);

#define MR_ALL_MODULES        ("<all>")
#define MR_SUBSCRIBED_MODULES ("<subscribed>")
#define MR_NO_MODULES         ("<none>")

#define MR_COMM_OUT_BUF_MAX (16384)
#define MR_COMM_FLUSH_INTERVAL (0.1)

enum Connection_Order {
  // i think these are self-explanatory :)
  MR_OTHER_MODULE_BEFORE_THIS_MODULE,
  MR_THIS_MODULE_BEFORE_OTHER_MODULE,
  // this value is sent when we connect via FIFO, can't tell who was
  //   here first.
  MR_ACTIVE_LINK
};

struct MR_Comm;

struct Connection_Record {
  struct Impl : public FSlotObject {
    Impl(const std::string& _module_name, MR_Comm& _parent);
    virtual ~Impl(void) {}

    // if buffering, adds line to message buffer and flushes if needed,
    //   otherwise sends the message immediately
    void send_message(const std::string& text);
    // turns buffering on
    void enable_output_buffering(void);
    // flushes the buffer if needed
    void maybe_flush_buffer(const timeval& cur_time);
    // flushes the buffer
    void flush_buffer(const timeval& cur_time);

    virtual bool is_ready_for_writing(void) const { return true; }
    std::string module_name;
    MR_Comm& parent;
    bool out_buf_enabled;
    char out_buf[MR_COMM_OUT_BUF_MAX];
    size_t out_buf_bytes;
    timeval out_buf_flush_time;

  protected:
    virtual void send_internal(const char* text) = 0;
  };
  SmartRef< Impl > impl;

  Connection_Record(void) {}
  Connection_Record(Impl *_impl) : impl(_impl) {}
  bool operator==(const Connection_Record& rhs) {
    return impl.pointer() == rhs.impl.pointer();
  }
  void send_message(const std::string& text) { impl->send_message(text); }
  void enable_output_buffering(void) { impl->enable_output_buffering(); }
  void maybe_flush_buffer(const timeval& cur_time) { impl->maybe_flush_buffer(cur_time); }
  void send_typed_message(const std::string& message_type,
			  const std::string& text);
  bool is_ready_for_writing(void) const {
    return impl->is_ready_for_writing();
  }
};

// for convenience, avoids annoying emacs indentation
typedef Connection_Record::Impl Connection_Record_Impl;

// hash of <other_module_name> to connection record
typedef const hash_map< std::string, Connection_Record>& Connection_Hash;

#ifndef ENABLE_FIFO
// set to 1 to allow fifo connections to be made, not implemented
// yet under gcc 3.2
#  define MR_COMM_ENABLE_FIFO 0
#else
#  define MR_COMM_ENABLE_FIFO 1
#endif

// handles automatic detection of daemons/clients entering and leaving
// the IPC network
//
// FIX: don't let an MR_Comm object go out of scope!  destructors are not
// set up properly!
struct MR_Comm : public FSlotObject {
  MR_Comm(const std::string& _this_module_type,
	  const std::string& _short_host_name = "");

  void set_verbose_mode(bool _verbose_mode, bool _verbose_config = true) {
    verbose_mode = _verbose_mode;
    verbose_config = _verbose_config;
  }

  std::string get_this_host_name(void) { return short_host_name; }
  std::string get_this_module_name(void) { return this_module_name; }

  // tells MR_Comm that we are ready to connect to central and start
  // listening for other modules.  See IPCHelper::reliableConnect for
  // callback semantics.
  void connect_to_central(void (*callback)(std::string, bool) = NULL);

  // call this instead of connect_to_central() if you are already connected
  void initialize(void);

  // gets the name of a module of the specified type.  exits with an
  // error if no modules of the specified type have connected.  if
  // more than one module of the specified type is connected, prints a
  // warning message, if suppressWarning is false.
  // available types are "d" (daemon) or "c" (client).
  std::string get_module_of_type(const std::string& type, bool suppressWarning = false);

  void send_message(const std::string& module_name,
		    const std::string& text);

  // switch to block buffering for the given module; block buffering is
  //   more efficient but will break older clients
  void enable_output_buffering(const std::string& module_name);

  // flush output buffers if needed
  void maybe_flush_output_buffers(void);

  // install a callback for messages from other modules.
  void subscribe_messages(Response_Handler _message_handler,
			  void *_message_client_data);

  // we automatically connect to any modules of a different type that
  // central knows about.  use these functions to take actions based on
  // new connections.
  void subscribe_connect(Connect_Handler _connect_handler,
			 void *_connect_client_data);
  void subscribe_disconnect(Connect_Handler _disconnect_handler,
			    void *_disconnect_client_data);

  // retrieves the current set of connections.
  Connection_Hash get_connections(void) { return connections; }

  bool is_connected_to_module(const std::string& module_name) {
    return connections.find(module_name) != connections.end();
  }

  // you *could* use this to explicitly disconnect from a module, but
  // normally it's called automatically when we detect that another
  // module has shut down.
  void remove_connection(const std::string& module_name);

  // these are used on the daemon side to handle subscriptions,
  // not currently used by clients.
  void send_typed_message(const std::string& module_name,
			  const std::string& message_type,
			  const std::string& text);
  void send_message_all(const std::string& message_type,
			const std::string& text)
  {
    send_message_all(message_type + " " + text);
  }
  void send_message_all(const std::string& text);
  void send_message_to_subscribers(const std::string& message_type,
				   const std::string& text);
  void add_subscriber(const std::string& message_type,
		      const std::string& module_name);
  void remove_subscriber(const std::string& message_type,
			 const std::string& module_name,
			 bool warn_if_not_subscribed = true);

#if MR_COMM_ENABLE_FIFO
  // for FIFO connections we can't autodetect other modules, so
  // connect_stream actively attempts to connect to a specific module.
  void connect_client_fifo(const std::string &module_name,
			   const std::string &fifo_name_stem);
#endif
  void connect_daemon_stdio(const std::string &module_name);

  // returns the order in which connections were made.  this function's
  // return value is only defined when you are inside a connect handler.
  // use it to tell whether this module connected first, or the module
  // at the other end of the link.
  Connection_Order get_connection_order(void) { return connection_order; }

  static std::string extract_type_from_module_name(const std::string& module_name) {
    return module_name.substr(0,1);
  }
  static std::string extract_host_from_module_name(const std::string& module_name);

  // to be used only when the user changes a config in the middle
  //    of running.  the right behavior in that case is to forward
  //    the new config to everyone currently connected.  (new peers
  //    connecting later will automatically pick up whatever exists
  //    when they connect).
  void broadcast_config(const rcl::exp& config);

  // call this before connect_to_central().  get_config_handler
  //   should be a slot pointing to a function get_config(RCLExp& config)
  //   that sets the value of config based on the current config
  //   when it is called.  have_local_config should be true if
  //   we are loading a local config file (that should be forwarded
  //   to peers).
  void handle_config_sync(FSlot1<rcl::exp&> _get_config_handler,
			  bool _have_local_config);

  // these should only be used by Connection_Record implementations
  Response_Handler message_handler;
  Connect_Handler connect_handler, disconnect_handler;
  void *message_client_data, *connect_client_data, *disconnect_client_data;
  std::string this_module_name;
  Connection_Order connection_order;
  bool verbose_mode;
  bool verbose_config; // Should config messages be printed in verbose mode?

  void call_connect_handler(const std::string& module_name);
  void call_disconnect_handler(const std::string& module_name);
  void call_message_handler(const std::string& module_name,
			    const std::string& message_text);
protected:
  void ping_handler(char *module_type);
  void ack_handler(char *module_name);

  static void static_ping_handler(MSG_INSTANCE msg_ref, BYTE_ARRAY call_data,
				  void *client_data);
  static void static_ack_handler(MSG_INSTANCE msg_ref, BYTE_ARRAY call_data,
				 void *client_data);
  
  void try_add(const std::string& module_name);
  void add_connection(const std::string& module_name,
		      const Connection_Record& conn);
  void remove_connection(const Connection_Record& conn);
  bool should_connect(const std::string& other_module_name,
		      const std::string& this_module_name);

  void send_config_module(const std::string& peer_name, const rcl::exp& config,
			  bool is_prior);

  std::string this_module_type; 

  hash_map< std::string, Connection_Record > connections;
  typedef hash_map< std::string, Connection_Record > Sub_List;
  hash_map< std::string, Sub_List > subscribers;
  std::string short_host_name;

  bool do_handle_config_sync;
  bool have_local_config;
  FSlot1<rcl::exp&> get_config_handler;
};

} // namespace microraptor

#endif // INCclientComm_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: clientComm.h,v $
 * Revision 1.35  2006/11/16 22:49:43  brennan
 * Added option to get_module_of_type to suppress warning message about
 * multiple modules.
 *
 * Revision 1.34  2006/11/16 18:57:24  brennan
 * Added support for callbacks during connection to central.
 *
 * Revision 1.33  2005/07/08 02:57:05  brennan
 * Added #include <sys/time.h> for RH6.2 compilation.
 *
 * Revision 1.32  2005/05/18 01:08:28  trey
 * switched out ostringstream class, now use more efficient homemade buffering
 *
 * Revision 1.31  2005/05/17 22:11:56  trey
 * added output buffering support
 *
 * Revision 1.30  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.29  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.28  2004/04/27 23:42:15  trey
 * added get_module_of_type() function, to make it easier to write simple clients
 *
 * Revision 1.27  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.26  2004/03/10 20:23:12  trey
 * added initialize() function which is separate from connecting to IPC
 *
 * Revision 1.25  2004/01/08 13:50:36  brennan
 * Added option to not print configs in verbose mode
 *
 * Revision 1.24  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.23  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.22  2003/08/19 16:03:15  brennan
 * Conflict resolution.
 *
 * Revision 1.21  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.20  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.19  2003/06/14 22:31:49  trey
 * added verbose mode for debugging
 *
 * Revision 1.18  2003/06/06 16:12:19  trey
 * fixed flaky connection order system; config is now not sent automatically to stdio
 *
 * Revision 1.17  2003/06/06 01:46:20  trey
 * added support for syncing config between daemons
 *
 * Revision 1.16  2003/03/22 17:04:26  trey
 * added ability to use the atacama telemetry manager, you can disable with gmake NO_TELEMETRY_MANAGER=1 install
 *
 * Revision 1.15  2003/03/17 02:37:17  trey
 * added ability to do message handler time profiling when you compile with MR_COMM_PROFILING=1
 *
 * Revision 1.14  2003/03/13 15:51:20  trey
 * added extract_host_from_module_name function
 *
 * Revision 1.13  2003/03/12 15:57:07  trey
 * MR_Comm now handles connecting to central.  this allows mraptord to operate disconnected until after it runs central.
 *
 * Revision 1.12  2003/03/10 22:55:22  trey
 * added dependency checking
 *
 * Revision 1.11  2003/03/10 05:35:11  trey
 * added get_clock command and ability of daemons to connect to each other
 *
 * Revision 1.10  2003/03/07 00:35:19  trey
 * added function to determine connection order
 *
 * Revision 1.9  2003/02/26 05:10:26  trey
 * made the module_name argument to the send_message functions more flexible.  now you can specify MR_ALL_MODULES or MR_SUBSCRIBED_MODULES
 *
 * Revision 1.8  2003/02/26 03:19:25  trey
 * fixed problem of core dump when client subscribes, unsubscribes, then quits
 *
 * Revision 1.7  2003/02/23 00:27:01  trey
 * added is_connected_to_module
 *
 * Revision 1.6  2003/02/20 14:28:59  trey
 * added get_connections
 *
 * Revision 1.5  2003/02/19 17:33:41  trey
 * added proper disconnect handling
 *
 * Revision 1.4  2003/02/19 10:19:17  trey
 * made improved clientComm
 *
 * Revision 1.3  2003/02/19 00:46:05  brennan
 * Changed clientComm to keep track of daemon name.
 *
 * Revision 1.2  2003/02/17 16:22:10  trey
 * implemented IPC client connection
 *
 * Revision 1.1  2003/02/16 18:09:19  trey
 * made comm abstraction layer for clients
 *
 *
 ***************************************************************************/
