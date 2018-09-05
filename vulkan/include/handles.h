#pragma once

#include <memory>

#define DECLARE_HANDLE(Object) \
    class Object; \
    using Object##Handle = std::shared_ptr<Object>;

DECLARE_HANDLE(Shader);
