/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.13 $  $Author: brennan $  $Date: 2006/12/20 19:06:54 $
 *  
 * @file    stdoutBuffer.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCstdoutBuffer_h
#define INCstdoutBuffer_h

#include <stdio.h>
#ifdef CYGWIN_HEADERS
#include <sys/time.h> // For timeval
#include <time.h>     // For timeval
#include <asm/socket.h> // For FIONREAD
#endif


#define MR_LINE_LENGTH (80)

struct Line_Data {
  timeval time_stamp;
  // line types are:
  //   x: ended by a CRLF pair (\r\n), LF (\n) character, or EOF
  //   c: ended due to length exceeding MR_LINE_LENGTH
  //   r: ended by a CR (\r) character
  char line_type;
  char data[MR_LINE_LENGTH+1];
};

enum Buffer_Status {
  BUFFER_NO_DATA,
  BUFFER_UNFINISHED_LINE,
  BUFFER_GOT_LINE
};

struct Byte_Buffer {
  Byte_Buffer(int _fd, int _size = BUFSIZ);
  ~Byte_Buffer(void);

  // inform the byte buffer that future getline() calls should only look
  // at the buffer (but not attempt to read data from the fd)
  void release_fd(void);

  // point the byte buffer at a new fd (used when restarting a process)
  void reopen(int _new_fd);

  // line_buf must point to a buffer with length at least max_bytes.
  // 
  // -- Case 1
  // If there is an EOL (ASCII CR, LF or CRLF) character in the first
  // max_bytes-1 bytes, copy the characters up to the EOL into line_buf.
  // The EOL character is removed from the byte buffer but not copied
  // into line_buf.  The resulting string is NULL-terminated.
  // line_type is set to be 'r' (CR) or 'x' (LF or CRLF).  return true.
  //
  // -- Case 2
  // Else if there are any bytes in the buffer, copy as many bytes
  // as are available and fit into line_buf and NULL-terminate the
  // resulting string.  line_type is set to be 'c'.  return true.
  //
  // -- Case 3
  // Return false.
  int getline(Line_Data *ld, bool cleanup = false);

  int get_fd(void) { return fd; }

protected:
  char *buf, *start_pos, *end_pos;
  int fd, size;
  bool at_eof;

  // attempts to read as many bytes as possible from the file
  // represented by fd.  returns the number of bytes read.
  int _read_bytes(void);

  // returns a pointer to the free space after the end of the data
  // currently in the buffer, and sets free_bytes to be the number of free
  // bytes available.
  char *_get_data_pointer(int* free_bytes);

  // after you read in data to the area returned by _get_data_pointer(),
  // use this to say how many bytes you read in.  it advances the end of
  // data pointer.
  void _mark_data_read(int num_bytes);

  // moves the location of data so that startPos is the beginning of
  // the buffer, and all free space is at the end.
  void _defrag_buffer(void);
};

struct Line_Buffer {
  Line_Buffer(int _fd, int _num_lines);
  ~Line_Buffer(void);

  // inform the line buffer that future getline() calls should only look
  // at the buffer (but not attempt to read data from the fd)
  void release_fd(void);

  // point the byte buffer at a new fd (used when restarting a process)
  void reopen(int _new_fd);

  // return the number of stored lines in the buffer
  int get_num_stored_lines(void) { return num_stored_lines; }

  // go back and return the stored line which was read in offset lines
  // ago.  offset must be between 1 and get_num_stored_lines()
  Line_Data *get_stored_line(int offset) {
    assert(1 <= offset && offset <= num_stored_lines);
    return &lbuf[_normalize(end_line - offset)];
  }

  // if there is an unread line in the buffer, return the corresponding
  // a pointer to the corresponding Line_Data (this is a pointer into
  // the buffer; do not try to delete it).  otherwise returns NULL.
  int getline(Line_Data** ldata, bool cleanup = false);

  // Push a line onto the end of the buffer.  This bypasses the
  // Byte_Buffer, and lines thus pushed will *not* be returned by
  // getline().  If line is longer than MR_LINE_LENGTH, it will be
  // truncated, unless line_type is '?', in which case the line will
  // be split.
  void pushline(const char* line, char line_type = '?');

  int get_fd(void) { return bbuf.get_fd(); }

protected:
  Byte_Buffer bbuf;
  int num_lines;

  Line_Data *lbuf;
  int end_line; // index into the lbuf array
  int num_stored_lines;

  // total lines read since the Line_Buffer was created (older lines
  // may have been silently overwritten due to buffer space limitations)
  int num_lines_read;

  // do the right modulo arithmetic on a line number
  int _normalize(int line) { return (line + num_lines) % num_lines; }
};

#endif // INCstdoutBuffer_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: stdoutBuffer.h,v $
 * Revision 1.13  2006/12/20 19:06:54  brennan
 * 'MRAPTORD:' outputs are now pushed into the relevant line buffers,
 * so that they're returned when a playback is requested.
 *
 * Revision 1.12  2005/06/24 18:00:10  trey
 * added release_fd() function
 *
 * Revision 1.11  2004/08/02 18:57:44  trey
 * sigh... tried to fix stderr output again
 *
 * Revision 1.9  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.8  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.7  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.6  2003/03/12 15:52:32  trey
 * changed behavior to immediately output partial lines with c line code.  this way the central prompt shows up properly.  may change it back later if it causes problems.
 *
 * Revision 1.5  2003/03/03 17:21:03  trey
 * removed eof() since i am not sure it works, fixed a couple of obscure corner cases
 *
 * Revision 1.4  2003/02/26 05:09:23  trey
 * implemented chris's suggestion for \r handling
 *
 * Revision 1.3  2003/02/19 10:18:55  trey
 * updated to use new clientComm version
 *
 * Revision 1.2  2003/02/16 02:31:10  trey
 * full initial command set now implemented
 *
 * Revision 1.1  2003/02/15 05:45:40  trey
 * added stdoutBuffer
 *
 *
 ***************************************************************************/
