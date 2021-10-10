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
#include <asio/experimental/as_tuple.hpp>

#include <iostream>


auto test(asio::ip::tcp::socket& sock, asio::mutable_buffer buf)
{
  return sock.async_read_some(buf, asio::experimental::deferred);
}

int main()
{
  asio::io_context ioc;

  asio::ip::tcp::socket sock { ioc };
  char buf[1024];

  using asio::experimental::as_tuple;
  using asio::experimental::deferred;

  auto read_a_bit = [&sock, &buf]()
  {
    return sock.async_read_some(asio::buffer(buf), asio::experimental::deferred);
  }();

  read_a_bit(as_tuple([](std::tuple<std::error_code, std::size_t> args){
    std::cout << get<0>(args) << " : " << get<1>(args) << "\n";
  }));

  auto read_a_bit2 = read_a_bit(deferred);

  read_a_bit2(as_tuple([](std::tuple<std::error_code, std::size_t> args){
    std::cout << get<0>(args) << " : " << get<1>(args) << "\n";
  }));

  asio::co_spawn(ioc, read_a_bit(asio::use_awaitable),
                 [](std::exception_ptr ep, std::size_t n)
                 {
                    try {
                      if(ep) std::rethrow_exception()
                    }
                 });


  ioc.run();

}