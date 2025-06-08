// This .cpp file was added as a workaround for clang-tidy limitations.
// Meson runs clang-tidy directly on headers (.hpp), but the tool relies on
// compile_commands.json, which only defines context for .cpp files.
// This results in invalid diagnostics when headers are processed alone.
// The workaround ensures headers like test_utils.hpp are properly analyzed
// via corresponding .cpp compilation units using HeaderFilterRegex settings.

#include "test_utils.hpp"
