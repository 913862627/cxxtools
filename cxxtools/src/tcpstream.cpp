/* tcpstream.cpp
   Copyright (C) 2003 Tommi Maekitalo

This file is part of cxxtools.

Cxxtools is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Cxxtools is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cxxtools; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330,
Boston, MA  02111-1307  USA
*/

#include "cxxtools/tcpstream.h"
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <errno.h>
#include <netdb.h>

#undef log_define
#undef log_warn
#undef log_debug

#define log_define(expr)
#define log_warn(expr)
#define log_debug(expr)

namespace cxxtools
{

namespace tcp
{
  using namespace std;

  Exception::Exception(int Errno, const string& msg)
    : runtime_error(msg + ": " + strerror(Errno)),
      m_Errno(Errno)
    { }

  Exception::Exception(const string& msg)
    : runtime_error(msg + ": " + strerror(errno)),
      m_Errno(errno)
    { }

  ////////////////////////////////////////////////////////////////////////
  // implementation of Socket
  //
  Socket::saveflags::saveflags(int _fd)
    : fd(_fd)
  {
    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
    {
      int errnum = errno;
      throw Exception(strerror(errnum));
    }
  }

  Socket::saveflags::~saveflags()
  {
    if (flags >= 0)
      fcntl(fd, F_SETFL, flags);
  }

  Socket::Socket(int domain, int type, int protocol) throw (Exception)
  {
    if ((m_sockFd = ::socket(domain, type, protocol)) < 0)
      throw Exception("cannot create socket");
  }

  Socket::~Socket()
  {
    if (m_sockFd >= 0)
    {
      if (::close(m_sockFd) < 0)
        fprintf(stderr, "error in close(%d)\n", (int)m_sockFd);
    }
  }

  void Socket::create(int domain, int type, int protocol) throw (Exception)
  {
    close();

    if ((m_sockFd = ::socket(domain, type, protocol)) < 0)
      throw Exception("cannot create socket");
  }

  void Socket::close()
  {
    if (m_sockFd >= 0)
    {
      ::close(m_sockFd);
      m_sockFd = -1;
    }
  }

  struct sockaddr Socket::getSockAddr() const throw (Exception)
  {
    struct sockaddr ret;

    socklen_t slen = sizeof(ret);
    if (::getsockname(getFd(), &ret, &slen) < 0)
      throw Exception("error in getsockname");

    return ret;
  }

  ////////////////////////////////////////////////////////////////////////
  // implementation of Server
  //
  Server::Server()
    : Socket(AF_INET, SOCK_STREAM, 0)
  { }

  Server::Server(const std::string& ipaddr, unsigned short int port, int backlog)
                 throw (Exception)
    : Socket(AF_INET, SOCK_STREAM, 0)
  {
    Listen(ipaddr, port, backlog);
  }

  Server::Server(const char* ipaddr, unsigned short int port,
      int backlog) throw (Exception)
    : Socket(AF_INET, SOCK_STREAM, 0)
  {
    Listen(ipaddr, port, backlog);
  }

  void Server::Listen(const char* ipaddr, unsigned short int port,
      int backlog) throw (Exception)
  {
    struct hostent* host = ::gethostbyname(ipaddr);
    if (host == 0)
      throw Exception(std::string("invalid ipaddress ") + ipaddr);

    memset(&servaddr.sockaddr_in, 0, sizeof(servaddr.sockaddr_in));

    servaddr.sockaddr_in.sin_family = AF_INET;
    servaddr.sockaddr_in.sin_port = htons(port);

    memmove(&(servaddr.sockaddr_in.sin_addr.s_addr), host->h_addr, host->h_length);
    int reuseAddr = 1;
    if (::setsockopt(getFd(), SOL_SOCKET, SO_REUSEADDR,
        &reuseAddr, sizeof(reuseAddr)) < 0)
      throw Exception("error in setsockopt");

    if (::bind(getFd(),
               (struct sockaddr *)&servaddr.sockaddr_in,
               sizeof(servaddr.sockaddr_in)) < 0)
      throw Exception("error in bind");

    if (::listen(getFd(), backlog) < 0)
      throw Exception("error in listen");
  }

  ////////////////////////////////////////////////////////////////////////
  // implementation of Stream
  //
  Stream::Stream()
    : timeout(-1)
    { }

  Stream::Stream(const Server& server)
    : timeout(-1)
  {
    Accept(server);
  }

  Stream::Stream(const string& ipaddr, unsigned short int port)
    : Socket(AF_INET, SOCK_STREAM, 0),
      timeout(-1)
  {
    Connect(ipaddr, port);
  }

  Stream::Stream(const char* ipaddr, unsigned short int port)
    : Socket(AF_INET, SOCK_STREAM, 0),
      timeout(-1)
  {
    Connect(ipaddr, port);
  }

  void Stream::Accept(const Server& server)
  {
    close();

    socklen_t peeraddr_len;
    peeraddr_len = sizeof(peeraddr);
    setFd(accept(server.getFd(), &peeraddr.sockaddr, &peeraddr_len));
    if (bad())
      throw Exception("error in accept");

    setTimeout(timeout);
  }

  void Stream::Connect(const char* ipaddr, unsigned short int port)
  {
    if (getFd() < 0)
      create(AF_INET, SOCK_STREAM, 0);

    struct hostent* host = ::gethostbyname(ipaddr);
    if (host == 0)
      throw Exception(std::string("invalid ipaddress ") + ipaddr);

    memset(&peeraddr, 0, sizeof(peeraddr));
    peeraddr.sockaddr_in.sin_family = AF_INET;
    peeraddr.sockaddr_in.sin_port = htons(port);

    memmove(&(peeraddr.sockaddr_in.sin_addr.s_addr), host->h_addr, host->h_length);

    if (::connect(getFd(), &peeraddr.sockaddr,
        sizeof(peeraddr)) < 0)
      throw Exception("error in connect");

    setTimeout(timeout);
  }

  log_define("cxxtools.tcp");

  Stream::size_type Stream::Read(char* buffer, Stream::size_type bufsize) const
  {
    if (timeout < 0)
    {
      // blocking read
      log_debug("blocking read");
      size_type n = ::read(getFd(), buffer, bufsize);
      log_debug("blocking read ready, return " << n);
      if (n < 0)
      {
        // das ging schief
        int errnum = errno;
        throw Exception(strerror(errnum));
      }

      return n;
    }
    else
    {
      // non-blocking read

      ssize_t n;

      // try reading without timeout
      log_debug("non blocking read fd=" << getFd());
      n = ::read(getFd(), buffer, bufsize);
      log_debug("non blocking read, return " << n);

      if (n < 0)
      {
        // no data available

        if (errno == EAGAIN)
        {
          if (timeout == 0)
          {
            log_warn("timeout");
            throw Timeout();
          }

          struct pollfd fds;
          fds.fd = getFd();
          fds.events = POLLIN;
          log_debug("poll timeout " << timeout);
          int p = poll(&fds, 1, timeout);
          log_debug("poll returns " << p);
          if (p < 0)
          {
            int errnum = errno;
            throw Exception(strerror(errnum));
          }
          else if (p == 0)
            throw Timeout();

          log_debug("read");
          n = ::read(getFd(), buffer, bufsize);
          log_debug("read returns " << n);
        }
        else
        {
          int errnum = errno;
          throw Exception(strerror(errnum));
        }
      }

      return n;
    }
  }

  Stream::size_type Stream::Write(const char* buffer,
                                  Stream::size_type bufsize) const
  {
    size_t n = ::write(getFd(), buffer, bufsize);
    if (n <= 0)
      // au weia - das ging schief
      throw Exception("tcp::Stream: error in write");

    return n;
  }

  void Stream::setTimeout(int t)
  {
    timeout = t;

    if (getFd() >= 0)
    {
      long a = timeout >= 0 ? O_NONBLOCK : 0;
      log_debug("fcntl(" << getFd() << ", F_SETFL, " << a);
      fcntl(getFd(), F_SETFL, a);
    }
  }

  streambuf::streambuf(Stream& stream, unsigned bufsize, int timeout)
    : m_stream(stream),
      m_bufsize(bufsize),
      m_buffer(new char_type[bufsize])
  {
    setTimeout(timeout);
  }

  streambuf::int_type streambuf::overflow(streambuf::int_type c)
  {
    if (pptr() != pbase())
    {
      int n = m_stream.Write(pbase(), pptr() - pbase());
      if (n <= 0)
        return traits_type::eof();
    }

    setp(m_buffer, m_buffer + m_bufsize);
    if (c != traits_type::eof())
    {
      *pptr() = (char_type)c;
      pbump(1);
    }

    return 0;
  }

  streambuf::int_type streambuf::underflow()
  {
    Stream::size_type n = m_stream.Read(m_buffer, m_bufsize);
    if (n <= 0)
      return traits_type::eof();

    setg(m_buffer, m_buffer, m_buffer + n);
    return (int_type)(unsigned char)m_buffer[0];
  }

  int streambuf::sync()
  {
    if (pptr() != pbase())
    {
      int n = m_stream.Write(pbase(), pptr() - pbase());
      if (n <= 0)
        return -1;
      else
        setp(m_buffer, m_buffer + m_bufsize);
    }
    return 0;
  }

} // namespace tcp

} // namespace cxxtools
