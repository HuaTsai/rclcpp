// Copyright 2017 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>

#include "rcl/error_handling.h"
#include "rcl/time.h"
#include "rclcpp/clock.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/duration.hpp"

#include "../utils/rclcpp_gtest_macros.hpp"

using namespace std::chrono_literals;

class TestDuration : public ::testing::Test
{
};

TEST_F(TestDuration, operators) {
  rclcpp::Duration old(1, 0);
  rclcpp::Duration young(2, 0);

  EXPECT_TRUE(old < young);
  EXPECT_TRUE(young > old);
  EXPECT_TRUE(old <= young);
  EXPECT_TRUE(young >= old);
  EXPECT_FALSE(young == old);
  EXPECT_TRUE(young != old);

  rclcpp::Duration add = old + young;
  EXPECT_EQ(add.nanoseconds(), old.nanoseconds() + young.nanoseconds());
  EXPECT_EQ(add, old + young);

  rclcpp::Duration sub = young - old;
  EXPECT_EQ(sub.nanoseconds(), young.nanoseconds() - old.nanoseconds());
  EXPECT_EQ(sub, young - old);

  rclcpp::Duration addequal = old;
  addequal += young;
  EXPECT_EQ(addequal.nanoseconds(), old.nanoseconds() + young.nanoseconds());
  EXPECT_EQ(addequal, old + young);

  rclcpp::Duration subequal = young;
  subequal -= old;
  EXPECT_EQ(subequal.nanoseconds(), young.nanoseconds() - old.nanoseconds());
  EXPECT_EQ(subequal, young - old);

  rclcpp::Duration scale = old * 3;
  EXPECT_EQ(scale.nanoseconds(), old.nanoseconds() * 3);

  rclcpp::Duration scaleequal = old;
  scaleequal *= 3;
  EXPECT_EQ(scaleequal.nanoseconds(), old.nanoseconds() * 3);

  rclcpp::Duration time = rclcpp::Duration(0, 0);
  rclcpp::Duration copy_constructor_duration(time);
  rclcpp::Duration assignment_op_duration = rclcpp::Duration(1, 0);
  (void)assignment_op_duration;
  assignment_op_duration = time;

  EXPECT_TRUE(time == copy_constructor_duration);
  EXPECT_TRUE(time == assignment_op_duration);
}

TEST_F(TestDuration, operators_with_message_stamp) {
  rclcpp::Duration pos_duration(1, 100000000u);  // 1.1s
  rclcpp::Duration neg_duration(-2, 900000000u);  // -1.1s

  // Addition and subtraction operators
  builtin_interfaces::msg::Time time_msg = rclcpp::Time(0, 100000000u);  // 0.1s
  builtin_interfaces::msg::Time res_addpos = time_msg + pos_duration;
  EXPECT_EQ(res_addpos.sec, 1);
  EXPECT_EQ(res_addpos.nanosec, 200000000u);

  builtin_interfaces::msg::Time res_addneg = time_msg + neg_duration;
  EXPECT_EQ(res_addneg.sec, -1);
  EXPECT_EQ(res_addneg.nanosec, 0);

  builtin_interfaces::msg::Time res_subpos = time_msg - pos_duration;
  EXPECT_EQ(res_subpos.sec, -1);
  EXPECT_EQ(res_subpos.nanosec, 0);

  builtin_interfaces::msg::Time res_subneg = time_msg - neg_duration;
  EXPECT_EQ(res_subneg.sec, 1);
  EXPECT_EQ(res_subneg.nanosec, 200000000u);

  builtin_interfaces::msg::Time neg_time_msg;
  neg_time_msg.sec = -1;
  auto max = rclcpp::Duration::from_nanoseconds(std::numeric_limits<rcl_duration_value_t>::max());

  EXPECT_THROW(neg_time_msg + max, std::runtime_error);
  EXPECT_THROW(time_msg + max, std::overflow_error);

  // Addition and subtraction assignment operators
  time_msg = rclcpp::Time(0, 100000000u);
  time_msg += pos_duration;
  EXPECT_EQ(time_msg.sec, 1);
  EXPECT_EQ(time_msg.nanosec, 200000000u);

  time_msg -= pos_duration;
  EXPECT_EQ(time_msg.sec, 0);
  EXPECT_EQ(time_msg.nanosec, 100000000u);

  time_msg += neg_duration;
  EXPECT_EQ(time_msg.sec, -1);
  EXPECT_EQ(time_msg.nanosec, 0u);

  EXPECT_THROW(time_msg -= neg_duration, std::runtime_error);  // not allow negative left operand

  time_msg = rclcpp::Time(0, 100000000u);
  time_msg -= neg_duration;
  EXPECT_EQ(time_msg.sec, 1);
  EXPECT_EQ(time_msg.nanosec, 200000000u);

  EXPECT_THROW(neg_time_msg += max, std::runtime_error);
  EXPECT_THROW(time_msg += max, std::overflow_error);
}

TEST_F(TestDuration, chrono_overloads) {
  int64_t ns = 123456789l;
  auto chrono_ns = std::chrono::nanoseconds(ns);
  auto d1 = rclcpp::Duration::from_nanoseconds(ns);
  auto d2 = rclcpp::Duration(chrono_ns);
  auto d3 = rclcpp::Duration(123456789ns);
  EXPECT_EQ(d1, d2);
  EXPECT_EQ(d1, d3);
  EXPECT_EQ(d2, d3);

  // check non-nanosecond durations
  std::chrono::milliseconds chrono_ms(100);
  auto d4 = rclcpp::Duration(chrono_ms);
  EXPECT_EQ(chrono_ms, d4.to_chrono<std::chrono::nanoseconds>());
  std::chrono::duration<double, std::chrono::seconds::period> chrono_float_seconds(3.14);
  auto d5 = rclcpp::Duration(chrono_float_seconds);
  EXPECT_EQ(chrono_float_seconds, d5.to_chrono<decltype(chrono_float_seconds)>());
}

TEST_F(TestDuration, overflows) {
  auto max = rclcpp::Duration::from_nanoseconds(std::numeric_limits<rcl_duration_value_t>::max());
  auto min = rclcpp::Duration::from_nanoseconds(std::numeric_limits<rcl_duration_value_t>::min());

  rclcpp::Duration one(1ns);
  rclcpp::Duration negative_one(-1ns);

  EXPECT_THROW(max + one, std::overflow_error);
  EXPECT_THROW(min - one, std::underflow_error);
  EXPECT_THROW(negative_one + min, std::underflow_error);
  EXPECT_THROW(negative_one - max, std::underflow_error);

  rclcpp::Duration base_d = max * 0.3;
  EXPECT_THROW(base_d * 4, std::overflow_error);
  EXPECT_THROW(base_d * (-4), std::underflow_error);

  rclcpp::Duration base_d_neg = max * (-0.3);
  EXPECT_THROW(base_d_neg * (-4), std::overflow_error);
  EXPECT_THROW(base_d_neg * 4, std::underflow_error);
}

TEST_F(TestDuration, negative_duration) {
  rclcpp::Duration assignable_duration = rclcpp::Duration(0ns) - rclcpp::Duration(5, 0);

  {
    // avoid windows converting a literal number less than -INT_MAX to unsigned int C4146
    int64_t expected_value = -5000;
    expected_value *= 1000 * 1000;
    EXPECT_EQ(expected_value, assignable_duration.nanoseconds());
  }

  {
    builtin_interfaces::msg::Duration duration_msg;
    duration_msg.sec = -4;
    duration_msg.nanosec = 250000000;

    assignable_duration = duration_msg;
    // avoid windows converting a literal number less than -INT_MAX to unsigned int C4146
    int64_t expected_value = -3750;
    expected_value *= 1000 * 1000;
    EXPECT_EQ(expected_value, assignable_duration.nanoseconds());
  }
}

TEST_F(TestDuration, maximum_duration) {
  rclcpp::Duration max_duration = rclcpp::Duration::max();
  rclcpp::Duration max(std::numeric_limits<int32_t>::max(), 999999999);

  EXPECT_EQ(max_duration, max);
}

static const int64_t HALF_SEC_IN_NS = 500 * 1000 * 1000;
static const int64_t ONE_SEC_IN_NS = 1000 * 1000 * 1000;
static const int64_t ONE_AND_HALF_SEC_IN_NS = 3 * HALF_SEC_IN_NS;
static const int64_t MAX_NANOSECONDS = std::numeric_limits<int64_t>::max();

TEST_F(TestDuration, from_seconds) {
  EXPECT_EQ(rclcpp::Duration(0ns), rclcpp::Duration::from_seconds(0.0));
  EXPECT_EQ(rclcpp::Duration(0ns), rclcpp::Duration::from_seconds(0));
  EXPECT_EQ(rclcpp::Duration(1, HALF_SEC_IN_NS), rclcpp::Duration::from_seconds(1.5));
  EXPECT_EQ(
    rclcpp::Duration::from_nanoseconds(-ONE_AND_HALF_SEC_IN_NS),
    rclcpp::Duration::from_seconds(-1.5));
}

TEST_F(TestDuration, from_rmw_time) {
  constexpr auto max_rcl_duration = std::numeric_limits<rcl_duration_value_t>::max();
  {
    rmw_time_t rmw_duration{};
    rmw_duration.sec = RCL_NS_TO_S(max_rcl_duration) + 1uLL;
    EXPECT_EQ(rclcpp::Duration::from_rmw_time(rmw_duration).nanoseconds(), max_rcl_duration);
  }
  {
    rmw_time_t rmw_duration{};
    rmw_duration.nsec = max_rcl_duration + 1uLL;
    EXPECT_EQ(rclcpp::Duration::from_rmw_time(rmw_duration).nanoseconds(), max_rcl_duration);
  }
  {
    rmw_time_t rmw_duration{};
    rmw_duration.nsec = max_rcl_duration;
    rmw_duration.sec = RCL_NS_TO_S(max_rcl_duration);
    EXPECT_EQ(rclcpp::Duration::from_rmw_time(rmw_duration).nanoseconds(), max_rcl_duration);
  }
  {
    rmw_time_t rmw_duration{};
    rmw_duration.sec = 1u;
    rmw_duration.nsec = 1000u;
    EXPECT_EQ(
      rclcpp::Duration::from_rmw_time(rmw_duration).nanoseconds(),
      static_cast<rcl_duration_value_t>(RCL_S_TO_NS(rmw_duration.sec) + rmw_duration.nsec));
  }
}

TEST_F(TestDuration, std_chrono_constructors) {
  EXPECT_EQ(rclcpp::Duration(0ns), rclcpp::Duration(0.0s));
  EXPECT_EQ(rclcpp::Duration(0ns), rclcpp::Duration(0s));
  EXPECT_EQ(rclcpp::Duration(1, HALF_SEC_IN_NS), rclcpp::Duration(1.5s));
  EXPECT_EQ(rclcpp::Duration(-1, 0), rclcpp::Duration(-1s));
}

TEST_F(TestDuration, conversions) {
  {
    auto duration = rclcpp::Duration::from_nanoseconds(HALF_SEC_IN_NS);
    const auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, 0);
    EXPECT_EQ(duration_msg.nanosec, HALF_SEC_IN_NS);
    EXPECT_EQ(rclcpp::Duration(duration_msg).nanoseconds(), HALF_SEC_IN_NS);

    const auto rmw_time = duration.to_rmw_time();
    EXPECT_EQ(rmw_time.sec, 0u);
    EXPECT_EQ(rmw_time.nsec, static_cast<uint64_t>(HALF_SEC_IN_NS));

    const auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), HALF_SEC_IN_NS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(ONE_SEC_IN_NS);
    const auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, 1);
    EXPECT_EQ(duration_msg.nanosec, 0u);
    EXPECT_EQ(rclcpp::Duration(duration_msg).nanoseconds(), ONE_SEC_IN_NS);

    const auto rmw_time = duration.to_rmw_time();
    EXPECT_EQ(rmw_time.sec, 1u);
    EXPECT_EQ(rmw_time.nsec, 0u);

    const auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), ONE_SEC_IN_NS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(ONE_AND_HALF_SEC_IN_NS);
    auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, 1);
    EXPECT_EQ(duration_msg.nanosec, HALF_SEC_IN_NS);
    EXPECT_EQ(rclcpp::Duration(duration_msg).nanoseconds(), ONE_AND_HALF_SEC_IN_NS);

    auto rmw_time = duration.to_rmw_time();
    EXPECT_EQ(rmw_time.sec, 1u);
    EXPECT_EQ(rmw_time.nsec, static_cast<uint64_t>(HALF_SEC_IN_NS));

    auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), ONE_AND_HALF_SEC_IN_NS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(-HALF_SEC_IN_NS);
    auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, -1);
    EXPECT_EQ(duration_msg.nanosec, HALF_SEC_IN_NS);
    EXPECT_EQ(rclcpp::Duration(duration_msg).nanoseconds(), -HALF_SEC_IN_NS);

    EXPECT_THROW(duration.to_rmw_time(), std::runtime_error);

    auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), -HALF_SEC_IN_NS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(-ONE_SEC_IN_NS);
    auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, -1);
    EXPECT_EQ(duration_msg.nanosec, 0u);
    EXPECT_EQ(rclcpp::Duration(duration_msg).nanoseconds(), -ONE_SEC_IN_NS);

    EXPECT_THROW(duration.to_rmw_time(), std::runtime_error);

    auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), -ONE_SEC_IN_NS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(-ONE_AND_HALF_SEC_IN_NS);
    auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, -2);
    EXPECT_EQ(duration_msg.nanosec, HALF_SEC_IN_NS);
    EXPECT_EQ(rclcpp::Duration(duration_msg).nanoseconds(), -ONE_AND_HALF_SEC_IN_NS);

    EXPECT_THROW(duration.to_rmw_time(), std::runtime_error);

    auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), -ONE_AND_HALF_SEC_IN_NS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(MAX_NANOSECONDS);

    const auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, std::numeric_limits<int32_t>::max());
    EXPECT_EQ(duration_msg.nanosec, std::numeric_limits<uint32_t>::max());

    auto rmw_time = duration.to_rmw_time();
    EXPECT_EQ(rmw_time.sec, 9223372036u);
    EXPECT_EQ(rmw_time.nsec, 854775807u);

    auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), MAX_NANOSECONDS);
  }

  {
    auto duration = rclcpp::Duration::from_nanoseconds(-MAX_NANOSECONDS);

    const auto duration_msg = static_cast<builtin_interfaces::msg::Duration>(duration);
    EXPECT_EQ(duration_msg.sec, std::numeric_limits<int32_t>::min());
    EXPECT_EQ(duration_msg.nanosec, 0u);

    EXPECT_THROW(duration.to_rmw_time(), std::runtime_error);

    auto chrono_duration = duration.to_chrono<std::chrono::nanoseconds>();
    EXPECT_EQ(chrono_duration.count(), -MAX_NANOSECONDS);
  }
}

TEST_F(TestDuration, test_some_constructors) {
  builtin_interfaces::msg::Duration duration_msg;
  duration_msg.sec = 1;
  duration_msg.nanosec = 1000;
  rclcpp::Duration duration_from_msg(duration_msg);
  EXPECT_EQ(RCL_S_TO_NS(1) + 1000, duration_from_msg.nanoseconds());

  rcl_duration_t duration_struct;
  duration_struct.nanoseconds = 4000;
  rclcpp::Duration duration_from_struct(duration_struct);
  EXPECT_EQ(4000, duration_from_struct.nanoseconds());
}

TEST_F(TestDuration, test_some_exceptions) {
  rclcpp::Duration test_duration(0ns);
  RCLCPP_EXPECT_THROW_EQ(
    test_duration =
    rclcpp::Duration::from_nanoseconds(INT64_MAX) - rclcpp::Duration(-1ns),
    std::overflow_error("duration subtraction leads to int64_t overflow"));
  RCLCPP_EXPECT_THROW_EQ(
    test_duration = test_duration * (std::numeric_limits<double>::infinity()),
    std::runtime_error("abnormal scale in rclcpp::Duration"));
}
