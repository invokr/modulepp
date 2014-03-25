/**
 * @file module_library.hpp
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

#ifndef _MODULEPP_MODULE_LIBRARY_HPP_
#define	_MODULEPP_MODULE_LIBRARY_HPP_

#include <string>
#include <unordered_map>
#include <mutex>

#include "module_library_exceptions.hpp"
#include "module_factory.hpp"

#if defined(__LINUX__) || defined(__APPLE__) || defined(hpux) || defined(_hpux) || defined(__GNUC__)
    #include "module_implementation_unix.hpp"
    #define SHARED_LIBRARY_IMPLEMENTATION shared_library_unix
#elif defined(_WIN32)
    #include "module_implementation_win32.hpp"
    #define SHARED_LIBRARY_IMPLEMENTATION shared_library_win32
#else
    static_assert( false, "No viable shared library implementation found." );
#endif

namespace modulepp {
    /** Shared library class, inherits from its implementation based on the current operating system. */
    template <typename impl = SHARED_LIBRARY_IMPLEMENTATION>
    class shared_library_base : public impl {
        public:
            /** Returns true if specified symbol exists. */
            virtual bool hasSymbol(const std::string& name) {
                return (this->findSymbol(name) != nullptr);
            }

            /** Returns path to loaded library. */
            virtual const std::string& getPath() const {
                return this->path;
            }

            /** Default constructor */
            shared_library_base() = default;

            /** Virtual destructor */
            virtual ~shared_library_base() {}
        private:
            /** Non-Copyable */
            shared_library_base(const shared_library_base&) = delete;
    };

    /** Typedef for the operating-system specific shared_library implementation. */
    typedef shared_library_base<> shared_library;

    /** Loads exported C++ classes extending a common base class from a shared library. */
    template <class B>
    class class_loader {
        public:
            /** Definition the factory of the base class. */
            typedef factory<std::string, B> Factory;

            /** Struct containing information about a loaded library. */
            struct libraryInfo {
                /** Pointer to shared_library object */
                shared_library* pLibrary;
                /** Pointer to factory */
                const Factory* pFactory;
                /** Reference count for shared library */
                mutable int refCount;
            };

            /** Type for a map containing different loaded libraries. */
            typedef std::unordered_map<std::string, libraryInfo> libraryMap;

            /** Definition for factory creator for base class */
            typedef factoryCreator<B> Creator;

            /** Definition for optional initialization function. */
            typedef void (*InitializeLibraryFunc)();

            /** Definition for optional uninitialization function. */
            typedef void (*UninitializeLibraryFunc)();

            /** Definition for build function */
            typedef bool (*BuildFactoryFunc)(Factory*);

            /** The loaders very own iterator class */
            class Iterator {
                public:
                    typedef std::pair<std::string, const Factory*> Pair;
                private:
                    typename libraryMap::const_iterator _it;
                    mutable Pair _pair;
                public:
                    Iterator(const typename libraryMap::const_iterator& it) {
                        _it = it;
                    }

                    Iterator(const Iterator& it) {
                        _it = it._it;
                    }

                    ~Iterator() {}

                    Iterator& operator = (const Iterator& it) {
                        _it = it._it;
                        return *this;
                    }

                    inline bool operator == (const Iterator& it) const {
                        return _it == it._it;
                    }

                    inline bool operator != (const Iterator& it) const {
                        return _it != it._it;
                    }

                    Iterator& operator ++ () {
                        ++_it;
                        return *this;
                    }

                    Iterator operator ++ (int) {
                        Iterator result(_it);
                        ++_it;
                        return result;
                    }

                    inline const Pair* operator * () const {
                        _pair.first  = _it->first;
                        _pair.second = _it->second.pFactory;
                        return &_pair;
                    }

                    inline const Pair* operator -> () const {
                        _pair.first  = _it->first;
                        _pair.second = _it->second.pFactory;
                        return &_pair;
                    }
            };


            /** Destructor, frees left over library ressources */
            virtual ~class_loader() {
                std::lock_guard<std::mutex> lock(mutex);

                for (auto &it : _map) {
                    it.second.pLibrary->unload();
                    delete it.second.pLibrary;
                    delete it.second.pFactory;
                }
            }

            /** Loads a library from the given path */
            void load(const std::string& path) {
                std::lock_guard<std::mutex> lock(mutex);

                // check if library has already been loaded
                auto it = _map.find(path);
                if (it == _map.end()) {
                    // create info struct for library
                    libraryInfo li;
                    li.pLibrary = new shared_library();
                    li.pLibrary->load(path);
                    li.pFactory = new Factory();
                    li.refCount = 1;

                    try {
                        // initialize symbol (optional)
                        if (li.pLibrary->hasSymbol("initializeLibrary")) {
                            InitializeLibraryFunc initializeLibrary =
                                reinterpret_cast<InitializeLibraryFunc> ( li.pLibrary->findSymbol("initializeLibrary") );

                            initializeLibrary();
                        }

                        // build (required)
                        if (li.pLibrary->hasSymbol("buildFactory")) {
                            BuildFactoryFunc buildManifest =
                                reinterpret_cast<BuildFactoryFunc> ( li.pLibrary->findSymbol("buildFactory") );

                            if (buildManifest(const_cast<Factory*>(li.pFactory))) {
                                _map[path] = li;
                            }
                        } else {
                            throw librarySymbolMissingException();
                        }
                    } catch (...) {
                        // forward any exceptions thrown
                        delete li.pFactory;
                        delete li.pLibrary;
                        throw;
                    }
                } else {
                    // increase reference count if library was already loaded
                    ++it->second.refCount;
                }
            }

            /** Unload a shared library, invalidates all references to it. */
            void unload(const std::string& path) {
                std::lock_guard<std::mutex> lock(mutex);

                // check if library exists
                auto it = _map.find(path);
                if (it != _map.end()) {
                    if (--it->second.refCount == 0) {
                        // check for optional uninitialize symbol
                        if (it->second.pLibrary->hasSymbol("uninitializeLibrary")) {
                            UninitializeLibraryFunc uninitializeLibrary =
                                reinterpret_cast<UninitializeLibraryFunc> ( it->second.pLibrary->findSymbol("uninitializeLibrary") );

                            uninitializeLibrary();
                        }

                        // unload prior deleting
                        delete it->second.pFactory;
                        it->second.pLibrary->unload();
                        delete it->second.pLibrary;
                        _map.erase(it);
                    }
                }
            }

            /** Returns whether a specific class can be created. */
            bool has(const std::string& className) {
                return (getCreator(className) != nullptr);
            }

            /** Returns a new instance of a class by its name. */
            B* create(const std::string& className) {
                const Creator* cre = getCreator(className);
                if (cre == nullptr) {
                    throw libraryCreateException();
                } else {
                    return cre->create();
                }
            }

            /** Returns whether library a library is already loaded or not. */
            bool loaded(const std::string& path) {
                 return (getFactory(path) != nullptr);
            }

            /** Returns iterator pointing at the beginning of the class list. */
            const Iterator begin() {
                std::lock_guard<std::mutex> lock(mutex);
                return Iterator(_map.begin());
            }

            /** Returns iterator pointing at the end of the class list. */
            const Iterator end() {
                std::lock_guard<std::mutex> lock(mutex);
                return Iterator(_map.end());
            }
        private:
            /** Map of libraries and their corresponding info object */
            libraryMap _map;
            /** Mutex for internal syncronisation. */
            mutable std::mutex mutex;

            /** Returns a pointer to the classes creator object. */
            const Creator* getCreator(const std::string& className) {
                std::lock_guard<std::mutex> lock(mutex);

                // iterate available libraries
                for (auto &it: _map) {
                    auto itm = it.second.pFactory->find(className);
                    if (itm != it.second.pFactory->end())
                        return *itm;
                }

                // return NULL if class could not be found
                return nullptr;
            }

            /** Returns a pointer to the factory for the given library */
            const Factory* getFactory(const std::string& path) {
                std::lock_guard<std::mutex> lock(mutex);

                // check map for factory
                auto it = _map.find(path);
                if (it != _map.end()) {
                    return it->second.pFactory;
                } else {
                    return nullptr;
                }
            }
    };
}

#endif	/* _MODULEPP_MODULE_LIBRARY_HPP_ */