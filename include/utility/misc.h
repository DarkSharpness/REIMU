#pragma once

namespace dark {

template <typename... _Args>
static void allow_unused(_Args &&...) {}

} // namespace dark
