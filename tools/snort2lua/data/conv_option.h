/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
 * Copyright (C) 2002-2013 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
// conv_var.h author Josh Rosenbaum <jorosenba@cisco.com>

#ifndef CONV_OPTIONS_H
#define CONV_OPTIONS_H


#include <string>
#include <vector>
#include <iostream>

class Option
{
public:
    Option(std::string name, int val, int depth);
    Option(std::string name, bool val, int depth);
    Option(std::string name, std::string val, int depth);
    virtual ~Option();

    inline std::string get_name(){ return name; };
 
    // overloading operators
    friend std::ostream &operator<<( std::ostream&, const Option &);
    friend bool operator!=(const Option& lhs, const Option& rhs);
    friend bool operator==(const Option& lhs, const Option& rhs);

private:
    enum class OptionType{ STRING, BOOL, INT};

    std::string name;
    std::string value;
    int depth;
    OptionType type;


};


#endif
