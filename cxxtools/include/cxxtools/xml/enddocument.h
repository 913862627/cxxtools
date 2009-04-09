/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * As a special exception, you may use this file as part of a free
 * software library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU Library
 * General Public License.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef cxxtools_Xml_EndDocument_h
#define cxxtools_Xml_EndDocument_h

#include <cxxtools/xml/api.h>
#include <cxxtools/xml/node.h>


namespace cxxtools {

    namespace xml {

        /**
         * @brief A Node which represents the end of the XML document.
         *
         * The last Node/Element which is read from a document is the EndDocument-node. It is read after
         * the last tag, Text or comment was read from the XML document. This is similar to an eof character
         * at the end of a file read.
         *
         * @see Node
         */
        class CXXTOOLS_XML_API EndDocument : public Node {
            public:
                //! Creates an EndDocument object.
                EndDocument();

                //! Destructs this EndDocument object.
                ~EndDocument();

                /**
                 * @brief Clones this EndDocument object by creating a duplicate on the heap and returning it.
                 * @return A cloned version of this EndDocument object.
                 */
                EndDocument* clone() const
                { return new EndDocument(*this); }

        };

    }

}

#endif








