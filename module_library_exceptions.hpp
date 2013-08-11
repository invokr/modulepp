/**
 * This file is part of module++ (https://bitbucket.org/blue-dev/module).
 *
 * @copyright 2013, Robin Dietrich
 * @license Apache 2.0 <http://www.apache.org/licenses/LICENSE-2.0>
 * @license Public Domain <>
 */
 
#ifndef _MODULEPP_MODULE_LIBRARY_EXCEPTION_HPP_
#define _MODULEPP_MODULE_LIBRARY_EXCEPTION_HPP_

#include <exception>

namespace modulepp {
    /** exception thrown when library is loaded before being unloaded */
    class libraryOverwriteException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading library: handle not null.";
            }
    };
    
    /** exception thrown when library handle is null */
    class libraryLoadException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading library: handle is null.";
            }
    };
    
    /** exception thrown when library handle is null */
    class libraryAccessException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading symbol from uninitialized library.";
            }
    };
}

#endif /* _MODULEPP_MODULE_LIBRARY_EXCEPTION_HPP_ */