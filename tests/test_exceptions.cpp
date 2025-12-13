#include "../sigslot.h"

#include <exception>
#include <functional>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>
#include <vector>

#include "utilities.h"

class exceptions_test: public ::testing::Test
{
protected:
    generic_emitter<> empty_emitter;
    generic_emitter<int> int_emitter;
    generic_emitter<std::string> string_emitter;
    generic_emitter<int, std::string> int_string_emitter;
    generic_emitter<copy_move_counter> copy_move_emitter;
    generic_emitter<int&> int_ref_emitter;

    struct storing_policy
    {
        void execute(std::function<void()> callable)
        {
            m_functions.emplace_back(std::move(callable));
        }

        static constexpr bool is_synchronous { false };

        std::vector<std::function<void()>> m_functions;
    };

    storing_policy policy;
};

namespace
{

    void int_throwing_function()
    {
        throw 5;
    }

    void runtime_error_throwing_function()
    {
        throw std::runtime_error { "Test" };
    }

    int int_caught { 0 };
    int runtime_error_caught { 0 };
    int something_else_caught { 0 };

    void reset_counters()
    {
        int_caught = 0;
        runtime_error_caught = 0;
        something_else_caught = 0;
    }

    void catching_handler(std::exception_ptr exception)
    {
        try
        {
            std::rethrow_exception(std::move(exception));
        }
        catch (int& i)
        {
            if (i == 5)
            {
                ++int_caught;
            }
        }
        catch (std::runtime_error& e)
        {
            if (e.what() == std::string("Test"))
            {
                ++runtime_error_caught;
            }
        }
        catch (...)
        {
            ++something_else_caught;
        }
    }

} // namespace

TEST_F(exceptions_test, int_thrower)
{
    reset_counters();

    auto connection { empty_emitter.generic_signal.connect(int_throwing_function) };
    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    EXPECT_THROW(empty_emitter.generic_emit(), int);

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    connection.add_exception_handler(catching_handler);

    empty_emitter.generic_emit();

    EXPECT_EQ(int_caught, 1);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);
}

TEST_F(exceptions_test, runtime_error_thrower)
{
    reset_counters();

    auto connection { empty_emitter.generic_signal.connect(runtime_error_throwing_function) };
    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    EXPECT_THROW(empty_emitter.generic_emit(), std::runtime_error);
    ;

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    connection.add_exception_handler(catching_handler);

    empty_emitter.generic_emit();

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 1);
    EXPECT_EQ(something_else_caught, 0);
}

TEST_F(exceptions_test, int_thrower_with_policy)
{
    reset_counters();

    auto connection { empty_emitter.generic_signal.connect(int_throwing_function, policy) };
    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    empty_emitter.generic_emit();

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    connection.add_exception_handler(catching_handler);

    empty_emitter.generic_emit();

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);
    EXPECT_EQ(policy.m_functions.size(), 2);

    EXPECT_THROW(policy.m_functions.front()(), int);

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    policy.m_functions.back()();

    EXPECT_EQ(int_caught, 1);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);
}

TEST_F(exceptions_test, runtime_error_thrower_with_policy)
{
    reset_counters();

    auto connection { empty_emitter.generic_signal.connect(runtime_error_throwing_function,
                                                           policy) };
    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    empty_emitter.generic_emit();

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    connection.add_exception_handler(catching_handler);

    empty_emitter.generic_emit();

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);
    EXPECT_EQ(policy.m_functions.size(), 2);

    EXPECT_THROW(policy.m_functions.front()(), std::runtime_error);
    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 0);
    EXPECT_EQ(something_else_caught, 0);

    policy.m_functions.back()();

    EXPECT_EQ(int_caught, 0);
    EXPECT_EQ(runtime_error_caught, 1);
    EXPECT_EQ(something_else_caught, 0);
}