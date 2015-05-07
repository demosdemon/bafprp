/*
Copyright (C) 2015 by Brandon LeBlanc
demosdemon@gmail.com

This file is part of bafprp.

bafprp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

bafprp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with bafprp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "string_dt.h"
#include "output.h"

namespace bafprp
{
  const StringFieldMaker StringFieldMaker::registerThis;

  IField* StringFieldMaker::make() const
  {
    return new StringField;
  }

  StringField::StringField() : IField()
  {
    LOG_TRACE( "StringField::StringField" );
    _size = 0;
    LOG_TRACE( "/StringField::StringField" );
  }


  StringField::~StringField()
  {
    LOG_TRACE( "StringField::~StringField" );
    LOG_TRACE( "/StringField::~StringField" );
  }

  /*int StringField::getSize() const {
    return _size;
  }*/

  bool StringField::convert ( const BYTE* data )
  {
    LOG_TRACE( "StringField::convert" );

    int length = atoi( getChars(data, 3).c_str() );
    int dataType = atoi( getChars(data+2, 3).c_str() );
    if (dataType != 3) {
      _lastError = "Data Type does not indicate ASCII string";
      _converted = false;
      return false;
    }

    /*const BYTE* start = data + 4;
    const BYTE* end = data + 64;

    while ((*end) != 0x0C) {
      end++;
    }

    _size = (((end - data)-1) * 2);*/

    _return = std::string((const char*)(data+4), std::min(length, 64));
    _converted = true;

    if( _return.length() != length ) 
    {
      _lastError = "Data read is not the correct size";
      _converted = false;
    }

    LOG_TRACE( "/NumberField::convert" );
    return _converted;
  }

  std::string StringField::getString() const
  {
    LOG_TRACE( "NumberField::getString" );

    std::string ret;
    if( !_converted )
    {
      LOG_WARN( "Tried to get string before field was converted" );
      ret = "";
    } else {
      ret = _return;
    }

    LOG_TRACE( "/NumberField::getString" );
    return ret;
  }
}
