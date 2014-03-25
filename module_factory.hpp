/**
 * @file module_factory.hpp
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

#ifndef _MODULEPP_MODULE_FACTORY_HPP_
#define	_MODULEPP_MODULE_FACTORY_HPP_

#include <set>
#include <exception>
#include <unordered_map>

namespace modulepp {
    /** exception thrown when object cannot be created from identifier */
    class factoryException : public std::exception {
        public:
            virtual const char* what() const throw() {
                return "Error creating object: Unkown identifier.";
            }
    };

    /** base template for different class creators */
    template <typename B>
    class factoryCreator {
        public:
            /** constructor */
            factoryCreator() {}
            /** destructor */
            virtual ~factoryCreator() {}
            /** to be overloaded */
            virtual B* create() const = 0;
        private:
            /** noncopyable */
            factoryCreator(const factoryCreator&) = delete;
    };

    /** basic creator requiring you to manage the object lifetime */
    template <typename B, typename C>
    class factoryCreatorBasic : public factoryCreator<B> {
        public:
            /** constructor */
            factoryCreatorBasic() {}
            /** destructor */
            virtual ~factoryCreatorBasic() {}
            /** returns pointer to newly created object */
            B* create() const {
                return new C;
            }
        private:
            /** noncopyable */
            factoryCreatorBasic(const factoryCreatorBasic&) = delete;
    };

    /** advanced creator freeing all created objects when going out of scope */
    template <typename B, typename C>
    class factoryCreatorAdvance : public factoryCreator<B> {
        private:
            /** list of allocated objects for given type */
            typedef std::set<B*> ObjectSet;
            /** Set of objects created / to be deleted. */
            mutable ObjectSet deleteSet;

            /** noncopyable */
            factoryCreatorAdvance(const factoryCreatorAdvance&) = delete;
        public:
            /** constructor */
            factoryCreatorAdvance() : ObjectSet(), deleteSet() {}

            /** destructor, deletes all created objects */
            virtual ~factoryCreatorAdvance() {
                for (typename ObjectSet::iterator it = deleteSet.begin();
                    it != deleteSet.end(); ++it) {
                    delete *it;
                }
            };

            /** returns pointer to newly created object */
            B* create() const {
                B *b = new C;
                this->deleteSet.insert(b);
                return b;
            }
    };

    /** factory base class */
    class factoryBase {
        private:
            /** non-copyable */
            factoryBase(const factoryBase&);
        public:
            /** constructor */
            factoryBase() {}
            /** destructor */
            virtual ~factoryBase() {}
            /** returns own type name */
            virtual const char* typeName() const = 0;
    };

    /** factory creating objects from base class by identifier */
    template <typename T, typename C>
    class factory : public factoryBase {
        public:
            /** definition for type holding identifiers and corresponding object creators */
            typedef std::unordered_map<T, factoryCreator<C>*> FactoryMap;

            /** the factories very own iterator class. */
            class Iterator {
                private:
                    typename FactoryMap::const_iterator _it;
                public:
                    Iterator(const typename FactoryMap::const_iterator& it) {
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

                    inline const factoryCreator<C>* operator * () const {
                        return _it->second;
                    }

                    inline const factoryCreator<C>* operator -> () const {
                        return _it->second;
                    }
            };

            /** destructor */
            ~factory() = default;

            /** returns iterator to factory object or end if object cannot be found. */
            Iterator find(const std::string& className) const {
                return Iterator(_factoryMap.find(className));
            }

            /** returns iterator to the beginning of the map. */
            Iterator begin() const {
                return Iterator(_factoryMap.begin());
            }

            /** returns iterator to the end of the map. */
            Iterator end() const {
                return Iterator(_factoryMap.end());
            }

            /** insert a new object and its creator. */
            void insert(T id, factoryCreator<C>* c) {
                this->_factoryMap[id] = c;
            }

            /** creates an object by id */
            C* create(T id) {
                if (this->_factoryMap.find(id) != this->_factoryMap.end()) {
                    return _factoryMap[id]->create();
                } else {
                    throw factoryException();
                }
            }

            /** returns factory size */
            std::size_t size() const {
                return _factoryMap.size();
            }

            /** checks if factory is empty or not */
            bool empty() const {
                return _factoryMap.empty();
            }

            /** returns own type name */
            const char* typeName() const {
                return typeid(*this).name();
            }
        private:
            /** map holding identifiers and object creators. */
            FactoryMap _factoryMap;
    };
}

#endif	/* _MODULEPP_MODULE_FACTORY_HPP_ */