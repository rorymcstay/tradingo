#ifndef CACHE_H
#define CACHE_H

#include <memory>
#include <vector>
#include <string>
#include <functional>



namespace cache 
{

struct Person
{
    std::string name;
    int age;
    Person()
    :   name("")
    ,   age(0)
    {}
};

struct releaser
{
    void operator() (Person* person_)
    {
        person_->name = "";
        person_->age = 0;
    }
};


template<class T> using Deleter = std::function<void(T*)>;



template<typename T, size_t size_, typename Releaser>
class ObjectPool
{
public:
    using TPtr = std::shared_ptr<T>;
    using Ptr = std::shared_ptr<ObjectPool<T,size_,Releaser>>;
    ObjectPool();
    ~ObjectPool() {};
    std::shared_ptr<T> get();
    size_t size();

private:

    Deleter<T>          _releaser; 
    size_t              _size;
    std::vector<TPtr>    _freeObjectRegistry;
    std::vector<TPtr>    _usedObjectRegistry;
private:
    void initialise();
    void replace(T* ptr_, int pos );

};



} // cache

#include "ObjectPool.hpp"

#endif
