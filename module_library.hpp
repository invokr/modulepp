/**
 * This file is part of module++ (https://bitbucket.org/blue-dev/module).
 *
 * @copyright 2013, Robin Dietrich
 * @license Apache 2.0 <http://www.apache.org/licenses/LICENSE-2.0>
 * @license Public Domain <>
 */
 
#ifndef _MODULEPP_MODULE_LIBRARY_HPP_
#define	_MODULEPP_MODULE_LIBRARY_HPP_

#include <string>
#include <unordered_map>
#include <mutex>

#include "module_factory.hpp"

#if defined(__LINUX__) || defined(__APPLE__) || defined(hpux) || defined(_hpux) || defined(__GNUC__)
    #include "module_implementation_unix.hpp"
    #define SHARED_LIBRARY_IMPLEMENTATION sharedLibraryUnix
#else
    static_assert( false, "No viable sharedLibrary implementation found." );
#endif

namespace modulepp {
    /** exception thrown when pointer returned by creator is null */
    class libraryCreateException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading class, null returned by creator.";
            }
    };
    
    /** exception thrown when load symbol is missing from library */
    class librarySymbolMissingException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error loading class, load symbol missing.";
            }
    };
    
    /** shared library dummy class, inherits from its implementation. */
    template <typename impl = SHARED_LIBRARY_IMPLEMENTATION>
    class sharedLibraryBase : public impl {
        public:
            /** check if symbol exists. */
            virtual bool hasSymbol(const std::string& name) {
                return (this->findSymbol(name) != NULL);
            }
            
            /** returns path to library loaded. */
            virtual const std::string& getPath() const {
                return this->path;
            }
            
            /** empty constructor */
            sharedLibraryBase() {
                
            }
            
            /** nessecary to call parent destructor */
            virtual ~sharedLibraryBase() {
                
            }
        private:
            /** non-copyable */
            sharedLibraryBase(const sharedLibraryBase&);
    };
    
    /** type definition for the system specific shared library implementation. */
    typedef sharedLibraryBase<> sharedLibrary;
    
    /** loads exported C++ classes with a common base from a shared library */
    template <class B> 
    class classLoader {
        public:
            /** definition for the base class factory type. */
            typedef factory<std::string, B> Factory;
            
            /** struct containing information about a loaded library. */
            struct libraryInfo {
                /** pointer to sharedLibrary object */
                sharedLibrary* pLibrary;
                /** pointer to factory */
                const Factory* pFactory;
                /** reference count for shared library */
                mutable int refCount;
            };
            
            /** type for a map containing different loaded libraries. */
            typedef std::unordered_map<std::string, libraryInfo> libraryMap;
            
            /** definition for factory creator for base class */
            typedef factoryCreator<B> Creator;   
        
            /** definition for optional initialization function. */
            typedef void (*InitializeLibraryFunc)();
            
            /** definition for optional uninitialization function. */
            typedef void (*UninitializeLibraryFunc)();
            
            /** definition for build function */
            typedef bool (*BuildFactoryFunc)(Factory*);            

            /** the loaders very own iterator class */
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


            /** destructor, frees left over library ressources */
            virtual ~classLoader() {
                for (typename libraryMap::const_iterator it = _map.begin(); 
                     it != _map.end(); ++it) {
                    // unload library prior to deleting 
                    it->second.pLibrary->unload();
                    delete it->second.pLibrary;
                    delete it->second.pFactory;
                }
            }

            /** loads a library from the given path */
            void loadLibrary(const std::string& path) {
                std::lock_guard<std::mutex> lock(mutex);
                
                // check if library has already been loaded
                typename libraryMap::const_iterator it = _map.find(path);
                if (it == _map.end()) {
                    // create info struct for library
                    libraryInfo li;
                    li.pLibrary = new sharedLibrary();
                    li.pLibrary->load(path);
                    li.pFactory = new Factory();
                    li.refCount = 1;

                    // find symbols
                    try {
                        // initialize (optional)
                        if (li.pLibrary->hasSymbol("initializeLibrary")) {
                            InitializeLibraryFunc initializeLibrary = 
                                reinterpret_cast<InitializeLibraryFunc> (
                                    li.pLibrary->findSymbol("initializeLibrary")
                                );
                            
                            initializeLibrary();
                        }

                        // build (required)
                        if (li.pLibrary->hasSymbol("buildFactory")) {
                            BuildFactoryFunc buildManifest = 
                                reinterpret_cast<BuildFactoryFunc> ( 
                                    li.pLibrary->findSymbol("buildFactory")
                                );
                            
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

            /** unload shared library and delete all references to it. */
            void unloadLibrary(const std::string& path) {
                std::lock_guard<std::mutex> lock(mutex);

                // check if library exists
                typename libraryMap::iterator it = _map.find(path);
                if (it != _map.end()) {
                    // only unload if reference count is zero
                    if (--it->second.refCount == 0) {
                        // check for optional uninitialize symbol
                        if (it->second.pLibrary->hasSymbol("uninitializeLibrary")) {
                            UninitializeLibraryFunc uninitializeLibrary = 
                                reinterpret_cast<UninitializeLibraryFunc> (
                                    it->second.pLibrary->findSymbol("uninitializeLibrary")
                                );
                            
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
            
            /** returns whether a specific can be created. */
            bool hasClass(const std::string& className) {
                return (getCreator(className) != NULL);
            }

            /** returns an instance of a class by its name. */
            B* getClass(const std::string& className) {
                const Creator* cre = getCreator(className);
                if (cre == NULL) {
                    throw libraryCreateException();
                } else {
                    return cre->create();
                }
            }

            /** returns whether library is loaded or not. */
            bool isLibraryLoaded(const std::string& path) {
                 return (getFactory(path) != NULL);
            }

            /** returns iterator pointing at the beginning of the list. */
            const Iterator begin() {
                std::lock_guard<std::mutex> lock(mutex);
                return Iterator(_map.begin());
            }

            /** returns iterator pointing at the end of the list. */
            const Iterator end() {
                std::lock_guard<std::mutex> lock(mutex);
                return Iterator(_map.end());
            }
        private:
            /** map of libraries and their corresponding info object */
            libraryMap _map;
            
            /** mutex for syncronisation. */
            mutable std::mutex mutex;
            
            /** returns a pointer to the classes creator object */
            const Creator* getCreator(const std::string& className) {
                std::lock_guard<std::mutex> lock(mutex);

                // iterate available libraries
                for (typename libraryMap::const_iterator it = _map.begin(); 
                     it != _map.end(); ++it) 
                {
                    // check if class is available in current library
                    const Factory* pFactory = it->second.pFactory;
                    typename Factory::Iterator itm = pFactory->find(className);
                    if (itm != pFactory->end())
                        return *itm;
                }
                
                // return NULL if class could not be found
                return NULL;
            }
            
            /** returns a pointer to the factory for the given library */
            const Factory* getFactory(const std::string& path) {
                std::lock_guard<std::mutex> lock(mutex);

                // check map for factory
                typename libraryMap::const_iterator it = _map.find(path);
                if (it != _map.end()) {
                    return it->second.pFactory;
                } else {
                    // return NULL if factory could not be found
                    return NULL;
                }
            }
    };
}

#endif	/* _MODULEPP_MODULE_LIBRARY_HPP_ */