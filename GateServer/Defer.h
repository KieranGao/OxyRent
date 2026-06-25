#ifndef DEFER_H
#define DEFER_H

#include <functional>

// Go-style RAII scope guard: calls func_ on destruction
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
