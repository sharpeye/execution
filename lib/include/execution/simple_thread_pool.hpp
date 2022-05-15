#pragma once

#include "simple_thread_pool_impl.hpp"
#include "simple_thread_pool_scheduler.hpp"

namespace execution {

////////////////////////////////////////////////////////////////////////////////

using simple_thread_pool = simple_thread_pool_impl::thread_pool;

}   // namespace execution
