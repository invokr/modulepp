/**
 * @file module_library_exceptions.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.1
 *
 * @par License
 *    Module++
 *    Copyright 2014 Robin Dietrich
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 * @par License
 *    If available in your jurisdiction, this code may be treated as if placed
 *    in the public domain.
 */

#ifndef _MODULEPP_MODULE_LIBRARY_EXCEPTION_HPP_
#define _MODULEPP_MODULE_LIBRARY_EXCEPTION_HPP_

#include <exception>

namespace modulepp {
    /** Exception thrown when a new library is loaded before explicitly freeing the old one. */
    class libraryOverwriteException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading library: handle not null.";
            }
    };

    /** Exception thrown when library failed to load. */
    class libraryLoadException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading library: handle still null.";
            }
    };

    /** Exception thrown when trying to load a symbol from an uninitialized library. */
    class libraryAccessException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading symbol from uninitialized library.";
            }
    };

    /** Exception thrown when pointer returned by library creator is null */
    class libraryCreateException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error calling create: null returned by function.";
            }
    };

    /** Exception thrown when loading symbol is missing from library */
    class librarySymbolMissingException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading class: load symbol missing from library.";
            }
    };
}

#endif /* _MODULEPP_MODULE_LIBRARY_EXCEPTION_HPP_ */