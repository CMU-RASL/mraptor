/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.23 $  $Author: brennan $  $Date: 2007/02/28 00:44:45 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    stdoutBuffer.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#ifdef REDHAT6_2
#include <sys/time.h>
#include <unistd.h>
#endif
#ifdef CYGWIN_HEADERS
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/termios.h>
#endif

#include <iostream>
#include <cstring>

#include "mrCommonDefs.h"
#include "stdoutBuffer.h"
#include "mrSafeSystem.h"

using namespace std;

/**********************************************************************
 * BYTE BUFFER
 **********************************************************************/

Byte_Buffer::Byte_Buffer(int _fd, int _size) :
  fd(_fd),
  size(_size)
{
#if 0
  fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
  buf = new char[size];
  start_pos = end_pos = buf;
}

Byte_Buffer::~Byte_Buffer(void) {
  release_fd();
  delete buf;
}

void Byte_Buffer::release_fd(void) {
  fd = -1;
}

void Byte_Buffer::reopen(int _new_fd) {
  release_fd();
  fd = _new_fd;
#if 0
  fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
  start_pos = end_pos = buf;
}

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
int Byte_Buffer::getline(Line_Data *ld, bool cleanup)
{
  // get as much data as possible before looking for EOL
  _read_bytes();

  // hunt for EOL character
  char *c;
  char *eol_pos = 0;
  char *end_scan = end_pos;
  bool enough_to_fill_line = false;
  if (end_scan >= start_pos + MR_LINE_LENGTH) {
    end_scan = start_pos + MR_LINE_LENGTH;
    enough_to_fill_line = true;
  }
  int eol_chars = 0;

  ld->line_type = 'c'; // default, may get overridden later
  for (c=start_pos; c < end_scan; c++) {
    //printf("c = 0x%02x\n", *c);
    if (*c == '\r') {
      eol_pos = c;
      if (&c[1] < end_pos && c[1] == '\n') {
	eol_chars = 2;
	ld->line_type = 'x';
      } else {
	eol_chars = 1;
	ld->line_type = 'r';
      }
      break;
    }
    if (*c == '\n') {
      eol_pos = c;
      eol_chars = 1;
      ld->line_type = 'x';
      break;
    }
  }

  int copy_chars = 0;
  bool got_line = true;

  if (eol_pos) {
    // case 1
    copy_chars = eol_pos - start_pos;
  } else if
    // case 2
# if 0
  (end_pos - start_pos > 0)
#else
    (enough_to_fill_line
     || (cleanup && (end_pos - start_pos > 0)))
#endif
    {
    copy_chars = end_scan - start_pos;
  } else {
    got_line = false;
  }

  if (got_line) {
    strncpy(ld->data, start_pos, copy_chars);
    ld->data[copy_chars] = '\0';
    start_pos += copy_chars + eol_chars; // also advance past EOL chars
    gettimeofday(&ld->time_stamp, 0);
    return BUFFER_GOT_LINE;
  }

  // case 3
  if (end_pos != start_pos) {
    return BUFFER_UNFINISHED_LINE;
  } else {
    return BUFFER_NO_DATA;
  }
}

// attempts to read as many bytes as possible from the file
// represented by fd.  returns the number of bytes read.
int Byte_Buffer::_read_bytes(void) {
  if (-1 == fd) return 0;

  long available = 0;
  safe_ioctl(fd, FIONREAD, &available);
#ifndef CYGWIN_IO
  //safe_ioctl(fd, FIONREAD, &available);
#else
  // Cygwin coopts FIONREAD into a macro function;
  //  in theory, TIOCINQ is the same back in Linux too (it should be 
  //  #def'd to FIONREAD in ioctls.h), but don't change the Linux
  //  code just in case.
  //safe_ioctl(fd, TIOCINQ, &available);
  //safe_ioctl(fd, FIONREAD, &available);
#endif
  int free_space;
  char *data = _get_data_pointer(&free_space);
#if 0
  available = free_space;
#endif
  int bytes_to_read = MIN(available, free_space);
  int num_bytes_read = safe_read(fd, data, bytes_to_read);
  _mark_data_read(num_bytes_read);

  return num_bytes_read;
}

// returns a pointer to the free space after the end of the data
// currently in the buffer, and sets free_bytes to be the number of free
// bytes available.
char *Byte_Buffer::_get_data_pointer(int* free_bytes) {
  _defrag_buffer();
  if (free_bytes) *free_bytes = size - (end_pos - start_pos);
  return end_pos;
}

// after you read in data to the area returned by _get_data_pointer(),
// use this to say how many bytes you read in.  it advances the end of
// data pointer.
void Byte_Buffer::_mark_data_read(int num_bytes) {
  end_pos += num_bytes;
}

// moves the location of data so that start_pos is the beginning of
// the buffer, and all free space is at the end.
void Byte_Buffer::_defrag_buffer(void) {
  if (start_pos != buf) {
    int n = end_pos - start_pos;
    memmove(buf, start_pos, n);
    start_pos = buf;
    end_pos = buf+n;
  }
}


/**********************************************************************
 * LINE BUFFER
 **********************************************************************/

Line_Buffer::Line_Buffer(int _fd, int _num_lines) :
  bbuf(_fd),
  num_lines(_num_lines)
{
  end_line = 0;
  num_stored_lines = 0;
  num_lines_read = 0;

  lbuf = new Line_Data[num_lines];
}

Line_Buffer::~Line_Buffer(void) {
  delete lbuf;
}

void Line_Buffer::release_fd(void) {
  bbuf.release_fd();
}

void Line_Buffer::reopen(int _new_fd) {
  bbuf.reopen(_new_fd);
  end_line = 0;
  num_stored_lines = 0;
  num_lines_read = 0;
}

// behavior just like Byte_Buffer::getline, except that
// we (a) store lines as we read them, and (b) return the data
// in a Line_Data structure with a timestamp.
int Line_Buffer::getline(Line_Data** ldata, bool cleanup) {
  Line_Data* ld = &lbuf[end_line];
  int status = bbuf.getline(ld, cleanup);
  if (BUFFER_GOT_LINE == status) {
    end_line = _normalize(end_line + 1);
    if (num_stored_lines < num_lines) num_stored_lines++;
    num_lines_read++;
    (*ldata) = ld;
  } else {
    (*ldata) = NULL;
  }
  return status;
}

// Push a line onto the end of the buffer.  This bypasses the
// Byte_Buffer, and lines thus pushed will *not* be returned by
// getline().  If line is longer than MR_LINE_LENGTH, it will be
// truncated, unless line_type is '?', in which case the line will
// be split.
void Line_Buffer::pushline(const char* line, char line_type) {
  Line_Data* ld = NULL;
  const char* lineWalker = line;
  bool longLine = (line_type == '?' && strlen(line) > MR_LINE_LENGTH);
  unsigned int curSegmentLength;
  //printf("Entering pushline; line_type = '%c', line = '%s'\n", line_type, line);
  do {
    ld = &lbuf[end_line];
    ld->line_type = line_type;

    curSegmentLength = MIN(MR_LINE_LENGTH, strlen(lineWalker)+1); // Want \0 too.
    strncpy(ld->data, lineWalker, curSegmentLength);
    ld->data[curSegmentLength-1] = '\0';
    // lineWalker now points to the next character to read or the terminating null character
    lineWalker += curSegmentLength - 1;

    gettimeofday(&ld->time_stamp, 0);

    end_line = _normalize(end_line + 1);
    if (num_stored_lines < num_lines) num_stored_lines++;
    num_lines_read++;

    //printf("(%c, %s): Pushed '%s'.\n", line_type, lineWalker, ld->data);

    if(longLine) {
      if(lineWalker == NULL || strlen(lineWalker) <= 0) {
        ld->line_type = 'x';
      } else {
        ld->line_type = 'c';
      }
    } else if(line_type == '?') {
      ld->line_type = 'x';
    }
  } while(longLine && lineWalker != NULL && strlen(lineWalker) > 0);
  //printf("Exiting pushline.\n");
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: stdoutBuffer.cc,v $
 * Revision 1.23  2007/02/28 00:44:45  brennan
 * Fixed pointer overrun bug introduced by including MRAPTORD: entries in
 * playback history.
 *
 * Revision 1.22  2007/02/06 21:49:54  trey
 * patched the last patch to avoid an extremely subtle change in behavior which you can figure out yourself if you feel like bothering :)
 *
 * Revision 1.21  2007/02/06 00:22:08  trey
 * Found unexpected (though technically correct) processing of the CRLF end of line marker for lines that have exactly 79 normal characters. Tweaked the processing so that the behavior is the same as for other line lengths.  Clients will still need to handle the unexpected behavior properly because it can happen when a process prints to un unbuffered stream (e.g. stderr).
 *
 * Revision 1.20  2006/12/20 19:06:54  brennan
 * 'MRAPTORD:' outputs are now pushed into the relevant line buffers,
 * so that they're returned when a playback is requested.
 *
 * Revision 1.19  2005/06/24 18:00:10  trey
 * added release_fd() function
 *
 * Revision 1.18  2004/12/01 06:58:48  brennan
 * Added platform support for RH 6.2; mostly, headers are in different places.
 * Note that this will only work on a RH 6.2 system hacked to use a newer
 * gcc than ships with 6.2 by default (tested on Valerie with gcc 3.2.3).
 *
 * Revision 1.17  2004/08/02 20:28:26  trey
 * fixed bug with shadowed declaration of "status" variable
 *
 * Revision 1.16  2004/08/02 18:57:44  trey
 * sigh... tried to fix stderr output again
 *
 * Revision 1.13  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.12  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.11  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.10  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.9  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.8  2003/03/14 23:21:35  mwagner
 * reverted handling of partial lines to avoid problems with simulator output
 *
 * Revision 1.7  2003/03/12 15:52:32  trey
 * changed behavior to immediately output partial lines with c line code.  this way the central prompt shows up properly.  may change it back later if it causes problems.
 *
 * Revision 1.6  2003/03/03 17:21:03  trey
 * removed eof() since i am not sure it works, fixed a couple of obscure corner cases
 *
 * Revision 1.5  2003/02/26 05:09:23  trey
 * implemented chris's suggestion for \r handling
 *
 * Revision 1.4  2003/02/19 10:18:55  trey
 * updated to use new clientComm version
 *
 * Revision 1.3  2003/02/19 00:46:51  trey
 * implemented logging, updated format of stdout responses, improved cleanup
 *
 * Revision 1.2  2003/02/16 02:31:10  trey
 * full initial command set now implemented
 *
 * Revision 1.1  2003/02/15 05:45:40  trey
 * added stdoutBuffer
 *
 *
 ***************************************************************************/
