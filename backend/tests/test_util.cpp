#include <catch2/catch_test_macros.hpp>

#include <system_error>

#include "stora6e/util.hpp"

using namespace stora6e;

TEST_CASE("permission-denied is a benign filesystem error", "[util]") {
  REQUIRE(isBenignFilesystemError(std::make_error_code(std::errc::permission_denied)));
}

TEST_CASE("no-such-file is a benign filesystem error", "[util]") {
  REQUIRE(isBenignFilesystemError(std::make_error_code(std::errc::no_such_file_or_directory)));
}

TEST_CASE("not-a-directory is not a benign filesystem error", "[util]") {
  REQUIRE_FALSE(isBenignFilesystemError(std::make_error_code(std::errc::not_a_directory)));
}

TEST_CASE("filename-too-long is not a benign filesystem error", "[util]") {
  REQUIRE_FALSE(isBenignFilesystemError(std::make_error_code(std::errc::filename_too_long)));
}

TEST_CASE("a default-constructed error_code is falsy (no error occurred)", "[util]") {
  // Scanner::recordError() checks `!ec` before consulting
  // isBenignFilesystemError() at all, so the classifier itself doesn't need
  // to special-case "no error" — this just documents that contract.
  REQUIRE_FALSE(std::error_code());
}
