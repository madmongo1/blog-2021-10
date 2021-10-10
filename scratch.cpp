//
// Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/madmongo1/blog-2021-10
//

#include <asio.hpp>
#include <asio/experimental/deferred.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <iostream>

template <typename CompletionToken>
struct timed_token
{
    std::chrono::milliseconds timeout;
    CompletionToken& token;
};

template <typename... Signatures>
struct timed_initiation
{
    template <typename CompletionHandler, typename Initiation, typename... InitArgs>
    void operator()(CompletionHandler handler, std::chrono::milliseconds timeout, Initiation&& initiation, InitArgs&&... init_args)
    {
        auto ex = asio::get_associated_executor(handler, asio::get_associated_executor(initiation));
        auto timer = std::make_shared<asio::steady_timer>(ex, timeout);
        asio::experimental::make_parallel_group(
                asio::bind_executor(ex,
                                    [&](auto&& token)
                                    {
                                        return timer->async_wait(std::forward<decltype(token)>(token));
                                    }),
                asio::bind_executor(ex,
                                    [&](auto&& token)
                                    {
                                        return asio::async_initiate<decltype(token), Signatures...>(
                                                std::forward<Initiation>(initiation), token,
                                                std::forward<InitArgs>(init_args)...);
                                    })
        ).async_wait(
                asio::experimental::wait_for_one(),
                [handler = std::move(handler), timer](auto, auto, auto... results) mutable
                {
                    std::move(handler)(std::move(results)...);
                });
    }
};

template <typename CompletionToken, typename... Signatures>
struct asio::async_result<timed_token<CompletionToken>, Signatures...>
{
    template <typename Initiation, typename... InitArgs>
    static auto initiate(Initiation&& init, timed_token<CompletionToken> t, InitArgs&&... init_args)
    {
        return asio::async_initiate<CompletionToken, Signatures...>(
                timed_initiation<Signatures...>{}, t.token, t.timeout,
                std::forward<Initiation>(init), std::forward<InitArgs>(init_args)...);
    }
};

template <typename CompletionToken>
timed_token<CompletionToken> timed(std::chrono::milliseconds timeout, CompletionToken&& token)
{
    return timed_token<CompletionToken>{ timeout, token };
}

template <typename Op, typename CompletionToken>
auto timed(Op op, std::chrono::milliseconds timeout, CompletionToken&& token)
{
    return std::move(op)(timed(timeout, std::forward<CompletionToken>(token)));
}

template <typename Op>
asio::awaitable<void> test(Op op)
{
    co_await op(asio::use_awaitable);
}

asio::awaitable<void> run()
{
    using namespace asio;
    using namespace std::literals;
    using experimental::deferred;

    try
    {
        posix::stream_descriptor in(co_await this_coro::executor, ::dup(STDIN_FILENO));
        std::string line;
        std::size_t n = co_await async_read_until(in, dynamic_buffer(line), '\n',
                                                  timed(5s, use_awaitable));
        /*
        std::size_t n = co_await timed(
                async_read_until(in, asio::dynamic_buffer(line), '\n', deferred),
                5s,
                use_awaitable);
        */
        std::cout << line.substr(0, n);
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }
}

int main()
{
    asio::io_context ctx;
    //asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));

#if 0
    char data[1024];
    in.async_read_some(asio::buffer(data),
        timed(std::chrono::milliseconds(10000),
          [](std::error_code e, std::size_t n)
          {
            std::cout << "e: " << e << "\n";
            std::cout << "n: " << n << "\n";
          }));

    auto def = timed(
        in.async_read_some(asio::buffer(data), asio::experimental::deferred),
        std::chrono::milliseconds(10000),
        asio::experimental::deferred);
#endif

    /*def(
        [](std::error_code e, std::size_t n)
        {
          std::cout << "e: " << e << "\n";
          std::cout << "n: " << n << "\n";
        });*/

    //co_spawn(ctx, test(def), asio::detached);

    co_spawn(ctx, run(), asio::detached);
    ctx.run();
}