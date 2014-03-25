/**
 * @file module_implementation_win32.hpp
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

#ifndef _MODULEPP_MODULE_IMPLEMENTATION_WIN32_HPP_
#define	_MODULEPP_MODULE_IMPLEMENTATION_WIN32_HPP_
#ifdef _WIN32

#include <mutex>
#include <Windows.h>

#include "module_library_exceptions.hpp"

namespace modulepp {
    /** Shared library implementation for windows systems */
    class shared_library_win32 {
        public:
            /** Constructor */
            shared_library_win32() : path(""), handle(nullptr) {}

            /** Destructor */
            virtual ~shared_library_win32() {
                unload(); // free when destructing
            }

            /** Load library from given path */
            void load(const std::string& path, int flags = 0) {
                std::lock_guard<std::mutex> lock(mutex);

                if (handle != nullptr) {
                    throw libraryOverwriteException();
                }

                this->path = path+this->suffix();

                handle = LoadLibraryExA(this->path.c_str(), 0, 0);
                if (!handle) {
                    handle = nullptr;
                    throw libraryLoadException();
                }
            }

            /** Unload library freeing all ressources */
            void unload() {
                std::lock_guard<std::mutex> lock(mutex);

                if (handle != nullptr) {
                    FreeLibrary((HMODULE) handle);
                    handle = nullptr;
                }
            }

            /** Check if library is loaded */
            bool isLoaded() {
                return (handle != nullptr);
            }

            /** Return symbol to looked up pointer or NULL if non is found. */
            void* findSymbol(const std::string& name) {
                std::lock_guard<std::mutex> lock(mutex);

                void* result = nullptr;
                if (handle != nullptr) {
                    result = (void*) GetProcAddress((HMODULE) handle, name.c_str());
                } else {
                    throw libraryAccessException();
                }

                return result;
            }

            /** Returns library suffix. */
            const std::string suffix() {
                return ".dll";
            }
        protected:
            /** Path to the library. */
            std::string path;
            /** Pointer to the shared library. */
            void* handle;
            /** Mutex to synchonize library access. */
            mutable std::mutex mutex;
        private:
            /** Prevent copying */
            shared_library_win32(const shared_library_unix&) = delete;
    };
};

#endif  /* _WIN32 */
#endif	/* _MODULEPP_MODULE_IMPLEMENTATION_WIN32_HPP_ */