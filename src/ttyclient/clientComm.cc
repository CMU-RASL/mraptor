/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.36 $  $Author: brennan $  $Date: 2006/11/16 22:49:43 $
 *  
 * AUTHOR: Trey Smith
 *
 * @file    clientComm.cc
 * @brief   No brief
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>

#include "clientComm.h"
#include "mrIPCHelper.h"
#include "mrCommonDefs.h"
#include "mrCommonTime.h"

using namespace std;
using namespace microraptor;

#define DEBUG_BO 0

/**********************************************************************
 * INTERFACING WITH THE ATACAMA TELEMETRY MANAGER
 **********************************************************************/

#ifdef MR_USE_ATACAMA_TELEMETRY_MANAGER

/** Modules can send out a RegisterMessage object in order to notify the
    telemetry manager to track the number of subscribers to the message
    and forward as necessary. */
struct RegisterMessage {
  char *message_name;
  char *message_format;
};

#define TM_REGISTER_MSG_NAME "tm_register_msg"
#define TM_REGISTER_MSG_FORMAT "{string, string}"

#define MR_SUBSCRIBE(n,f,h,c) atacama_tm_subscribe(n,f,h,c)
static void atacama_tm_subscribe(const string& message_name,
				 const string& message_format,
				 HANDLER_TYPE handler,
				 void *client_data)
{
#if 0
  cerr << "atacama subscribe: name=" << message_name
       << ", format=" << message_format << endl;
#endif

  // we need to subscribe to both the standard message name (as would be
  // sent by any module on our side of the telemetry link), and the
  // message name with "tm_" prepended (as would be sent by the telemetry
  // manager when it forwards a message from the other side).
  IPC_subscribe(message_name.c_str(), handler, client_data);
  string tm_name = "tm_" + message_name;
  IPC_subscribe(tm_name.c_str(), handler, client_data);

  // send a message to the telemetry manager, registering the new message
  // type and telling it to forward it as necessary.
  RegisterMessage rm;
  rm.message_name = const_cast<char*>(message_name.c_str());
  rm.message_format = const_cast<char*>(message_format.c_str());
  IPC_publishData(TM_REGISTER_MSG_NAME, &rm);
}

#else // ifdef MR_USE_ATACAMA_TELEMETRY_MANAGER

#define MR_SUBSCRIBE(n,f,h,c) IPC_subscribe(n,h,c)

#endif // ifdef MR_USE_ATACAMA_TELEMETRY_MANAGER

static void ipc_publish_charp(const string& message_name,
			      const char* message_text)
{
  // HACK IPC_publishData does not modify arg 2 so this cast is ok
  char *text_c = const_cast<char*>(message_text);
  IPC_publishData(message_name.c_str(), (void *) &text_c);
}

static void ipc_publish_string(const string& message_name,
			       const string& message_text)
{
  char *text_c = const_cast<char*>(message_text.c_str());
  IPC_publishData(message_name.c_str(), (void *) &text_c);
}

/**********************************************************************
 * CONNECTION RECORD
 **********************************************************************/

Connection_Record::Impl::Impl(const std::string& _module_name, MR_Comm& _parent) :
  module_name(_module_name),
  parent(_parent),
  out_buf_enabled(false),
  out_buf_bytes(0)
{
  out_buf_flush_time.tv_sec = 0;
  out_buf_flush_time.tv_usec = 0;
}

void Connection_Record::Impl::send_message(const string& text)
{
  //cout << "send_message: [" << text << "]" << endl;
  if (out_buf_enabled) {
    if (out_buf_bytes+text.size()+2 > MR_COMM_OUT_BUF_MAX) {
      flush_buffer( getTime() );
      if (text.size()+2 > MR_COMM_OUT_BUF_MAX) {
	if (parent.verbose_mode) { cout << "CONREC FLUSH SEND" << endl; }
        send_internal( text.c_str() );
	return;
      }
    }
    memcpy( out_buf+out_buf_bytes, text.c_str(), text.size() );
    out_buf_bytes += text.size()+1;
    out_buf[out_buf_bytes-1] = '\n';
    out_buf[out_buf_bytes] = '\0';
    maybe_flush_buffer( getTime() );
  } else {
    if (parent.verbose_mode) { cout << "CONREC SEND" << endl; }
    send_internal( text.c_str() );
  }
}

void Connection_Record::Impl::enable_output_buffering(void)
{
  out_buf_enabled = true;
}

void Connection_Record::Impl::maybe_flush_buffer(const timeval& cur_time)
{
  if (out_buf_enabled
      && out_buf_bytes > 0
      && timeDiff( cur_time, out_buf_flush_time ) > MR_COMM_FLUSH_INTERVAL) {
    flush_buffer(cur_time);
  }
}
  
void Connection_Record::Impl::flush_buffer(const timeval& cur_time)
{
  if (out_buf_bytes > 0) {
    //if (parent.verbose_mode) { cout << "FLUSH_BUFFER SEND" << endl; }
    send_internal( out_buf );
    out_buf_bytes = 0;
    out_buf_flush_time = cur_time;
  }
}

void Connection_Record::send_typed_message(const string& message_type,
					   const string& text)
{
  if (message_type == "") {
    impl->send_message(text);
  } else {
    impl->send_message(message_type + " " + text);
  }
}

/**********************************************************************
 * STREAM CONNECTION
 **********************************************************************/

struct Stream_Connection : public Connection_Record_Impl {
#if MR_COMM_ENABLE_FIFO
  Stream_Connection(const string& _module_name,
		    MR_Comm& _parent,
		    const string& _output_fifo_name,
		    const string& _input_fifo_name);
#endif
  Stream_Connection(const string& _module_name,
		    MR_Comm& _parent,
		    ostream* _output_stream,
		    istream* _input_stream,
		    int _input_fd);
  ~Stream_Connection(void);
  
  void send_internal(const char* text);

  // this function should only be called by IPC
  static void static_fd_handler(int fd, void* client_data);

protected:
  void init(void);
  void fd_handler(void);

  int input_fd;
  ostream* output_stream;
  istream* input_stream;
};

#if MR_COMM_ENABLE_FIFO

Stream_Connection::Stream_Connection(const string& _module_name,
				     MR_Comm& _parent,
				     const string& _output_fifo_name,
				     const string& _input_fifo_name) :
  Connection_Record_Impl(_module_name, _parent)
{
  output_stream = new ofstream(_output_fifo_name.c_str());
  if (! *output_stream) {
    cerr << "failed to open output fifo " << _output_fifo_name
	 << ": " << strerror(errno) << endl;
    exit(1);
  }
  
  ifstream *tmp = new ifstream(_input_fifo_name.c_str());
  input_fd = tmp->rdbuf()->fd();
  input_stream = tmp;
  if (! *input_stream) {
    cerr << "failed to open input fifo " << _input_fifo_name
	 << ": " << strerror(errno) << endl;
    exit(1);
  }

  init();
}

#endif

Stream_Connection::Stream_Connection(const string& _module_name,
				     MR_Comm& _parent,
				     ostream* _output_stream,
				     istream* _input_stream,
				     int _input_fd) :
  Connection_Record_Impl(_module_name, _parent),
  input_fd(_input_fd),
  output_stream(_output_stream),
  input_stream(_input_stream)
{
  init();
}

void Stream_Connection::init(void) {
  IPC_subscribeFD(input_fd, &static_fd_handler, this);
}

Stream_Connection::~Stream_Connection(void) {
  IPC_unsubscribeFD(input_fd, &static_fd_handler);
  delete output_stream;
  delete input_stream;
}

void Stream_Connection::send_internal(const char* text) 
{
  // If it's not a config string or we have verbose config
  //  turned on, print
  if (parent.verbose_mode &&
      (strncmp(text, "config", 6) || parent.verbose_config) ) {
    cout << "SENT(" << module_name << "): " << text << endl;
  }
  (*output_stream) << text << endl;
}

void Stream_Connection::static_fd_handler(int fd, void* client_data) {
  Stream_Connection* c = (Stream_Connection*) client_data;
  c->fd_handler();
}

// kludgy replacement for libstdc++ gets(), which seems to have gone away
void gets2(istream& in, char **buf) 
{
  char tbuf[1024];
  ostringstream accum;
  int n;

  while (1) {
    in.get(tbuf, sizeof(tbuf));
    n = strlen(tbuf);
    if (tbuf[n-1] == '\n') {
      tbuf[n-1] = '\0';
      accum << tbuf;
      break;
    }
    accum << tbuf;
  }
  *buf = strdup(accum.str().c_str());
}

void Stream_Connection::fd_handler(void)
{
  if (input_stream->eof()) {
    parent.remove_connection(module_name);
  }

  char *buf;
#ifndef LIBSTDC_HAS_GETS
  // gets2 is a replacement for gets
  gets2(*input_stream, &buf);
#else
  input_stream->gets(&buf);
#endif

  //if (parent.verbose_mode) { cout << "STREAM_CONNECTION HANDLER" << endl; }
  parent.call_message_handler(module_name, buf);
  delete buf;
}

/**********************************************************************
 * IPC CONNECTION
 **********************************************************************/

struct IPC_Connection : public Connection_Record_Impl {
  IPC_Connection(const string& _module_name,
		 MR_Comm& _parent);
  ~IPC_Connection(void);

  void send_internal(const char* text);
  bool is_ready_for_writing(void) const {
    assert(num_handlers == 0 || num_handlers == 1);
    return (num_handlers == 1);
  }

protected:
  void message_handler(char *text);
  void handler_change_handler(const char *message_name,
			      int num_handlers);
  static void static_handler_change_handler(const char *message_name,
					    int num_handlers,
					    void *client_data);
  static void static_message_handler(MSG_INSTANCE msgRef,
				     BYTE_ARRAY callData,
				     void *clientData);

  string input_message_name, output_message_name;
  int num_handlers;
};

IPC_Connection::IPC_Connection(const string& _module_name,
			       MR_Comm& _parent) :
  Connection_Record_Impl(_module_name, _parent)
{
  output_message_name =
    string("mr_") + parent.this_module_name + "_to_" + module_name;
  input_message_name =
    string("mr_") + module_name + "_to_" + parent.this_module_name;
    
  IPCHelper::defineMessage(output_message_name, "string");
  IPCHelper::defineMessage(input_message_name, "string");

  MR_SUBSCRIBE(input_message_name.c_str(), "string",
	       &IPC_Connection::static_message_handler,
	       (void *) this);

  // track the number of handlers to this message so we can tell
  // when the other module connects and disconnects.
#if DEBUG_BO
  cerr << "IPC_Connection constructor: sub to handler change handler"
       << ", module_name=" << module_name << endl;
#endif
  IPC_subscribeHandlerChange(output_message_name.c_str(),
			     &IPC_Connection::static_handler_change_handler,
			     this);
  num_handlers = IPC_numHandlers(output_message_name.c_str());
}

IPC_Connection::~IPC_Connection(void) 
{
  IPC_unsubscribeHandlerChange(output_message_name.c_str(),
			       &IPC_Connection::static_handler_change_handler);
  IPC_unsubscribe(input_message_name.c_str(),
		  &IPC_Connection::static_message_handler);
}

void IPC_Connection::static_message_handler(MSG_INSTANCE msgRef,
					    BYTE_ARRAY call_data,
					    void *client_data)
{
  IPC_Connection *c = (IPC_Connection *) client_data;

#ifdef MR_COMM_PROFILING
  timeval startTime = getTime();
#endif
  
  char **message_text;
  FORMATTER_PTR formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_unmarshall(formatter, call_data, (void **) &message_text);
  IPC_freeByteArray(call_data);
  //if (c->parent.verbose_mode) { cout << "IPC STATIC MSG HANDLER" << endl; }
  c->message_handler(*message_text);
  free(*message_text);
  free(message_text);

#ifdef MR_COMM_PROFILING
  double diff = timeDiff( getTime(), startTime );
  fprintf(stderr, "MRC: message handling completed in %d ms\n",
	  (int) (diff * 1.0e+3));
#endif
}

void IPC_Connection::message_handler(char *text)
{
  //if (parent.verbose_mode) { cout << "IPC_CONNECTION HANDLER" << endl; }
  parent.call_message_handler(module_name, text);
}

void IPC_Connection::send_internal(const char* text) 
{
  if (parent.verbose_mode &&
      (strncmp(text, "config", 6) || parent.verbose_config) ) {
    cout << "SENT(" << module_name << "): [" << text << "]" << endl;
  }
  ipc_publish_charp(output_message_name, text);
}
 
void IPC_Connection::handler_change_handler(const char *message_name_c,
					    int new_num_handlers)
{
#if DEBUG_BO
  cerr << "handler_change_handler: message_name=" << message_name_c
       << ", old#=" << num_handlers
       << ", new#=" << new_num_handlers << endl;
#endif

  // sanity check.  if we got notification, both new_num_handlers and
  // num_handlers should be either 0 or 1, and they should be different.
  assert(num_handlers == 0 || num_handlers == 1);
  assert(new_num_handlers == 0 || new_num_handlers == 1);

  // apparently if a module quickly subscribes and then exits, the central
  // server can send us a spurious change of the number of handlers from
  // 0 to 0.  just ignore it.
  //assert(num_handlers != new_num_handlers);
  if (num_handlers == new_num_handlers) {
    return;
  }

  num_handlers = new_num_handlers;

  if (num_handlers == 1) {
    // connection is now ready for writing, activate connect callback
    //cerr << "activating connect callback" << endl;

    // they sent a ping, we sent an ack, they subscribed to our message.
    // that means we were here first.
    parent.connection_order = MR_THIS_MODULE_BEFORE_OTHER_MODULE;
#if DEBUG_BO
    cerr << "calling connect handler " << module_name << " , yay!" << endl;
#endif

    parent.call_connect_handler(module_name);
  } else {
    // doh, the other module disconnected
    //cerr << "activating disconnect callback" << endl;
    parent.remove_connection(module_name);
  }
}

void IPC_Connection::static_handler_change_handler(const char *message_name_c,
						   int num_handlers,
						   void *client_data)
{
  IPC_Connection *c = (IPC_Connection*) client_data;
  c->handler_change_handler(message_name_c, num_handlers);
}

/**********************************************************************
 * MAIN MR_COMM DATA STRUCTURE
 **********************************************************************/
 
MR_Comm::MR_Comm(const string& _this_module_type,
		 const string& _short_host_name) :
  message_handler(NULL),
  connect_handler(NULL),
  disconnect_handler(NULL),
  verbose_mode(false),
  verbose_config(false),
  this_module_type(_this_module_type),
  short_host_name(_short_host_name),
  do_handle_config_sync(false),
  have_local_config(false)
{
  if ("" == short_host_name) {
    short_host_name = IPCHelper::getshorthostname();
  }

  if (this_module_type == "c") {
    string user = getenv("USER");
    if(user.size() <= 0) {
      user = "stealthed";
    }
    short_host_name = user + "@" + short_host_name;
  }

  this_module_name = this_module_type + "_" + short_host_name
    + "_" + rcl::toString(getpid());

  IPC_initialize();
}

// tells MR_Comm that we are ready to connect to central and start
// listening for other modules.
void MR_Comm::initialize(void) {
#ifdef MR_USE_ATACAMA_TELEMETRY_MANAGER
  IPCHelper::defineMessage(TM_REGISTER_MSG_NAME, TM_REGISTER_MSG_FORMAT);
#endif

  IPCHelper::defineMessage("mr_search_ping", "string");
  IPCHelper::defineMessage("mr_search_ack", "string");

  MR_SUBSCRIBE("mr_search_ping", "string", &MR_Comm::static_ping_handler, this);
  MR_SUBSCRIBE("mr_search_ack", "string", &MR_Comm::static_ack_handler, this);

  ipc_publish_string("mr_search_ping", this_module_name);
}

// tells MR_Comm that we are ready to connect to central and start
// listening for other modules.
void MR_Comm::connect_to_central(void (*callback)(string, bool)) {
  IPCHelper::reliableConnect(this_module_name, callback);
  initialize();
}

void MR_Comm::send_message(const string& module_name,
			   const string& text)
{
  if (module_name == MR_NO_MODULES) {
    return;
  } else if (module_name == MR_ALL_MODULES) {
    send_message_all(text);
    return;
  }

  if (connections.find(module_name) == connections.end()) {
    throw MRException
      (string("trying to send message to unknown module ")
       + module_name);
  }
  connections[module_name].send_message(text);
}

// switch to block buffering for the given module; block buffering is
//   more efficient but will break older clients
void MR_Comm::enable_output_buffering(const std::string& module_name)
{
  if (connections.find(module_name) == connections.end()) {
    throw MRException
      (string("trying to set buffer block for unknown module ")
       + module_name);
  }
  connections[module_name].enable_output_buffering();
}

void MR_Comm::maybe_flush_output_buffers(void)
{
  timeval cur_time = getTime();
  FOR_EACH ( pr, connections ) {
    pr->second.maybe_flush_buffer(cur_time);
  }
}

// install a callback for messages from other modules.
void MR_Comm::subscribe_messages(Response_Handler _message_handler,
				 void *_message_client_data)
{
  message_handler = _message_handler;
  message_client_data = _message_client_data;
}

// we automatically connect to any modules of a different type that
// central knows about.  use these functions to take actions based on
// new connections.
void MR_Comm::subscribe_connect(Connect_Handler _connect_handler,
				void *_connect_client_data)
{
  connect_handler = _connect_handler;
  connect_client_data = _connect_client_data;
}

void MR_Comm::subscribe_disconnect(Connect_Handler _disconnect_handler,
				   void *_disconnect_client_data)
{
  disconnect_handler = _disconnect_handler;
  disconnect_client_data = _disconnect_client_data;
}

// these are used on the daemon side to handle subscriptions,
// not currently used by clients.
void MR_Comm::send_typed_message(const string& module_name,
				 const string& message_type,
				 const string& text)
{
  //cout << "SEND TYPED MESSAGE MRCOM" << endl;
  if (module_name == MR_NO_MODULES) {
    return;
  } else if (module_name == MR_ALL_MODULES) {
    send_message_all(message_type, text);
  } else if (module_name == MR_SUBSCRIBED_MODULES) {
    send_message_to_subscribers(message_type, text);
  } else {
    send_message(module_name, message_type + " " + text);
  }
}

void MR_Comm::send_message_all(const string& text)
{
  FOR_EACH(pr,connections) {
    pr->second.send_message(text);
  }
}

std::string MR_Comm::get_module_of_type(const std::string& type, bool suppressWarning) {
  int count = 0;
  string moduleName;
  FOR_EACH ( pr, connections ) {
    string moduleType = extract_type_from_module_name(pr->first);
    if (moduleType == type) {
      if (count == 0) {
	moduleName = pr->first;
      }
      count++;
    }
  }
  if (0 == count) {
    throw MRException(string("MR_Comm::get_module_of_type(\"")
		       + type
		       + "\"): no modules of desired type have connected yet");
  }
  if (count > 1 && !suppressWarning) {
    cerr << "WARNING: MR_Comm::get_module_of_type(\"" << type
	 << "\"): more than 1 module of desired type has connected; "
	 << "arbitrarily using the first one" << endl;
  }
  return moduleName;
}

void MR_Comm::send_message_to_subscribers(const string& message_type,
					  const string& text)
{
  //cout << "SUBSCRIBER SEND MRCOM" << endl;
  if (subscribers.find(message_type) != subscribers.end()) {
    Sub_List& sub_list = subscribers[message_type];
    FOR_EACH(subscriber,sub_list) {
      subscriber->second.send_typed_message(message_type, text);
    }
  }
  // (ok if we didn't find the given message type in the table of
  //  messages with subscribers... that just means that there are no
  //  subscribers)
}

void MR_Comm::add_subscriber(const string& message_type,
			     const string& module_name)
{
  if (connections.find(module_name) == connections.end()) {
    throw MRException
      (string("trying to add a subscription for unknown module ")
       + module_name);
  }
  Connection_Record& conn = connections[module_name];

  if (subscribers.find(message_type) == subscribers.end()) {
    subscribers[message_type] = Sub_List();
  }
  Sub_List& sub_list = subscribers[message_type];

  if (sub_list.find(module_name) != sub_list.end()) {
    throw MRException
      (string("trying to subscribe to message type ") + message_type
       + ", but already subscribed",
       MR_WARNING);
  }
  sub_list[module_name] = conn;
}

void MR_Comm::remove_subscriber(const string& message_type,
				const string& module_name,
				bool warn_if_not_subscribed)
{
  if (subscribers.find(message_type) == subscribers.end()) {
    if (warn_if_not_subscribed) {
      throw MRException
	(string("trying to remove subscription from ")
	 + message_type + ", but nobody is currently subscribed",
	 MR_WARNING);
    } else {
      return;
    }
  }
  Sub_List& sub_list = subscribers[message_type];
  if (sub_list.find(module_name) == sub_list.end()) {
    if (warn_if_not_subscribed) {
      throw MRException
	(string("trying to remove subscription from ")
	 + message_type + ", but not currently subscribed",
	 MR_WARNING);
    } else {
      return;
    }
  }
  sub_list.erase(module_name);
}

#if MR_COMM_ENABLE_FIFO
// for FIFO connections we can't autodetect other modules, so
// connect_stream actively attempts to connect to a specific module.
void MR_Comm::connect_client_fifo(const string &module_name,
				  const string &fifo_name_stem)
{
  connection_order = MR_ACTIVE_LINK;
  add_connection
    (module_name,
     new Stream_Connection(module_name, *this,
			   fifo_name_stem + ".command",
			   fifo_name_stem + ".response"));
}
#endif

void MR_Comm::connect_daemon_stdio(const string &module_name)
{
  connection_order = MR_ACTIVE_LINK;
  add_connection
    (module_name,
     new Stream_Connection(module_name, *this,
			   &cout, &cin,
			   fileno(stdin)));
}

bool MR_Comm::should_connect(const string& other_module_name,
			     const string& this_module_name)
{
  // don't connect to yourself :)
  if (other_module_name == this_module_name) return false;

#if 0
  string other_module_type = other_module_name.substr(0,1);
  string this_module_type = this_module_name.substr(0,1);

  // of course, daemons connect to clients and vice versa
  if (other_module_type != this_module_type) return true;

  // new: daemons connect to other daemons
  if (this_module_type == "d") return true;

  return false;
#endif
  
  // new behavior: all modules connect to all other modules.
  // this is needed so that all of the configs get synced properly.
  return true;
}

void MR_Comm::ping_handler(char *module_name_c) {
  string module_name = module_name_c;
#if DEBUG_BO
  cerr << "got ping: " << module_name << endl;
#endif
  if (should_connect(module_name, this_module_name)) {
#if DEBUG_BO
    cerr << "sending ack" << endl;
#endif
    // we received the ping, therefore we were here first
    connection_order = MR_THIS_MODULE_BEFORE_OTHER_MODULE;
    try_add(module_name);
    // Moved this to after adding the module to try to compensate for 
    // IPC race condition (in IPC 3.10.x) Reid, 8/5/2015
    ipc_publish_string("mr_search_ack", this_module_name);
  }
}

void MR_Comm::ack_handler(char *module_name_c)
{
  //cerr << "got ack: " << module_name_c << endl;

  // we sent the ping and got the ack, therefore they were here before us
  connection_order = MR_OTHER_MODULE_BEFORE_THIS_MODULE;
  try_add(module_name_c);
}

void MR_Comm::static_ping_handler(MSG_INSTANCE msg_ref, BYTE_ARRAY call_data,
				  void *client_data)
{
  MR_Comm* c = (MR_Comm*) client_data;
  
  FORMATTER_PTR formatter = IPC_msgInstanceFormatter(msg_ref);
  char **msg_data;
  IPC_unmarshall(formatter, call_data, (void **) &msg_data);
  IPC_freeByteArray(call_data);

  c->ping_handler(*msg_data);

  IPC_freeData(formatter, (void *) msg_data);
}

void MR_Comm::static_ack_handler(MSG_INSTANCE msg_ref, BYTE_ARRAY call_data,
				 void *client_data)
{
  MR_Comm* c = (MR_Comm*) client_data;
  
  FORMATTER_PTR formatter = IPC_msgInstanceFormatter(msg_ref);
  char **msg_data;
  IPC_unmarshall(formatter, call_data, (void **) &msg_data);
  IPC_freeByteArray(call_data);

  c->ack_handler(*msg_data);

  IPC_freeData(formatter, (void *) msg_data);
}

void MR_Comm::try_add(const string& module_name) {
  // add it if it's the right type and we don't know about it already
  if (should_connect(module_name, this_module_name)
      && connections.find(module_name) == connections.end())
    {
#if DEBUG_BO
      cerr << "try_add: adding connection for " << module_name << endl;
#endif
      add_connection(module_name, new IPC_Connection(module_name, *this));
    }
}

void MR_Comm::add_connection(const string& module_name,
			     const Connection_Record& conn)
{
  connections[module_name] = conn;
  if (conn.is_ready_for_writing()) {
    call_connect_handler(module_name);
  }
}

void MR_Comm::remove_connection(const Connection_Record& conn)
{
  call_disconnect_handler(conn.impl->module_name);

  // remove conn from subscribers lists
  FOR_EACH(pair, subscribers) {
    remove_subscriber(pair->first, conn.impl->module_name,
		      /* warn_if_not_subscribed = */ false);
  }

  // remove conn from connections list
  connections.erase(conn.impl->module_name);
}

void MR_Comm::remove_connection(const string& module_name) {
  if (connections.find(module_name) == connections.end()) {
    throw MRException
      (string("trying to remove connection to unknown module ")
       + module_name);
  }
  remove_connection(connections[module_name]);
}

string MR_Comm::extract_host_from_module_name(const string& module_name) {
  string after_type = module_name.substr(2);
  size_t pos;
  if (string::npos == (pos = after_type.find("_"))) {
    throw MRException
      (string("couldn't extract host from module name ") + module_name);
  }
  return after_type.substr(0,pos);
}

void MR_Comm::send_config_module(const string& peer_name,
				 const rcl::exp& config,
				 bool is_prior)
{
  //cout << "SEND CONFIG MOD MRCOM" << endl;
  string peer_type = extract_type_from_module_name(peer_name);
  string prior_string = (is_prior ? " prior" : "");
  if (peer_type == "d") {
    send_typed_message(peer_name, "set config",
		       rcl::toString(config) + prior_string);
  } else {
    send_typed_message(peer_name, "config",
		       rcl::toString(config) + prior_string);
  }
}

void MR_Comm::handle_config_sync(FSlot1<rcl::exp&> _get_config_handler,
				 bool _have_local_config)
{
  get_config_handler = _get_config_handler;
  do_handle_config_sync = true;
  have_local_config = _have_local_config;
}

// to be used only when the user changes a config in the middle
//    of running.
void MR_Comm::broadcast_config(const rcl::exp& config) {
  FOR_EACH(pr, connections) {
    send_config_module(pr->first, config, false);
  }
}

void MR_Comm::call_connect_handler(const string& module_name)
{
  // in order to keep things synced, always send our config to
  //   new modules as they connect
  if (do_handle_config_sync
      && module_name != "stdio"
      && (have_local_config
	  || connection_order == MR_THIS_MODULE_BEFORE_OTHER_MODULE))
    {
      rcl::exp config;
      get_config_handler.callHandler(config);
      if (config.defined()) {
	send_config_module(module_name, config,
			   /* is_prior = */
			   (connection_order
			    == MR_THIS_MODULE_BEFORE_OTHER_MODULE));
      }
    }
  if (NULL != connect_handler) {
    (*connect_handler)(module_name, connect_client_data);
  }
}

void MR_Comm::call_disconnect_handler(const string& module_name) {
  if (NULL != disconnect_handler) {
    (*disconnect_handler)(module_name, disconnect_client_data);
  }
}

void MR_Comm::call_message_handler(const string& module_name,
			  const string& message_text)
{
  if (NULL != message_handler) {
    if (verbose_mode &&
	(strncmp(message_text.c_str(), "config", 6) || verbose_config) ) {
      cout << "RECEIVED(" << module_name << "): " << message_text << endl;
    }
    (*message_handler)(module_name, message_text, message_client_data);
  }
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: clientComm.cc,v $
 * Revision 1.36  2006/11/16 22:49:43  brennan
 * Added option to get_module_of_type to suppress warning message about
 * multiple modules.
 *
 * Revision 1.35  2006/11/16 18:57:24  brennan
 * Added support for callbacks during connection to central.
 *
 * Revision 1.34  2005/05/24 23:04:21  trey
 * fixed uninitialized variable that was causing badly formed outgoing messages and resulting claw crashes
 *
 * Revision 1.33  2005/05/18 01:08:28  trey
 * switched out ostringstream class, now use more efficient homemade buffering
 *
 * Revision 1.32  2005/05/17 22:11:56  trey
 * added output buffering support
 *
 * Revision 1.31  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.30  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.29  2004/04/27 23:42:15  trey
 * added get_module_of_type() function, to make it easier to write simple clients
 *
 * Revision 1.28  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.27  2004/03/10 20:23:12  trey
 * added initialize() function which is separate from connecting to IPC
 *
 * Revision 1.26  2004/01/08 13:50:36  brennan
 * Added option to not print configs in verbose mode
 *
 * Revision 1.25  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.24  2003/10/07 17:55:03  brennan
 * Fixed cross-daemon dependency issue (one daemon not knowing about the other);
 * dependency loop detection still broken.
 *
 * Revision 1.23  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.22  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.21  2003/06/14 22:32:34  trey
 * added verbose mode for debugging, and fixed problem with sending empty config expressions while trying to sync
 *
 * Revision 1.20  2003/06/06 16:12:19  trey
 * fixed flaky connection order system; config is now not sent automatically to stdio
 *
 * Revision 1.19  2003/06/06 01:46:20  trey
 * added support for syncing config between daemons
 *
 * Revision 1.18  2003/03/22 17:17:26  trey
 * removed some debugging messages
 *
 * Revision 1.17  2003/03/22 17:04:25  trey
 * added ability to use the atacama telemetry manager, you can disable with gmake NO_TELEMETRY_MANAGER=1 install
 *
 * Revision 1.16  2003/03/17 02:37:17  trey
 * added ability to do message handler time profiling when you compile with MR_COMM_PROFILING=1
 *
 * Revision 1.15  2003/03/13 15:51:20  trey
 * added extract_host_from_module_name function
 *
 * Revision 1.14  2003/03/12 15:57:07  trey
 * MR_Comm now handles connecting to central.  this allows mraptord to operate disconnected until after it runs central.
 *
 * Revision 1.13  2003/03/11 01:50:56  trey
 * allowed stream connection to handle arbitrary-length lines
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
 * Revision 1.7  2003/02/25 21:44:16  trey
 * added check to avoid multiple subscriptions to same message bug
 *
 * Revision 1.6  2003/02/19 20:06:39  trey
 * fixed fifo disconnect behavior
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
