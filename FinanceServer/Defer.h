#ifndef DEFER_H
#define DEFER_H

#include <functional>

// Go风格的RAII作用域守卫：析构时调用func_
class Defer {
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer() {
        func_();
    }
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
    Defer(Defer&&) = default;
    Defer& operator=(Defer&&) = default;
private:
    std::function<void()> func_;
};

#endif /* DEFER_H */
