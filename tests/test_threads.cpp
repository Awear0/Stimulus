#include "../stimulus.h"

#include <gtest/gtest.h>
#include <thread>

#include "utilities.h"

class test_threads: public ::testing::Test
{
protected:
    safe_generic_emitter<> empty_emitter;
};

template<std::size_t Count>
void create_connections(safe_generic_emitter<>& empty)
{
    for (std::size_t i = 0; i < Count; ++i)
    {
        empty.generic_signal.connect(slot_function<>);
    }
}

TEST_F(test_threads, many_connections)
{
    int& count = call_count<>;
    reset<>();

    std::thread t1 { create_connections<10000>, std::ref(empty_emitter) };
    std::thread t2 { create_connections<10000>, std::ref(empty_emitter) };
    std::thread t3 { create_connections<10000>, std::ref(empty_emitter) };
    std::thread t4 { create_connections<10000>, std::ref(empty_emitter) };
    std::thread t5 { create_connections<10000>, std::ref(empty_emitter) };

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    empty_emitter.generic_emit();

    EXPECT_EQ(count, 5 * 10000);
}

TEST_F(test_threads, many_connections_many_emits)
{
    std::thread t1 { create_connections<1000>, std::ref(empty_emitter) };
    std::thread t2 { create_connections<1000>, std::ref(empty_emitter) };
    std::thread t3 { create_connections<1000>, std::ref(empty_emitter) };
    std::thread t4 { create_connections<1000>, std::ref(empty_emitter) };
    std::thread t5 { create_connections<1000>, std::ref(empty_emitter) };

    for (int i { 0 }; i < 1000; ++i)
    {
        empty_emitter.generic_emit();
    }

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    // Ensuring there's no crash
}
