/* cxxtools/base64stream.h
   Copyright (C) 2004 Tommi Mäkital

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
along with cxxtools; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330,
Boston, MA  02111-1307  USA
*/

#ifndef CXXTOOLS_BASE64STREAM_H
#define CXXTOOLS_BASE64STREAM_H

#include <iostream>

/*
 base64ostream is a base64-encoder. To base64-encode, instantiate
 a base64ostream with an outputstream. Write the data to encode into the
 stream. base64ostream writes the base64-encoded data to the outputstream.
 When ready call end() to pad and flush.

 base64istream is a base54-decoder. To base64-decode, instantiate
 a base64istream with an inputstream. The class reads base64-encoded
 data from the inputstream and you get encoded output.
 */

class base64stream_streambuf : public std::streambuf
{
    std::streambuf* sinksource;
    char obuffer[3];
    char decodebuf[3];
    unsigned count;
    bool indecode;

  public:
    base64stream_streambuf(std::streambuf* sinksource_)
      : sinksource(sinksource_),
        count(0),
        indecode(false)
      { }
    ~base64stream_streambuf()
      { if (indecode) end(); }

    void end();

  private:
    std::streambuf::int_type overflow(std::streambuf::int_type ch);
    std::streambuf::int_type underflow();
    std::streambuf::int_type sync();

    void putchar(char ch);
    int getval();
};

class base64ostream : public std::ostream
{
    base64stream_streambuf streambuf;

  public:
    base64ostream(std::ostream& out)
      : std::ostream(&streambuf),
        streambuf(out.rdbuf())
      { }
    base64ostream(std::streambuf* sb)
      : std::ostream(&streambuf),
        streambuf(sb)
      { }

    void end()  { streambuf.end(); }
};

class base64istream : public std::istream
{
    base64stream_streambuf streambuf;

  public:
    base64istream(std::istream& in)
      : std::istream(&streambuf),
        streambuf(in.rdbuf())
      { }
    base64istream(std::streambuf* sb)
      : std::istream(&streambuf),
        streambuf(sb)
      { }
};

#endif // CXXTOOLS_BASE64STREAM_H
