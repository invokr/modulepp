# module++ - C++ Module Library

## About

module++ is a header-only C++ library providing a mechanism to load classes inheriting a common
base class from shared librarys. The library is thread-safe.

Some parts of the code are inspired by (Poco)[http://pocoproject.org/].

## License

Dual-licensed under the Public Domain and Apache 2.0 license.

## Requirements

module++ requires the C++11 standard library. Implementations for individual operating systems may require
linking of a additional libraries (e.g. dl for linux).

## Using the code

 * Declare a base class your modules will inherit from:

        // module_base.hpp
        
        class Module {
        public:
            /** method to be overloaded by each module */
            virtual void DoStuff(int) = 0;
        };

 * Create modules and export them using the provided macros:

        // module_add.hpp
        
        #include "module_base.hpp"
        #include "modulepp/module_header.hpp"
        
        class ModuleAdd : public Module {
        private:
            int MyInt;
        public:
            ModuleAdd() : MyInt(0) {};
            virtual void DoStuff(int);
        };
        
        BEGIN_MODULE_FACTORY(Module)
            EXPORT_CLASS(ModuleAdd)
        END_MODULE_FACTORY
        
        // module_add.cpp
        
        void ModuleAdd::DoStuff(int i) {
            MyInt += i;
        }
    
 * Create a shared library "module_add.so":

        g++ -fpic -std=c++0x -c module_add.cpp
        g++ -shared -std=c++0x -omodule_add module_add.o

 * At this point the only file that needs to be available to your program to load and use the
   functionallity in module_add.so in addition to modulepp is the base class all your modules 
   inherit from:

        // main.cpp
        
        #include "module_base.hpp"
        #include "modulepp/module_library.hpp"
        
        using namespace modulepp;
        
        int main() {
            // create a class loader, template parameter is your base class
            classLoader<Module> loader;
            
            // load the library by path, !important! don't add the extension to get cross-platform compability
            loader.loadLibrary("./module_add");
            
            // create the ModuleAdd class and access it using the public methods provided through the base class
            ModuleAdd *module = loader.getClass("ModuleAdd");
            ModuleAdd->DoStuff(100);
            
            // clean up after yourself
            delete module;
            loader.unloadLibrary("./module_add");
        
            return 0;
        }