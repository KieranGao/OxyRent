#ifndef SINGLETON_H
#define SINGLETON_H

// CRTP Singleton base class
template<class T>
class Singleton {

protected:
    Singleton() = default;
    ~Singleton() = default;
    Singleton<T>& operator=(const Singleton<T>&) = delete;
    Singleton(const Singleton<T>&) = delete;

public:
    static T& getInstance() {
        static T instance;
        return instance;
    }
};

#endif /* SINGLETON_H */
