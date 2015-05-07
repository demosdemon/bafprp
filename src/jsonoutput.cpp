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

#include <vector>
#include <algorithm>
#include <iostream>

#include "base64.h"

#include "json/json.h"

#include "jsonoutput.h"
#include "baffile.h"

namespace bafprp
{
  const JSON JSON::registerThis;

  template<typename T> 
  static inline std::string ConvertToString(const T& value) {
    std::ostringstream os;
    os << value;
    return os.str();
  }

  static std::vector<std::string> splitString(const std::string& str, const std::string& delimiter = ",") {
    std::vector<std::string> ret;

    std::string::size_type last = 0;
    std::string::size_type pos = 0;

    while ((pos = str.find(delimiter, last)) != std::string::npos) {
      std::string token = str.substr(last, pos - last);
      ret.push_back(token);
      last = pos + 1;
    }

    ret.push_back(str.substr(last));

    return ret;
  }

  typedef struct {
    int module;
    std::string field_type;
    int index;
    Json::Value value;
  } FieldRecord;

  static FieldRecord serializeField(const IField* field) {
    FieldRecord name = {-1, "", 0};

    std::vector<std::string> nameParts = splitString(field->getID(), ".");

    switch (nameParts.size())
    {
    case 1:
      name.module = -1;
      name.field_type = nameParts[0];
      break;
    case 3:
      name.index = atoi( nameParts[2].c_str() );
    case 2:
      name.module = atoi( nameParts[0].substr(1).c_str() );
      name.field_type = nameParts[1];
      break;
    default:
      name.field_type = field->getID();
      break;
    }

    const std::string type = field->getType();
    if (type == "amadns") {
      name.value = field->getLong();
    } else if (type == "duration") {
      name.value = field->getFloat();
    } else if (type == "number") {
      name.value = field->getLong();
    } else if (type == "shortduration") {
      name.value = field->getFloat();
    } else {
      name.value = field->getString();
    }

    return name;
  }

  static Json::Value serializeRecord(const BafRecord* record) {
    Json::Value object(Json::objectValue);

    object["filename"] = record->getFilename();
    object["filepos"] = record->getFilePosition();
    object["type"] = record->getType();
    object["typecode"] = record->getTypeCode();
    object["size"] = record->getSize();
    object["crc"] = ConvertToString(record->getCRC());

    const IField* field = record->getNextField();
    while (field != NULL) {
      if (field->getID() != "modulecode") {
        FieldRecord rec = serializeField(field);

        if (rec.module < 0) {
          object[rec.field_type] = rec.value;
        } else if (rec.module > 0) {
          if (!object.isMember("modules")) {
            object["modules"] = Json::Value(Json::objectValue);
          }

          const std::string module = ConvertToString(rec.module);

          if (!object["modules"].isMember(module)) {
            object["modules"][module] = Json::Value(Json::arrayValue);
          }

          while (object["modules"][module].size() <= rec.index) {
            object["modules"][module].append(Json::Value(Json::objectValue));
          }

          object["modules"][module][rec.index][rec.field_type] = rec.value;
        }
      }

      field = record->getNextField(field->getUID());

    }

    return object;
  }

  JSON::~JSON()
  {
    if( _file.is_open() ) _file.close();
  }

  void JSON::error( const BafRecord* record, const std::string& error )
  {
    LOG_TRACE( "JSON::error" );
    checkFile( _errorProperties, true );
    if( !_file.is_open() )
    {
      Output::setOutputError("console");
      LOG_ERROR( "No valid file for error output, falling back to console output" );

      // be nice
      Output::outputRecord( record );
      return;
    }

    Json::Value errorRecord(Json::objectValue);
    errorRecord["time"] = NowTime();
    errorRecord["error"] = error;
    errorRecord["type"] = record->getType();
    errorRecord["size"] = record->getSize();
    errorRecord["position"] = record->getFilePosition();
    errorRecord["filename"] = record->getFilename();
    errorRecord["data"] = record->getData();

    _file << Json::FastWriter().write(errorRecord);
    _file.flush();

    checkFile(_errorProperties, false);
    LOG_TRACE( "/JSON::error" );
  }

  void JSON::log( LOG_LEVEL level, const std::string& log )
  {
    checkFile( _logProperties, true );
    if( !_file.is_open() )
    {
      Output::setOutputLog( "console" );
      LOG_ERROR( "No valid file for log output, falling back to console output" );

      // be nice
      Output::outputLog( level, log );
      return;
    }

    Json::Value logRecord(Json::objectValue);
    logRecord["time"] = NowTime();
    logRecord["level"] = getStrLogLevel(level);
    logRecord["log"] = log;

    _file << Json::FastWriter().write(logRecord);
    _file.flush();

    checkFile( _logProperties, false );
  }

  void JSON::record( const BafRecord* record )
  {
    LOG_TRACE( "JSON::record" );

    checkFile( _recordProperties, true );
    if( !_file.is_open() )
    {
      LOG_ERROR( "No valid file for record output, falling back to console output" );
      Output::setOutputRecord( "console" );

      // be nice
      Output::outputRecord( record );
      return;
    }

    _file << Json::FastWriter().write(serializeRecord(record));
    _file.flush();

    checkFile( _recordProperties, false );
    LOG_TRACE( "/JSON::record" );
  }

  void JSON::checkFile( property_map& props, bool start )
  {
    // The property should have a filename parameter
    property_map::iterator filename = props.find( "filename" );
    if( filename == props.end() )
    {
      printf( "Error: no 'filename' property for output\n" );
      return;	
    }

    std::string desiredFilename = filename->second;
    while( desiredFilename.find('$') != std::string::npos )
    {
      int begin = desiredFilename.find('$');
      int end = desiredFilename.find( '$', begin + 1 );

      if( end == std::string::npos )
        break;

      std::string keyword = desiredFilename.substr( begin + 1, end - begin - 1 );

      if( keyword == "filename" )
      {
        std::string processingFile = (*(splitString(BafFile::getFilename(), 
#ifdef _WIN32
          "\\"
#else
          "/"
#endif
          ).cend() - 1));

        if( processingFile == "" ) 
          processingFile = "bafprp";
        desiredFilename.replace( begin, end - begin + 1, processingFile );
      }
    }

    // If the function is at the begining, we need to make sure the right file is open
    if( start )
    {
      if( _file.is_open() ) 
      {
        if( _filename != desiredFilename )  // Is the RIGHT file open?
        {
          // If not, we need to store the current file's name, and open the correct file
          _storedFilenames.push_back( _filename );
          _filename = "";
          _file.close();

          // If the filename we are opening has been used before we need to open it to APPEND, however
          // if it has NOT been used before, open it normally, wiping out the previous contents.
          bool bFound = false;
          for( std::vector<std::string>::iterator itr = _usedFilenames.begin(); itr != _usedFilenames.end(); itr++ )
          {
            if( (*itr) == desiredFilename )
            {
              bFound = true;
              break;
            }
          }

          if( !bFound )
            _file.open( desiredFilename.c_str() );  // Clear original file
          else
            _file.open( desiredFilename.c_str(), std::ios::app );  // Append

          // does not matter if file opened or not, individual functions check and make better errors then
          // we could dream of making

          _filename = desiredFilename;
          _usedFilenames.push_back( _filename );  // Make sure we open it for APPENDING next time
        }
        // current open file matches desired file.
      }
      else // No file is open, need to open a new file for output
      {
        // Since there should ALWAYS been one file open, we can assume this is the first time this
        // file is being opened and not check for appending.
        // If you encounter a bug where your files are being cleared in the middle of the program,
        // you are probably doing some very weird things to your output, but if need be, we can copy 
        // the used file check here.
        _file.open( desiredFilename.c_str() );

        // does not matter if file opened or not, individual functions check and make better errors then
        // we could dream of making

        _filename = desiredFilename;
        _usedFilenames.push_back( _filename );  // Open for appending next time
      }
    }
    else  // end of function file check
    {
      // The function is ending, we need to restore the previous open file, if there was one.
      if( _storedFilenames.size() > 0 )  // we had to push off another file to output
      {
        // restore that file
        _file.close();
        _file.open( _storedFilenames.back().c_str(), std::ios::app );
        _filename = _storedFilenames.back();
        _storedFilenames.pop_back();
      }
    }
  }
}