// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Authors: vladl@google.com (Vlad Losev), wan@google.com (Zhanyong Wan)
//
// This file tests the internal cross-platform support utilities.

#include "gtest/internal/gtest-port.h"

#include <stdio.h>

#if GTEST_OS_MAC
# include <time.h>
#endif  // GTEST_OS_MAC

#include <list>
#include <utility>  // For std::pair and std::make_pair.
#include <vector>

#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

// Indicates that this translation unit is part of Google Test's
// implementation.  It must come before gtest-internal-inl.h is
// included, or there will be a compiler error.  This trick is to
// prevent a user from accidentally including gtest-internal-inl.h in
// his code.
#define GTEST_IMPLEMENTATION_ 1
#include "src/gtest-internal-inl.h"
#undef GTEST_IMPLEMENTATION_

using std::make_pair;
using std::pair;

namespace testing {
namespace internal {

class Base {
 public:
  // Copy constructor and assignment operator do exactly what we need, so we
  // use them.
  Base() : member_(0) {}
  explicit Base(int n) : member_(n) {}
  virtual ~Base() {}
  int member() { return member_; }

 private:
  int member_;
};

class Derived : public Base {
 public:
  explicit Derived(int n) : Base(n) {}
};

TEST(ImplicitCastTest, ConvertsPointers) {
  Derived derived(0);
  EXPECT_TRUE(&derived == ::testing::internal::ImplicitCast_<Base*>(&derived));
}

TEST(ImplicitCastTest, CanUseInheritance) {
  Derived derived(1);
  Base base = ::testing::internal::ImplicitCast_<Base>(derived);
  EXPECT_EQ(derived.member(), base.member());
}

class Castable {
 public:
  Castable(bool* converted) : converted_(converted) {}
  operator Base() {
    *converted_ = true;
    return Base();
  }

 private:
  bool* converted_;
};

TEST(ImplicitCastTest, CanUseNonConstCastOperator) {
  bool converted = false;
  Castable castable(&converted);
  Base base = ::testing::internal::ImplicitCast_<Base>(castable);
  EXPECT_TRUE(converted);
}

class ConstCastable {
 public:
  ConstCastable(bool* converted) : converted_(converted) {}
  operator Base() const {
    *converted_ = true;
    return Base();
  }

 private:
  bool* converted_;
};

TEST(ImplicitCastTest, CanUseConstCastOperatorOnConstValues) {
  bool converted = false;
  const ConstCastable const_castable(&converted);
  Base base = ::testing::internal::ImplicitCast_<Base>(const_castable);
  EXPECT_TRUE(converted);
}

class ConstAndNonConstCastable {
 public:
  ConstAndNonConstCastable(bool* converted, bool* const_converted)
      : converted_(converted), const_converted_(const_converted) {}
  operator Base() {
    *converted_ = true;
    return Base();
  }
  operator Base() const {
    *const_converted_ = true;
    return Base();
  }

 private:
  bool* converted_;
  bool* const_converted_;
};

TEST(ImplicitCastTest, CanSelectBetweenConstAndNonConstCasrAppropriately) {
  bool converted = false;
  bool const_converted = false;
  ConstAndNonConstCastable castable(&converted, &const_converted);
  Base base = ::testing::internal::ImplicitCast_<Base>(castable);
  EXPECT_TRUE(converted);
  EXPECT_FALSE(const_converted);

  converted = false;
  const_converted = false;
  const ConstAndNonConstCastable const_castable(&converted, &const_converted);
  base = ::testing::internal::ImplicitCast_<Base>(const_castable);
  EXPECT_FALSE(converted);
  EXPECT_TRUE(const_converted);
}

class To {
 public:
  To(bool* converted) { *converted = true; }  // NOLINT
};

TEST(ImplicitCastTest, CanUseImplicitConstructor) {
  bool converted = false;
  To to = ::testing::internal::ImplicitCast_<To>(&converted);
  (void)to;
  EXPECT_TRUE(converted);
}

TEST(IteratorTraitsTest, WorksForSTLContainerIterators) {
  StaticAssertTypeEq<int,
      IteratorTraits< ::std::vector<int>::const_iterator>::value_type>();
  StaticAssertTypeEq<bool,
      IteratorTraits< ::std::list<bool>::iterator>::value_type>();
}

TEST(IteratorTraitsTest, WorksForPointerToNonConst) {
  StaticAssertTypeEq<char, IteratorTraits<char*>::value_type>();
  StaticAssertTypeEq<const void*, IteratorTraits<const void**>::value_type>();
}

TEST(IteratorTraitsTest, WorksForPointerToConst) {
  StaticAssertTypeEq<char, IteratorTraits<const char*>::value_type>();
  StaticAssertTypeEq<const void*,
      IteratorTraits<const void* const*>::value_type>();
}

// Tests that the element_type typedef is available in scoped_ptr and refers
// to the parameter type.
TEST(ScopedPtrTest, DefinesElementType) {
  StaticAssertTypeEq<int, ::testing::internal::scoped_ptr<int>::element_type>();
}

// TODO(vladl@google.com): Implement THE REST of scoped_ptr tests.

TEST(GtestCheckSyntaxTest, BehavesLikeASingleStatement) {
  if (AlwaysFalse())
    GTEST_CHECK_(false) << "This should never be executed; "
                           "It's a compilation test only.";

  if (AlwaysTrue())
    GTEST_CHECK_(true);
  else
    ;  // NOLINT

  if (AlwaysFalse())
    ;  // NOLINT
  else
    GTEST_CHECK_(true) << "";
}

TEST(GtestCheckSyntaxTest, WorksWithSwitch) {
  switch (0) {
    case 1:
      break;
    default:
      GTEST_CHECK_(true);
  }

  switch(0)
    case 0:
      GTEST_CHECK_(true) << "Check failed in switch case";
}

// Verifies behavior of FormatFileLocation.
TEST(FormatFileLocationTest, FormatsFileLocation) {
  EXPECT_PRED_FORMAT2(IsSubstring, "foo.cc", FormatFileLocation("foo.cc", 42));
  EXPECT_PRED_FORMAT2(IsSubstring, "42", FormatFileLocation("foo.cc", 42));
}

TEST(FormatFileLocationTest, FormatsUnknownFile) {
  EXPECT_PRED_FORMAT2(
      IsSubstring, "unknown file", FormatFileLocation(NULL, 42));
  EXPECT_PRED_FORMAT2(IsSubstring, "42", FormatFileLocation(NULL, 42));
}

TEST(FormatFileLocationTest, FormatsUknownLine) {
  EXPECT_EQ("foo.cc:", FormatFileLocation("foo.cc", -1));
}

TEST(FormatFileLocationTest, FormatsUknownFileAndLine) {
  EXPECT_EQ("unknown file:", FormatFileLocation(NULL, -1));
}

// Verifies behavior of FormatCompilerIndependentFileLocation.
TEST(FormatCompilerIndependentFileLocationTest, FormatsFileLocation) {
  EXPECT_EQ("foo.cc:42", FormatCompilerIndependentFileLocation("foo.cc", 42));
}

TEST(FormatCompilerIndependentFileLocationTest, FormatsUknownFile) {
  EXPECT_EQ("unknown file:42",
            FormatCompilerIndependentFileLocation(NULL, 42));
}

TEST(FormatCompilerIndependentFileLocationTest, FormatsUknownLine) {
  EXPECT_EQ("foo.cc", FormatCompilerIndependentFileLocation("foo.cc", -1));
}

TEST(FormatCompilerIndependentFileLocationTest, FormatsUknownFileAndLine) {
  EXPECT_EQ("unknown file", FormatCompilerIndependentFileLocation(NULL, -1));
}

#if GTEST_OS_MAC
void* ThreadFunc(void* data) {
  pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(data);
  pthread_mutex_lock(mutex);
  pthread_mutex_unlock(mutex);
  return NULL;
}

TEST(GetThreadCountTest, ReturnsCorrectValue) {
  EXPECT_EQ(1U, GetThreadCount());
  pthread_mutex_t mutex;
  pthread_attr_t  attr;
  pthread_t       thread_id;

  // TODO(vladl@google.com): turn mutex into internal::Mutex for automatic
  // destruction.
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_lock(&mutex);
  ASSERT_EQ(0, pthread_attr_init(&attr));
  ASSERT_EQ(0, pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE));

  const int status = pthread_create(&thread_id, &attr, &ThreadFunc, &mutex);
  ASSERT_EQ(0, pthread_attr_destroy(&attr));
  ASSERT_EQ(0, status);
  EXPECT_EQ(2U, GetThreadCount());
  pthread_mutex_unlock(&mutex);

  void* dummy;
  ASSERT_EQ(0, pthread_join(thread_id, &dummy));

  // MacOS X may not immediately report the updated thread count after
  // joining a thread, causing flakiness in this test. To counter that, we
  // wait for up to .5 seconds for the OS to report the correct value.
  for (int i = 0; i < 5; ++i) {
    if (GetThreadCount() == 1)
      break;

    SleepMilliseconds(100);
  }
  EXPECT_EQ(1U, GetThreadCount());
  pthread_mutex_destroy(&mutex);
}
#else
TEST(GetThreadCountTest, ReturnsZeroWhenUnableToCountThreads) {
  EXPECT_EQ(0U, GetThreadCount());
}
#endif  // GTEST_OS_MAC

TEST(GtestCheckDeathTest, DiesWithCorrectOutputOnFailure) {
  const bool a_false_condition = false;
  const char regex[] =
#ifdef _MSC_VER
     "gtest-port_test\\.cc\\(\\d+\\):"
#elif GTEST_USES_POSIX_RE
     "gtest-port_test\\.cc:[0-9]+"
#else
     "gtest-port_test\\.cc:\\d+"
#endif  // _MSC_VER
     ".*a_false_condition.*Extra info.*";

  EXPECT_DEATH_IF_SUPPORTED(GTEST_CHECK_(a_false_condition) << "Extra info",
                            regex);
}

#if GTEST_HAS_DEATH_TEST

TEST(GtestCheckDeathTest, LivesSilentlyOnSuccess) {
  EXPECT_EXIT({
      GTEST_CHECK_(true) << "Extra info";
      ::std::cerr << "Success\n";
      exit(0); },
      ::testing::ExitedWithCode(0), "Success");
}

#endif  // GTEST_HAS_DEATH_TEST

// Verifies that Google Test choose regular expression engine appropriate to
// the platform. The test will produce compiler errors in case of failure.
// For simplicity, we only cover the most important platforms here.
TEST(RegexEngineSelectionTest, SelectsCorrectRegexEngine) {
#if GTEST_HAS_POSIX_RE

  EXPECT_TRUE(GTEST_USES_POSIX_RE);

#else

  EXPECT_TRUE(GTEST_USES_SIMPLE_RE);

#endif
}

#if GTEST_USES_POSIX_RE

# if GTEST_HAS_TYPED_TEST

template <typename Str>
class RETest : public ::testing::Test {};

// Defines StringTypes as the list of all string types that class RE
// supports.
typedef testing::Types<
    ::std::string,
#  if GTEST_HAS_GLOBAL_STRING
    ::string,
#  endif  // GTEST_HAS_GLOBAL_STRING
    const char*> StringTypes;

TYPED_TEST_CASE(RETest, StringTypes);

// Tests RE's implicit constructors.
TYPED_TEST(RETest, ImplicitConstructorWorks) {
  const RE empty(TypeParam(""));
  EXPECT_STREQ("", empty.pattern());

  const RE simple(TypeParam("hello"));
  EXPECT_STREQ("hello", simple.pattern());

  const RE normal(TypeParam(".*(\\w+)"));
  EXPECT_STREQ(".*(\\w+)", normal.pattern());
}

// Tests that RE's constructors reject invalid regular expressions.
TYPED_TEST(RETest, RejectsInvalidRegex) {
  EXPECT_NONFATAL_FAILURE({
    const RE invalid(TypeParam("?"));
  }, "\"?\" is not a valid POSIX Extended regular expression.");
}

// Tests RE::FullMatch().
TYPED_TEST(RETest, FullMatchWorks) {
  const RE empty(TypeParam(""));
  EXPECT_TRUE(RE::FullMatch(TypeParam(""), empty));
  EXPECT_FALSE(RE::FullMatch(TypeParam("a"), empty));

  const RE re(TypeParam("a.*z"));
  EXPECT_TRUE(RE::FullMatch(TypeParam("az"), re));
  EXPECT_TRUE(RE::FullMatch(TypeParam("axyz"), re));
  EXPECT_FALSE(RE::FullMatch(TypeParam("baz"), re));
  EXPECT_FALSE(RE::FullMatch(TypeParam("azy"), re));
}

// Tests RE::PartialMatch().
TYPED_TEST(RETest, PartialMatchWorks) {
  const RE empty(TypeParam(""));
  EXPECT_TRUE(RE::PartialMatch(TypeParam(""), empty));
  EXPECT_TRUE(RE::PartialMatch(TypeParam("a"), empty));

  const RE re(TypeParam("a.*z"));
  EXPECT_TRUE(RE::PartialMatch(TypeParam("az"), re));
  EXPECT_TRUE(RE::PartialMatch(TypeParam("axyz"), re));
  EXPECT_TRUE(RE::PartialMatch(TypeParam("baz"), re));
  EXPECT_TRUE(RE::PartialMatch(TypeParam("azy"), re));
  EXPECT_FALSE(RE::PartialMatch(TypeParam("zza"), re));
}

# endif  // GTEST_HAS_TYPED_TEST

#elif GTEST_USES_SIMPLE_RE

TEST(IsInSetTest, NulCharIsNotInAnySet) {
  EXPECT_FALSE(IsInSet('\0', ""));
  EXPECT_FALSE(IsInSet('\0', "\0"));
  EXPECT_FALSE(IsInSet('\0', "a"));
}

TEST(IsInSetTest, WorksForNonNulChars) {
  EXPECT_FALSE(IsInSet('a', "Ab"));
  EXPECT_FALSE(IsInSet('c', ""));

  EXPECT_TRUE(IsInSet('b', "bcd"));
  EXPECT_TRUE(IsInSet('b', "ab"));
}

TEST(IsAsciiDigitTest, IsFalseForNonDigit) {
  EXPECT_FALSE(IsAsciiDigit('\0'));
  EXPECT_FALSE(IsAsciiDigit(' '));
  EXPECT_FALSE(IsAsciiDigit('+'));
  EXPECT_FALSE(IsAsciiDigit('-'));
  EXPECT_FALSE(IsAsciiDigit('.'));
  EXPECT_FALSE(IsAsciiDigit('a'));
}

TEST(IsAsciiDigitTest, IsTrueForDigit) {
  EXPECT_TRUE(IsAsciiDigit('0'));
  EXPECT_TRUE(IsAsciiDigit('1'));
  EXPECT_TRUE(IsAsciiDigit('5'));
  EXPECT_TRUE(IsAsciiDigit('9'));
}

TEST(IsAsciiPunctTest, IsFalseForNonPunct) {
  EXPECT_FALSE(IsAsciiPunct('\0'));
  EXPECT_FALSE(IsAsciiPunct(' '));
  EXPECT_FALSE(IsAsciiPunct('\n'));
  EXPECT_FALSE(IsAsciiPunct('a'));
  EXPECT_FALSE(IsAsciiPunct('0'));
}

TEST(IsAsciiPunctTest, IsTrueForPunct) {
  for (const char* p = "^-!\"#$%&'()*+,./:;<=>?@[\\]_`{|}~"; *p; p++) {
    EXPECT_PRED1(IsAsciiPunct, *p);
  }
}

TEST(IsRepeatTest, IsFalseForNonRepeatChar) {
  EXPECT_FALSE(IsRepeat('\0'));
  EXPECT_FALSE(IsRepeat(' '));
  EXPECT_FALSE(IsRepeat('a'));
  EXPECT_FALSE(IsRepeat('1'));
  EXPECT_FALSE(IsRepeat('-'));
}

TEST(IsRepeatTest, IsTrueForRepeatChar) {
  EXPECT_TRUE(IsRepeat('?'));
  EXPECT_TRUE(IsRepeat('*'));
  EXPECT_TRUE(IsRepeat('+'));
}

TEST(IsAsciiWhiteSpaceTest, IsFalseForNonWhiteSpace) {
  EXPECT_FALSE(IsAsciiWhiteSpace('\0'));
  EXPECT_FALSE(IsAsciiWhiteSpace('a'));
  EXPECT_FALSE(IsAsciiWhiteSpace('1'));
  EXPECT_FALSE(IsAsciiWhiteSpace('+'));
  EXPECT_FALSE(IsAsciiWhiteSpace('_'));
}

TEST(IsAsciiWhiteSpaceTest, IsTrueForWhiteSpace) {
  EXPECT_TRUE(IsAsciiWhiteSpace(' '));
  EXPECT_TRUE(IsAsciiWhiteSpace('\n'));
  EXPECT_TRUE(IsAsciiWhiteSpace('\r'));
  EXPECT_TRUE(IsAsciiWhiteSpace('\t'));
  EXPECT_TRUE(IsAsciiWhiteSpace('\v'));
  EXPECT_TRUE(IsAsciiWhiteSpace('\f'));
}

TEST(IsAsciiWordCharTest, IsFalseForNonWordChar) {
  EXPECT_FALSE(IsAsciiWordChar('\0'));
  EXPECT_FALSE(IsAsciiWordChar('+'));
  EXPECT_FALSE(IsAsciiWordChar('.'));
  EXPECT_FALSE(IsAsciiWordChar(' '));
  EXPECT_FALSE(IsAsciiWordChar('\n'));
}

TEST(IsAsciiWordCharTest, IsTrueForLetter) {
  EXPECT_TRUE(IsAsciiWordChar('a'));
  EXPECT_TRUE(IsAsciiWordChar('b'));
  EXPECT_TRUE(IsAsciiWordChar('A'));
  EXPECT_TRUE(IsAsciiWordChar('Z'));
}

TEST(IsAsciiWordCharTest, IsTrueForDigit) {
  EXPECT_TRUE(IsAsciiWordChar('0'));
  EXPECT_TRUE(IsAsciiWordChar('1'));
  EXPECT_TRUE(IsAsciiWordChar('7'));
  EXPECT_TRUE(IsAsciiWordChar('9'));
}

TEST(IsAsciiWordCharTest, IsTrueForUnderscore) {
  EXPECT_TRUE(IsAsciiWordChar('_'));
}

TEST(IsValidEscapeTest, IsFalseForNonPrintable) {
  EXPECT_FALSE(IsValidEscape('\0'));
  EXPECT_FALSE(IsValidEscape('\007'));
}

TEST(IsValidEscapeTest, IsFalseForDigit) {
  EXPECT_FALSE(IsValidEscape('0'));
  EXPECT_FALSE(IsValidEscape('9'));
}

TEST(IsValidEscapeTest, IsFalseForWhiteSpace) {
  EXPECT_FALSE(IsValidEscape(' '));
  EXPECT_FALSE(IsValidEscape('\n'));
}

TEST(IsValidEscapeTest, IsFalseForSomeLetter) {
  EXPECT_FALSE(IsValidEscape('a'));
  EXPECT_FALSE(IsValidEscape('Z'));
}

TEST(IsValidEscapeTest, IsTrueForPunct) {
  EXPECT_TRUE(IsValidEscape('.'));
  EXPECT_TRUE(IsValidEscape('-'));
  EXPECT_TRUE(IsValidEscape('^'));
  EXPECT_TRUE(IsValidEscape('$'));
  EXPECT_TRUE(IsValidEscape('('));
  EXPECT_TRUE(IsValidEscape(']'));
  EXPECT_TRUE(IsValidEscape('{'));
  EXPECT_TRUE(IsValidEscape('|'));
}

TEST(IsValidEscapeTest, IsTrueForSomeLetter) {
  EXPECT_TRUE(IsValidEscape('d'));
  EXPECT_TRUE(IsValidEscape('D'));
  EXPECT_TRUE(IsValidEscape('s'));
  EXPECT_TRUE(IsValidEscape('S'));
  EXPECT_TRUE(IsValidEscape('w'));
  EXPECT_TRUE(IsValidEscape('W'));
}

TEST(AtomMatchesCharTest, EscapedPunct) {
  EXPECT_FALSE(AtomMatchesChar(true, '\\', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, '\\', ' '));
  EXPECT_FALSE(AtomMatchesChar(true, '_', '.'));
  EXPECT_FALSE(AtomMatchesChar(true, '.', 'a'));

  EXPECT_TRUE(AtomMatchesChar(true, '\\', '\\'));
  EXPECT_TRUE(AtomMatchesChar(true, '_', '_'));
  EXPECT_TRUE(AtomMatchesChar(true, '+', '+'));
  EXPECT_TRUE(AtomMatchesChar(true, '.', '.'));
}

TEST(AtomMatchesCharTest, Escaped_d) {
  EXPECT_FALSE(AtomMatchesChar(true, 'd', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'd', 'a'));
  EXPECT_FALSE(AtomMatchesChar(true, 'd', '.'));

  EXPECT_TRUE(AtomMatchesChar(true, 'd', '0'));
  EXPECT_TRUE(AtomMatchesChar(true, 'd', '9'));
}

TEST(AtomMatchesCharTest, Escaped_D) {
  EXPECT_FALSE(AtomMatchesChar(true, 'D', '0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'D', '9'));

  EXPECT_TRUE(AtomMatchesChar(true, 'D', '\0'));
  EXPECT_TRUE(AtomMatchesChar(true, 'D', 'a'));
  EXPECT_TRUE(AtomMatchesChar(true, 'D', '-'));
}

TEST(AtomMatchesCharTest, Escaped_s) {
  EXPECT_FALSE(AtomMatchesChar(true, 's', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 's', 'a'));
  EXPECT_FALSE(AtomMatchesChar(true, 's', '.'));
  EXPECT_FALSE(AtomMatchesChar(true, 's', '9'));

  EXPECT_TRUE(AtomMatchesChar(true, 's', ' '));
  EXPECT_TRUE(AtomMatchesChar(true, 's', '\n'));
  EXPECT_TRUE(AtomMatchesChar(true, 's', '\t'));
}

TEST(AtomMatchesCharTest, Escaped_S) {
  EXPECT_FALSE(AtomMatchesChar(true, 'S', ' '));
  EXPECT_FALSE(AtomMatchesChar(true, 'S', '\r'));

  EXPECT_TRUE(AtomMatchesChar(true, 'S', '\0'));
  EXPECT_TRUE(AtomMatchesChar(true, 'S', 'a'));
  EXPECT_TRUE(AtomMatchesChar(true, 'S', '9'));
}

TEST(AtomMatchesCharTest, Escaped_w) {
  EXPECT_FALSE(AtomMatchesChar(true, 'w', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'w', '+'));
  EXPECT_FALSE(AtomMatchesChar(true, 'w', ' '));
  EXPECT_FALSE(AtomMatchesChar(true, 'w', '\n'));

  EXPECT_TRUE(AtomMatchesChar(true, 'w', '0'));
  EXPECT_TRUE(AtomMatchesChar(true, 'w', 'b'));
  EXPECT_TRUE(AtomMatchesChar(true, 'w', 'C'));
  EXPECT_TRUE(AtomMatchesChar(true, 'w', '_'));
}

TEST(AtomMatchesCharTest, Escaped_W) {
  EXPECT_FALSE(AtomMatchesChar(true, 'W', 'A'));
  EXPECT_FALSE(AtomMatchesChar(true, 'W', 'b'));
  EXPECT_FALSE(AtomMatchesChar(true, 'W', '9'));
  EXPECT_FALSE(AtomMatchesChar(true, 'W', '_'));

  EXPECT_TRUE(AtomMatchesChar(true, 'W', '\0'));
  EXPECT_TRUE(AtomMatchesChar(true, 'W', '*'));
  EXPECT_TRUE(AtomMatchesChar(true, 'W', '\n'));
}

TEST(AtomMatchesCharTest, EscapedWhiteSpace) {
  EXPECT_FALSE(AtomMatchesChar(true, 'f', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'f', '\n'));
  EXPECT_FALSE(AtomMatchesChar(true, 'n', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'n', '\r'));
  EXPECT_FALSE(AtomMatchesChar(true, 'r', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'r', 'a'));
  EXPECT_FALSE(AtomMatchesChar(true, 't', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 't', 't'));
  EXPECT_FALSE(AtomMatchesChar(true, 'v', '\0'));
  EXPECT_FALSE(AtomMatchesChar(true, 'v', '\f'));

  EXPECT_TRUE(AtomMatchesChar(true, 'f', '\f'));
  EXPECT_TRUE(AtomMatchesChar(true, 'n', '\n'));
  EXPECT_TRUE(AtomMatchesChar(true, 'r', '\r'));
  EXPECT_TRUE(AtomMatchesChar(true, 't', '\t'));
  EXPECT_TRUE(AtomMatchesChar(true, 'v', '\v'));
}

TEST(AtomMatchesCharTest, UnescapedDot) {
  EXPECT_FALSE(AtomMatchesChar(false, '.', '\n'));

  EXPECT_TRUE(AtomMatchesChar(false, '.', '\0'));
  EXPECT_TRUE(AtomMatchesChar(false, '.', '.'));
  EXPECT_TRUE(AtomMatchesChar(false, '.', 'a'));
  EXPECT_TRUE(AtomMatchesChar(false, '.', ' '));
}

TEST(AtomMatchesCharTest, UnescapedChar) {
  EXPECT_FALSE(AtomMatchesChar(false, 'a', '\0'));
  EXPECT_FALSE(AtomMatchesChar(false, 'a', 'b'));
  EXPECT_FALSE(AtomMatchesChar(false, '$', 'a'));

  EXPECT_TRUE(AtomMatchesChar(false, '$', '$'));
  EXPECT_TRUE(AtomMatchesChar(false, '5', '5'));
  EXPECT_TRUE(AtomMatchesChar(false, 'Z', 'Z'));
}

TEST(ValidateRegexTest, GeneratesFailureAndReturnsFalseForInvalid) {
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex(NULL)),
                          "NULL is not a valid simple regular expression");
  EXPECT_NONFATAL_FAILURE(
      ASSERT_FALSE(ValidateRegex("a\\")),
      "Syntax error at index 1 in simple regular expression \"a\\\": ");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("a\\")),
                          "'\\' cannot appear at the end");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("\\n\\")),
                          "'\\' cannot appear at the end");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("\\s\\hb")),
                          "invalid escape sequence \"\\h\"");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("^^")),
                          "'^' can only appear at the beginning");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex(".*^b")),
                          "'^' can only appear at the beginning");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("$$")),
                          "'$' can only appear at the end");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("^$a")),
                          "'$' can only appear at the end");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("a(b")),
                          "'(' is unsupported");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("ab)")),
                          "')' is unsupported");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("[ab")),
                          "'[' is unsupported");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("a{2")),
                          "'{' is unsupported");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("?")),
                          "'?' can only follow a repeatable token");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("^*")),
                          "'*' can only follow a repeatable token");
  EXPECT_NONFATAL_FAILURE(ASSERT_FALSE(ValidateRegex("5*+")),
                          "'+' can only follow a repeatable token");
}

TEST(ValidateRegexTest, ReturnsTrueForValid) {
  EXPECT_TRUE(ValidateRegex(""));
  EXPECT_TRUE(ValidateRegex("a"));
  EXPECT_TRUE(ValidateRegex(".*"));
  EXPECT_TRUE(ValidateRegex("^a_+"));
  EXPECT_TRUE(ValidateRegex("^a\\t\\&?"));
  EXPECT_TRUE(ValidateRegex("09*$"));
  EXPECT_TRUE(ValidateRegex("^Z$"));
  EXPECT_TRUE(ValidateRegex("a\\^Z\\$\\(\\)\\|\\[\\]\\{\\}"));
}

TEST(MatchRepetitionAndRegexAtHeadTest, WorksForZeroOrOne) {
  EXPECT_FALSE(MatchRepetitionAndRegexAtHead(false, 'a', '?', "a", "ba"));
  // Repeating more than once.
  EXPECT_FALSE(MatchRepetitionAndRegexAtHead(false, 'a', '?', "b", "aab"));

  // Repeating zero times.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(false, 'a', '?', "b", "ba"));
  // Repeating once.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(false, 'a', '?', "b", "ab"));
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(false, '#', '?', ".", "##"));
}

TEST(MatchRepetitionAndRegexAtHeadTest, WorksForZeroOrMany) {
  EXPECT_FALSE(MatchRepetitionAndRegexAtHead(false, '.', '*', "a$", "baab"));

  // Repeating zero times.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(false, '.', '*', "b", "bc"));
  // Repeating once.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(false, '.', '*', "b", "abc"));
  // Repeating more than once.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(true, 'w', '*', "-", "ab_1-g"));
}

TEST(MatchRepetitionAndRegexAtHeadTest, WorksForOneOrMany) {
  EXPECT_FALSE(MatchRepetitionAndRegexAtHead(false, '.', '+', "a$", "baab"));
  // Repeating zero times.
  EXPECT_FALSE(MatchRepetitionAndRegexAtHead(false, '.', '+', "b", "bc"));

  // Repeating once.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(false, '.', '+', "b", "abc"));
  // Repeating more than once.
  EXPECT_TRUE(MatchRepetitionAndRegexAtHead(true, 'w', '+', "-", "ab_1-g"));
}

TEST(MatchRegexAtHeadTest, ReturnsTrueForEmptyRegex) {
  EXPECT_TRUE(MatchRegexAtHead("", ""));
  EXPECT_TRUE(MatchRegexAtHead("", "ab"));
}

TEST(MatchRegexAtHeadTest, WorksWhenDollarIsInRegex) {
  EXPECT_FALSE(MatchRegexAtHead("$", "a"));

  EXPECT_TRUE(MatchRegexAtHead("$", ""));
  EXPECT_TRUE(MatchRegexAtHead("a$", "a"));
}

TEST(MatchRegexAtHeadTest, WorksWhenRegexStartsWithEscapeSequence) {
  EXPECT_FALSE(MatchRegexAtHead("\\w", "+"));
  EXPECT_FALSE(MatchRegexAtHead("\\W", "ab"));

  EXPECT_TRUE(MatchRegexAtHead("\\sa", "\nab"));
  EXPECT_TRUE(MatchRegexAtHead("\\d", "1a"));
}

TEST(MatchRegexAtHeadTest, WorksWhenRegexStartsWithRepetition) {
  EXPECT_FALSE(MatchRegexAtHead(".+a", "abc"));
  EXPECT_FALSE(MatchRegexAtHead("a?b", "aab"));

  EXPECT_TRUE(MatchRegexAtHead(".*a", "bc12-ab"));
  EXPECT_TRUE(MatchRegexAtHead("a?b", "b"));
  EXPECT_TRUE(MatchRegexAtHead("a?b", "ab"));
}

TEST(MatchRegexAtHeadTest,
     WorksWhenRegexStartsWithRepetionOfEscapeSequence) {
  EXPECT_FALSE(MatchRegexAtHead("\\.+a", "abc"));
  EXPECT_FALSE(MatchRegexAtHead("\\s?b", "  b"));

  EXPECT_TRUE(MatchRegexAtHead("\\(*a", "((((ab"));
  EXPECT_TRUE(MatchRegexAtHead("\\^?b", "^b"));
  EXPECT_TRUE(MatchRegexAtHead("\\\\?b", "b"));
  EXPECT_TRUE(MatchRegexAtHead("\\\\?b", "\\b"));
}

TEST(MatchRegexAtHeadTest, MatchesSequentially) {
  EXPECT_FALSE(MatchRegexAtHead("ab.*c", "acabc"));

  EXPECT_TRUE(MatchRegexAtHead("ab.*c", "ab-fsc"));
}

TEST(MatchRegexAnywhereTest, ReturnsFalseWhenStringIsNull) {
  EXPECT_FALSE(MatchRegexAnywhere("", NULL));
}

TEST(MatchRegexAnywhereTest, WorksWhenRegexStartsWithCaret) {
  EXPECT_FALSE(MatchRegexAnywhere("^a", "ba"));
  EXPECT_FALSE(MatchRegexAnywhere("^$", "a"));

  EXPECT_TRUE(MatchRegexAnywhere("^a", "ab"));
  EXPECT_TRUE(MatchRegexAnywhere("^", "ab"));
  EXPECT_TRUE(MatchRegexAnywhere("^$", ""));
}

TEST(MatchRegexAnywhereTest, ReturnsFalseWhenNoMatch) {
  EXPECT_FALSE(MatchRegexAnywhere("a", "bcde123"));
  EXPECT_FALSE(MatchRegexAnywhere("a.+a", "--aa88888888"));
}

TEST(MatchRegexAnywhereTest, ReturnsTrueWhenMatchingPrefix) {
  EXPECT_TRUE(MatchRegexAnywhere("\\w+", "ab1_ - 5"));
  EXPECT_TRUE(MatchRegexAnywhere(".*=", "="));
  EXPECT_TRUE(MatchRegexAnywhere("x.*ab?.*bc", "xaaabc"));
}

TEST(MatchRegexAnywhereTest, ReturnsTrueWhenMatchingNonPrefix) {
  EXPECT_TRUE(MatchRegexAnywhere("\\w+", "$$$ ab1_ - 5"));
  EXPECT_TRUE(MatchRegexAnywhere("\\.+=", "=  ...="));
}

// Tests RE's implicit constructors.
TEST(RETest, ImplicitConstructorWorks) {
  const RE empty("");
  EXPECT_STREQ("", empty.pattern());

  const RE simple("hello");
  EXPECT_STREQ("hello", simple.pattern());
}

// Tests that RE's constructors reject invalid regular expressions.
TEST(RETest, RejectsInvalidRegex) {
  EXPECT_NONFATAL_FAILURE({
    const RE normal(NULL);
  }, "NULL is not a valid simple regular expression");

  EXPECT_NONFATAL_FAILURE({
    const RE normal(".*(\\w+");
  }, "'(' is unsupported");

  EXPECT_NONFATAL_FAILURE({
    const RE invalid("^?");
  }, "'?' can only follow a repeatable token");
}

// Tests RE::FullMatch().
TEST(RETest, FullMatchWorks) {
  const RE empty("");
  EXPECT_TRUE(RE::FullMatch("", empty));
  EXPECT_FALSE(RE::FullMatch("a", empty));

  const RE re1("a");
  EXPECT_TRUE(RE::FullMatch("a", re1));

  const RE re("a.*z");
  EXPECT_TRUE(RE::FullMatch("az", re));
  EXPECT_TRUE(RE::FullMatch("axyz", re));
  EXPECT_FALSE(RE::FullMatch("baz", re));
  EXPECT_FALSE(RE::FullMatch("azy", re));
}

// Tests RE::PartialMatch().
TEST(RETest, PartialMatchWorks) {
  const RE empty("");
  EXPECT_TRUE(RE::PartialMatch("", empty));
  EXPECT_TRUE(RE::PartialMatch("a", empty));

  const RE re("a.*z");
  EXPECT_TRUE(RE::PartialMatch("az", re));
  EXPECT_TRUE(RE::PartialMatch("axyz", re));
  EXPECT_TRUE(RE::PartialMatch("baz", re));
  EXPECT_TRUE(RE::PartialMatch("azy", re));
  EXPECT_FALSE(RE::PartialMatch("zza", re));
}

#endif  // GTEST_USES_POSIX_RE

#if !GTEST_OS_WINDOWS_MOBILE

TEST(CaptureTest, CapturesStdout) {
  CaptureStdout();
  fprintf(stdout, "abc");
  EXPECT_STREQ("abc", GetCapturedStdout().c_str());

  CaptureStdout();
  fprintf(stdout, "def%cghi", '\0');
  EXPECT_EQ(::std::string("def\0ghi", 7), ::std::string(GetCapturedStdout()));
}

TEST(CaptureTest, CapturesStderr) {
  CaptureStderr();
  fprintf(stderr, "jkl");
  EXPECT_STREQ("jkl", GetCapturedStderr().c_str());

  CaptureStderr();
  fprintf(stderr, "jkl%cmno", '\0');
  EXPECT_EQ(::std::string("jkl\0mno", 7), ::std::string(GetCapturedStderr()));
}

// Tests that stdout and stderr capture don't interfere with each other.
TEST(CaptureTest, CapturesStdoutAndStderr) {
  CaptureStdout();
  CaptureStderr();
  fprintf(stdout, "pqr");
  fprintf(stderr, "stu");
  EXPECT_STREQ("pqr", GetCapturedStdout().c_str());
  EXPECT_STREQ("stu", GetCapturedStderr().c_str());
}

TEST(CaptureDeathTest, CannotReenterStdoutCapture) {
  CaptureStdout();
  EXPECT_DEATH_IF_SUPPORTED(CaptureStdout();,
                            "Only one stdout capturer can exist at a time");
  GetCapturedStdout();

  // We cannot test stderr capturing using death tests as they use it
  // themselves.
}

#endif  // !GTEST_OS_WINDOWS_MOBILE

TEST(ThreadLocalTest, DefaultConstructorInitializesToDefaultValues) {
  ThreadLocal<int> t1;
  EXPECT_EQ(0, t1.get());

  ThreadLocal<void*> t2;
  EXPECT_TRUE(t2.get() == NULL);
}

TEST(ThreadLocalTest, SingleParamConstructorInitializesToParam) {
  ThreadLocal<int> t1(123);
  EXPECT_EQ(123, t1.get());

  int i = 0;
  ThreadLocal<int*> t2(&i);
  EXPECT_EQ(&i, t2.get());
}

class NoDefaultConstructor {
 public:
  explicit NoDefaultConstructor(const char*) {}
  NoDefaultConstructor(const NoDefaultContructor&) {}
};

TEST(ThreadLocalTest, ValueDefaultConstructorIsNotRequiredForParamVersion) {
  ThreadLocal<NoDefaultConstructor> bar(NoDefaultContructor("foo"));
  bar.pointer();
}

TEST(ThreadLocalTest, GetAndPointerReturnSameValue) {
  ThreadLocal<String> thread_local;

  EXPECT_EQ(thread_local.pointer(), &(thread_local.get()));

  // Verifies the condition still holds after calling set.
  thread_local.set("foo");
  EXPECT_EQ(thread_local.pointer(), &(thread_local.get()));
}

TEST(ThreadLocalTest, PointerAndConstPointerReturnSameValue) {
  ThreadLocal<String> thread_local;
  const ThreadLocal<String>& const_thread_local = thread_local;

  EXPECT_EQ(thread_local.pointer(), const_thread_local.pointer());

  thread_local.set("foo");
  EXPECT_EQ(thread_local.pointer(), const_thread_local.pointer());
}

#if GTEST_IS_THREADSAFE

void AddTwo(int* param) { *param += 2; }

TEST(ThreadWithParamTest, ConstructorExecutesThreadFunc) {
  int i = 40;
  ThreadWithParam<int*> thread(&AddTwo, &i, NULL);
  thread.Join();
  EXPECT_EQ(42, i);
}

TEST(MutexDeathTest, AssertHeldShouldAssertWhenNotLocked) {
  // AssertHeld() is flaky only in the presence of multiple threads accessing
  // the lock. In this case, the test is robust.
  EXPECT_DEATH_IF_SUPPORTED({
    Mutex m;
    { MutexLock lock(&m); }
    m.AssertHeld();
  },
  "thread .*hold");
}

TEST(MutexTest, AssertHeldShouldNotAssertWhenLocked) {
  Mutex m;
  MutexLock lock(&m);
  m.AssertHeld();
}

class AtomicCounterWithMutex {
 public:
  explicit AtomicCounterWithMutex(Mutex* mutex) :
    value_(0), mutex_(mutex), random_(42) {}

  void Increment() {
    MutexLock lock(mutex_);
    int temp = value_;
    {
      // Locking a mutex puts up a memory barrier, preventing reads and
      // writes to value_ rearranged when observed from other threads.
      //
      // We cannot use Mutex and MutexLock here or rely on their memory
      // barrier functionality as we are testing them here.
      pthread_mutex_t memory_barrier_mutex;
      GTEST_CHECK_POSIX_SUCCESS_(
          pthread_mutex_init(&memory_barrier_mutex, NULL));
      GTEST_CHECK_POSIX_SUCCESS_(pthread_mutex_lock(&memory_barrier_mutex));

      SleepMilliseconds(random_.Generate(30));

      GTEST_CHECK_POSIX_SUCCESS_(pthread_mutex_unlock(&memory_barrier_mutex));
    }
    value_ = temp + 1;
  }
  int value() const { return value_; }

 private:
  volatile int value_;
  Mutex* const mutex_;  // Protects value_.
  Random       random_;
};

void CountingThreadFunc(pair<AtomicCounterWithMutex*, int> param) {
  for (int i = 0; i < param.second; ++i)
      param.first->Increment();
}

// Tests that the mutex only lets one thread at a time to lock it.
TEST(MutexTest, OnlyOneThreadCanLockAtATime) {
  Mutex mutex;
  AtomicCounterWithMutex locked_counter(&mutex);

  typedef ThreadWithParam<pair<AtomicCounterWithMutex*, int> > ThreadType;
  const int kCycleCount = 20;
  const int kThreadCount = 7;
  scoped_ptr<ThreadType> counting_threads[kThreadCount];
  Notification threads_can_start;
  // Creates and runs kThreadCount threads that increment locked_counter
  // kCycleCount times each.
  for (int i = 0; i < kThreadCount; ++i) {
    counting_threads[i].reset(new ThreadType(&CountingThreadFunc,
                                             make_pair(&locked_counter,
                                                       kCycleCount),
                                             &threads_can_start));
  }
  threads_can_start.Notify();
  for (int i = 0; i < kThreadCount; ++i)
    counting_threads[i]->Join();

  // If the mutex lets more than one thread to increment the counter at a
  // time, they are likely to encounter a race condition and have some
  // increments overwritten, resulting in the lower then expected counter
  // value.
  EXPECT_EQ(kCycleCount * kThreadCount, locked_counter.value());
}

template <typename T>
void RunFromThread(void (func)(T), T param) {
  ThreadWithParam<T> thread(func, param, NULL);
  thread.Join();
}

void RetrieveThreadLocalValue(pair<ThreadLocal<String>*, String*> param) {
  *param.second = param.first->get();
}

TEST(ThreadLocalTest, ParameterizedConstructorSetsDefault) {
  ThreadLocal<String> thread_local("foo");
  EXPECT_STREQ("foo", thread_local.get().c_str());

  thread_local.set("bar");
  EXPECT_STREQ("bar", thread_local.get().c_str());

  String result;
  RunFromThread(&RetrieveThreadLocalValue, make_pair(&thread_local, &result));
  EXPECT_STREQ("foo", result.c_str());
}

// DestructorTracker keeps track of whether its instances have been
// destroyed.
static std::vector<bool> g_destroyed;

class DestructorTracker {
 public:
  DestructorTracker() : index_(GetNewIndex()) {}
  DestructorTracker(const DestructorTracker& /* rhs */)
      : index_(GetNewIndex()) {}
  ~DestructorTracker() {
    // We never access g_destroyed concurrently, so we don't need to
    // protect the write operation under a mutex.
    g_destroyed[index_] = true;
  }

 private:
  static int GetNewIndex() {
    g_destroyed.push_back(false);
    return g_destroyed.size() - 1;
  }
  const int index_;
};

typedef ThreadLocal<DestructorTracker>* ThreadParam;

void CallThreadLocalGet(ThreadParam thread_local) {
  thread_local->get();
}

// Tests that when a ThreadLocal object dies in a thread, it destroys
// the managed object for that thread.
TEST(ThreadLocalTest, DestroysManagedObjectForOwnThreadWhenDying) {
  g_destroyed.clear();

  {
    // The next line default constructs a DestructorTracker object as
    // the default value of objects managed by thread_local.
    ThreadLocal<DestructorTracker> thread_local;
    ASSERT_EQ(1U, g_destroyed.size());
    ASSERT_FALSE(g_destroyed[0]);

    // This creates another DestructorTracker object for the main thread.
    thread_local.get();
    ASSERT_EQ(2U, g_destroyed.size());
    ASSERT_FALSE(g_destroyed[0]);
    ASSERT_FALSE(g_destroyed[1]);
  }

  // Now thread_local has died.  It should have destroyed both the
  // default value shared by all threads and the value for the main
  // thread.
  ASSERT_EQ(2U, g_destroyed.size());
  EXPECT_TRUE(g_destroyed[0]);
  EXPECT_TRUE(g_destroyed[1]);

  g_destroyed.clear();
}

// Tests that when a thread exits, the thread-local object for that
// thread is destroyed.
TEST(ThreadLocalTest, DestroysManagedObjectAtThreadExit) {
  g_destroyed.clear();

  {
    // The next line default constructs a DestructorTracker object as
    // the default value of objects managed by thread_local.
    ThreadLocal<DestructorTracker> thread_local;
    ASSERT_EQ(1U, g_destroyed.size());
    ASSERT_FALSE(g_destroyed[0]);

    // This creates another DestructorTracker object in the new thread.
    ThreadWithParam<ThreadParam> thread(
        &CallThreadLocalGet, &thread_local, NULL);
    thread.Join();

    // Now the new thread has exited.  The per-thread object for it
    // should have been destroyed.
    ASSERT_EQ(2U, g_destroyed.size());
    ASSERT_FALSE(g_destroyed[0]);
    ASSERT_TRUE(g_destroyed[1]);
  }

  // Now thread_local has died.  The default value should have been
  // destroyed too.
  ASSERT_EQ(2U, g_destroyed.size());
  EXPECT_TRUE(g_destroyed[0]);
  EXPECT_TRUE(g_destroyed[1]);

  g_destroyed.clear();
}

TEST(ThreadLocalTest, ThreadLocalMutationsAffectOnlyCurrentThread) {
  ThreadLocal<String> thread_local;
  thread_local.set("Foo");
  EXPECT_STREQ("Foo", thread_local.get().c_str());

  String result;
  RunFromThread(&RetrieveThreadLocalValue, make_pair(&thread_local, &result));
  EXPECT_TRUE(result.c_str() == NULL);
}

#endif  // GTEST_IS_THREADSAFE

}  // namespace internal
}  // namespace testing
