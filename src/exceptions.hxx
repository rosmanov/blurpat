/**
 * Copyright © 2014,2015 Ruslan Osmanov <rrosmanov@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#pragma once
#ifndef EXCEPTIONS_HXX
#define EXCEPTIONS_HXX

#include <iosfwd>
#include <string>
#include <stdexcept>

using std::runtime_error;
using std::string;

class ErrorException : public runtime_error
{
  public:
    ErrorException(): runtime_error(""), mMsg("") {}
    ErrorException(string msg) : runtime_error(""), mMsg(msg) {}
    ErrorException(const char* format, ...);
    virtual ~ErrorException() throw() { mMsg.clear(); }
    /* virtual */ const char* what() throw() { return mMsg.c_str(); }

  protected:
    string mMsg;
};

class InvalidCliArgException: public ErrorException
{
  public:
    InvalidCliArgException(string& msg) { mMsg = msg; }
    InvalidCliArgException(const char* format, ...);
    virtual ~InvalidCliArgException() throw() {}
};

#endif // EXCEPTIONS_HXX
// vim: et ts=2 sts=2 sw=2
