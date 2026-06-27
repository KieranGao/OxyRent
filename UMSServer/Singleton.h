#ifndef SINGLETON_H
#define SINGLETON_H

// CRTP 单例基类
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
