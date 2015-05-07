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

#ifndef BAFPRPSTRING_H
#define BAFPRPSTRING_H

#include "ifield.h"

/*
Generic class for converting number fields
A number field is any field that describes a solitary number, nothing special
*/

namespace bafprp
{
	class StringField : public IField
	{
		friend class StringFieldMaker;
	public:
		std::string getString() const;
    //int getSize() const;

		bool convert ( const BYTE* data );

		std::string getType() const { return "string"; }

		~StringField();
	private:
		StringField();
	};

	class StringFieldMaker : public FieldMaker
	{
	public:
		StringFieldMaker() : FieldMaker ( "string" ) {}
	protected:
		IField* make() const;
	private:
		static const StringFieldMaker registerThis;
	};

}

#endif
