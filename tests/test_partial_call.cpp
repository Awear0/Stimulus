#include "../stimulus.h"

#include <string>

#include <gtest/gtest.h>

using ::testing::Test;

template<class... Args>
void function(Args...)
{
}

template<class... Args>
auto lambda { [](Args...) {} };

template<class... Args>
struct functor
{
    void operator()(Args...)
    {
    }
};

TEST(partial_call, basic_test)
{
    EXPECT_EQ(details::partially_callable<decltype(function<>)>, true);
    EXPECT_EQ((details::partially_callable<decltype(function<>), int>), true);
    EXPECT_EQ((details::partially_callable<decltype(function<>), int, std::string>), true);
    EXPECT_EQ((details::partially_callable<decltype(function<>), int, std::string, double>), true);
    EXPECT_EQ(details::partially_callable<decltype(function<int>)>, false);
    EXPECT_EQ((details::partially_callable<decltype(function<int>), int>), true);
    EXPECT_EQ((details::partially_callable<decltype(function<int>), int, std::string>), true);
    EXPECT_EQ((details::partially_callable<decltype(function<int>), int, std::string, double>),
              true);
    EXPECT_EQ(details::partially_callable<decltype(function<int, std::string>)>, false);
    EXPECT_EQ((details::partially_callable<decltype(function<int, std::string>), int>), false);
    EXPECT_EQ((details::partially_callable<decltype(function<int, std::string>), int, std::string>),
              true);
    EXPECT_EQ(
        (details::
             partially_callable<decltype(function<int, std::string>), int, std::string, double>),
        true);
    EXPECT_EQ(details::partially_callable<decltype(function<int, std::string, double>)>, false);
    EXPECT_EQ((details::partially_callable<decltype(function<int, std::string, double>), int>),
              false);
    EXPECT_EQ((details::partially_callable<decltype(function<int, std::string, double>),
                                           int,
                                           std::string>),
              false);
    EXPECT_EQ((details::partially_callable<decltype(function<int, std::string, double>),
                                           int,
                                           std::string,
                                           double>),
              true);

    EXPECT_EQ((details::partially_callable<decltype(function<int, std::string, double>),
                                           int,
                                           std::string,
                                           int>),
              true);

    EXPECT_EQ(
        (details::
             partially_callable<decltype(function<int, std::string, double>), int, void*, double>),
        false);

    EXPECT_EQ(details::partially_callable<decltype(lambda<>)>, true);
    EXPECT_EQ((details::partially_callable<decltype(lambda<>), int>), true);
    EXPECT_EQ((details::partially_callable<decltype(lambda<>), int, std::string>), true);
    EXPECT_EQ((details::partially_callable<decltype(lambda<>), int, std::string, double>), true);
    EXPECT_EQ(details::partially_callable<decltype(lambda<int>)>, false);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int>), int>), true);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int>), int, std::string>), true);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int>), int, std::string, double>), true);
    EXPECT_EQ(details::partially_callable<decltype(lambda<int, std::string>)>, false);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int, std::string>), int>), false);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int, std::string>), int, std::string>),
              true);
    EXPECT_EQ(
        (details::partially_callable<decltype(lambda<int, std::string>), int, std::string, double>),
        true);
    EXPECT_EQ(details::partially_callable<decltype(lambda<int, std::string, double>)>, false);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int, std::string, double>), int>),
              false);
    EXPECT_EQ(
        (details::partially_callable<decltype(lambda<int, std::string, double>), int, std::string>),
        false);
    EXPECT_EQ((details::partially_callable<decltype(lambda<int, std::string, double>),
                                           int,
                                           std::string,
                                           double>),
              true);

    EXPECT_EQ(
        (details::
             partially_callable<decltype(lambda<int, std::string, double>), int, std::string, int>),
        true);

    EXPECT_EQ(
        (details::
             partially_callable<decltype(lambda<int, std::string, double>), int, void*, double>),
        false);

    EXPECT_EQ(details::partially_callable<functor<>>, true);
    EXPECT_EQ((details::partially_callable<functor<>, int>), true);
    EXPECT_EQ((details::partially_callable<functor<>, int, std::string>), true);
    EXPECT_EQ((details::partially_callable<functor<>, int, std::string, double>), true);
    EXPECT_EQ(details::partially_callable<functor<int>>, false);
    EXPECT_EQ((details::partially_callable<functor<int>, int>), true);
    EXPECT_EQ((details::partially_callable<functor<int>, int, std::string>), true);
    EXPECT_EQ((details::partially_callable<functor<int>, int, std::string, double>), true);
    EXPECT_EQ((details::partially_callable<functor<int, std::string>>), false);
    EXPECT_EQ((details::partially_callable<functor<int, std::string>, int>), false);
    EXPECT_EQ((details::partially_callable<functor<int, std::string>, int, std::string>), true);
    EXPECT_EQ((details::partially_callable<functor<int, std::string>, int, std::string, double>),
              true);
    EXPECT_EQ((details::partially_callable<functor<int, std::string, double>>), false);
    EXPECT_EQ((details::partially_callable<functor<int, std::string, double>, int>), false);
    EXPECT_EQ((details::partially_callable<functor<int, std::string, double>, int, std::string>),
              false);
    EXPECT_EQ(
        (details::partially_callable<functor<int, std::string, double>, int, std::string, double>),
        true);

    EXPECT_EQ(
        (details::partially_callable<functor<int, std::string, double>, int, std::string, int>),
        true);

    EXPECT_EQ((details::partially_callable<functor<int, std::string, double>, int, void*, double>),
              false);
}