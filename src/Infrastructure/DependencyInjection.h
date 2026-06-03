#pragma once

#include <memory>

#include "src/Infrastructure/Log.h"

namespace Infrastructure {
    class DependencyInjection {
    public:
        template<typename T>
        static std::shared_ptr<T> Resolve() {
            return nullptr;
        }

    private:
        static std::shared_ptr<Log> log;
    };

    template<>
    inline std::shared_ptr<Log> DependencyInjection::Resolve<Log>() {
        if (DependencyInjection::log == nullptr) {
            DependencyInjection::log = std::make_shared<Log>();
        }
        return DependencyInjection::log;
    }
};

using InfrastructureDependencies = Infrastructure::DependencyInjection;
