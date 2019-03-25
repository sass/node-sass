#include "../src/util_string.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string escape_string(const std::string& str) {
  std::string out;
  out.reserve(str.size());
  for (char c : str) {
    switch (c) {
      case '\n':
        out.append("\\n");
        break;
      case '\r':
        out.append("\\r");
        break;
      case '\f':
        out.append("\\f");
        break;
      default:
        out += c;
    }
  }
  return out;
}

#define ASSERT_STR_EQ(a, b) \
  if (a != b) { \
    std::cerr << \
      "Expected LHS == RHS at " << __FILE__ << ":" << __LINE__ << \
      "\n  LHS: [" << escape_string(a) << "]" \
      "\n  RHS: [" << escape_string(b) << "]" << \
      std::endl; \
    return false; \
  } \

bool TestNormalizeNewlinesNoNewline() {
  std::string input = "a";
  std::string normalized = Sass::Util::normalize_newlines(input);
  ASSERT_STR_EQ(input, normalized);
  return true;
}

bool TestNormalizeNewlinesLF() {
  std::string input = "a\nb";
  std::string normalized = Sass::Util::normalize_newlines(input);
  ASSERT_STR_EQ(input, normalized);
  return true;
}

bool TestNormalizeNewlinesCR() {
  std::string normalized = Sass::Util::normalize_newlines("a\rb");
  ASSERT_STR_EQ("a\nb", normalized);
  return true;
}

bool TestNormalizeNewlinesCRLF() {
  std::string normalized = Sass::Util::normalize_newlines("a\r\nb\r\n");
  ASSERT_STR_EQ("a\nb\n", normalized);
  return true;
}

bool TestNormalizeNewlinesFF() {
  std::string normalized = Sass::Util::normalize_newlines("a\fb\f");
  ASSERT_STR_EQ("a\nb\n", normalized);
  return true;
}

bool TestNormalizeNewlinesMixed() {
  std::string normalized = Sass::Util::normalize_newlines("a\fb\nc\rd\r\ne\ff");
  ASSERT_STR_EQ("a\nb\nc\nd\ne\nf", normalized);
  return true;
}

bool TestNormalizeUnderscores() {
  std::string normalized = Sass::Util::normalize_underscores("a_b_c");
  ASSERT_STR_EQ("a-b-c", normalized);
  return true;
}

bool TestNormalizeDecimalsLeadingZero() {
  std::string normalized = Sass::Util::normalize_decimals("0.5");
  ASSERT_STR_EQ("0.5", normalized);
  return true;
}

bool TestNormalizeDecimalsNoLeadingZero() {
  std::string normalized = Sass::Util::normalize_decimals(".5");
  ASSERT_STR_EQ("0.5", normalized);
  return true;
}

}  // namespace

#define TEST(fn) \
  if (fn()) { \
    passed.push_back(#fn); \
  } else { \
    failed.push_back(#fn); \
    std::cerr << "Failed: " #fn << std::endl; \
  } \

int main(int argc, char **argv) {
  std::vector<std::string> passed;
  std::vector<std::string> failed;
  TEST(TestNormalizeNewlinesNoNewline);
  TEST(TestNormalizeNewlinesLF);
  TEST(TestNormalizeNewlinesCR);
  TEST(TestNormalizeNewlinesCRLF);
  TEST(TestNormalizeNewlinesFF);
  TEST(TestNormalizeNewlinesMixed);
  TEST(TestNormalizeUnderscores);
  TEST(TestNormalizeDecimalsLeadingZero);
  TEST(TestNormalizeDecimalsNoLeadingZero);
  std::cerr << argv[0] << ": Passed: " << passed.size()
            << ", failed: " << failed.size()
            << "." << std::endl;
  return failed.size();
}
