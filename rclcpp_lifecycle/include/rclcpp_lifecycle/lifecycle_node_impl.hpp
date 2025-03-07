// Copyright 2016 Open Source Robotics Foundation, Inc.
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

#ifndef RCLCPP_LIFECYCLE__LIFECYCLE_NODE_IMPL_HPP_
#define RCLCPP_LIFECYCLE__LIFECYCLE_NODE_IMPL_HPP_

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rcl_interfaces/msg/parameter_descriptor.hpp"

#include "rclcpp/callback_group.hpp"
#include "rclcpp/create_client.hpp"
#include "rclcpp/create_generic_publisher.hpp"
#include "rclcpp/create_generic_subscription.hpp"
#include "rclcpp/create_publisher.hpp"
#include "rclcpp/create_service.hpp"
#include "rclcpp/create_subscription.hpp"
#include "rclcpp/parameter.hpp"
#include "rclcpp/publisher_options.hpp"
#include "rclcpp/qos.hpp"
#include "rclcpp/subscription_options.hpp"
#include "rclcpp/type_support_decl.hpp"

#include "rmw/types.h"

#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "rclcpp_lifecycle/lifecycle_subscriber.hpp"
#include "rclcpp_lifecycle/visibility_control.h"

namespace rclcpp_lifecycle
{

template<typename MessageT, typename AllocatorT>
std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<MessageT, AllocatorT>>
LifecycleNode::create_publisher(
  const std::string & topic_name,
  const rclcpp::QoS & qos,
  const rclcpp::PublisherOptionsWithAllocator<AllocatorT> & options)
{
  using PublisherT = rclcpp_lifecycle::LifecyclePublisher<MessageT, AllocatorT>;
  auto pub = rclcpp::create_publisher<MessageT, AllocatorT, PublisherT>(
    *this,
    topic_name,
    qos,
    options);
  this->add_managed_entity(pub);
  return pub;
}

template<
  typename MessageT,
  typename CallbackT,
  typename AllocatorT,
  typename SubscriptionT = rclcpp_lifecycle::LifecycleSubscription<MessageT>,
  typename MessageMemoryStrategyT>
std::shared_ptr<SubscriptionT>
LifecycleNode::create_subscription(
  const std::string & topic_name,
  const rclcpp::QoS & qos,
  CallbackT && callback,
  const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options,
  typename MessageMemoryStrategyT::SharedPtr msg_mem_strat)
{
  auto sub = rclcpp::create_subscription<MessageT, CallbackT, AllocatorT, SubscriptionT>(
    *this,
    topic_name,
    qos,
    std::forward<CallbackT>(callback),
    options,
    msg_mem_strat);
  this->add_managed_entity(sub);
  return sub;
}

template<typename DurationRepT, typename DurationT, typename CallbackT>
typename rclcpp::WallTimer<CallbackT>::SharedPtr
LifecycleNode::create_wall_timer(
  std::chrono::duration<DurationRepT, DurationT> period,
  CallbackT callback,
  rclcpp::CallbackGroup::SharedPtr group)
{
  return rclcpp::create_wall_timer(
    period,
    std::move(callback),
    group,
    this->node_base_.get(),
    this->node_timers_.get());
}

template<typename DurationRepT, typename DurationT, typename CallbackT>
typename rclcpp::GenericTimer<CallbackT>::SharedPtr
LifecycleNode::create_timer(
  std::chrono::duration<DurationRepT, DurationT> period,
  CallbackT callback,
  rclcpp::CallbackGroup::SharedPtr group)
{
  return rclcpp::create_timer(
    this->get_clock(),
    period,
    std::move(callback),
    group,
    this->node_base_.get(),
    this->node_timers_.get());
}

template<typename ServiceT>
typename rclcpp::Client<ServiceT>::SharedPtr
LifecycleNode::create_client(
  const std::string & service_name,
  const rmw_qos_profile_t & qos_profile,
  rclcpp::CallbackGroup::SharedPtr group)
{
  return rclcpp::create_client<ServiceT>(
    node_base_, node_graph_, node_services_,
    service_name, qos_profile, group);
}

template<typename ServiceT>
typename rclcpp::Client<ServiceT>::SharedPtr
LifecycleNode::create_client(
  const std::string & service_name,
  const rclcpp::QoS & qos,
  rclcpp::CallbackGroup::SharedPtr group)
{
  return rclcpp::create_client<ServiceT>(
    node_base_, node_graph_, node_services_,
    service_name, qos, group);
}

template<typename ServiceT, typename CallbackT>
typename rclcpp::Service<ServiceT>::SharedPtr
LifecycleNode::create_service(
  const std::string & service_name,
  CallbackT && callback,
  const rmw_qos_profile_t & qos_profile,
  rclcpp::CallbackGroup::SharedPtr group)
{
  return rclcpp::create_service<ServiceT, CallbackT>(
    node_base_, node_services_,
    service_name, std::forward<CallbackT>(callback), qos_profile, group);
}

template<typename ServiceT, typename CallbackT>
typename rclcpp::Service<ServiceT>::SharedPtr
LifecycleNode::create_service(
  const std::string & service_name,
  CallbackT && callback,
  const rclcpp::QoS & qos,
  rclcpp::CallbackGroup::SharedPtr group)
{
  return rclcpp::create_service<ServiceT, CallbackT>(
    node_base_, node_services_,
    service_name, std::forward<CallbackT>(callback), qos, group);
}

template<typename AllocatorT>
std::shared_ptr<rclcpp::GenericPublisher>
LifecycleNode::create_generic_publisher(
  const std::string & topic_name,
  const std::string & topic_type,
  const rclcpp::QoS & qos,
  const rclcpp::PublisherOptionsWithAllocator<AllocatorT> & options)
{
  return rclcpp::create_generic_publisher(
    node_topics_,
    // TODO(karsten1987): LifecycleNode is currently not supporting subnamespaces
    // see https://github.com/ros2/rclcpp/issues/1614
    topic_name,
    topic_type,
    qos,
    options
  );
}

template<typename AllocatorT>
std::shared_ptr<rclcpp::GenericSubscription>
LifecycleNode::create_generic_subscription(
  const std::string & topic_name,
  const std::string & topic_type,
  const rclcpp::QoS & qos,
  std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback,
  const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options)
{
  return rclcpp::create_generic_subscription(
    node_topics_,
    // TODO(karsten1987): LifecycleNode is currently not supporting subnamespaces
    // see https://github.com/ros2/rclcpp/issues/1614
    topic_name,
    topic_type,
    qos,
    std::move(callback),
    options
  );
}

template<typename ParameterT>
auto
LifecycleNode::declare_parameter(
  const std::string & name,
  const ParameterT & default_value,
  const rcl_interfaces::msg::ParameterDescriptor & parameter_descriptor,
  bool ignore_override)
{
  return this->declare_parameter(
    name,
    rclcpp::ParameterValue(default_value),
    parameter_descriptor,
    ignore_override
  ).get<ParameterT>();
}

template<typename ParameterT>
auto
LifecycleNode::declare_parameter(
  const std::string & name,
  const rcl_interfaces::msg::ParameterDescriptor & parameter_descriptor,
  bool ignore_override)
{
  // get advantage of parameter value template magic to get
  // the correct rclcpp::ParameterType from ParameterT
  rclcpp::ParameterValue value{ParameterT{}};
  return this->declare_parameter(
    name,
    value.get_type(),
    parameter_descriptor,
    ignore_override
  ).get<ParameterT>();
}

template<typename ParameterT>
std::vector<ParameterT>
LifecycleNode::declare_parameters(
  const std::string & namespace_,
  const std::map<std::string, ParameterT> & parameters)
{
  std::vector<ParameterT> result;
  std::string normalized_namespace = namespace_.empty() ? "" : (namespace_ + ".");
  std::transform(
    parameters.begin(), parameters.end(), std::back_inserter(result),
    [this, &normalized_namespace](auto element) {
      return this->declare_parameter(normalized_namespace + element.first, element.second);
    }
  );
  return result;
}

template<typename ParameterT>
std::vector<ParameterT>
LifecycleNode::declare_parameters(
  const std::string & namespace_,
  const std::map<
    std::string,
    std::pair<ParameterT, rcl_interfaces::msg::ParameterDescriptor>
  > & parameters)
{
  std::vector<ParameterT> result;
  std::string normalized_namespace = namespace_.empty() ? "" : (namespace_ + ".");
  std::transform(
    parameters.begin(), parameters.end(), std::back_inserter(result),
    [this, &normalized_namespace](auto element) {
      return static_cast<ParameterT>(
        this->declare_parameter(
          normalized_namespace + element.first,
          element.second.first,
          element.second.second)
      );
    }
  );
  return result;
}

template<typename ParameterT>
bool
LifecycleNode::get_parameter(const std::string & name, ParameterT & parameter) const
{
  rclcpp::Parameter param(name, parameter);
  bool result = get_parameter(name, param);
  parameter = param.get_value<ParameterT>();

  return result;
}

// this is a partially-specialized version of get_parameter above,
// where our concrete type for ParameterT is std::map, but the to-be-determined
// type is the value in the map.
template<typename MapValueT>
bool
LifecycleNode::get_parameters(
  const std::string & prefix,
  std::map<std::string, MapValueT> & values) const
{
  std::map<std::string, rclcpp::Parameter> params;
  bool result = node_parameters_->get_parameters_by_prefix(prefix, params);
  if (result) {
    for (const auto & param : params) {
      values[param.first] = param.second.get_value<MapValueT>();
    }
  }

  return result;
}

template<typename ParameterT>
bool
LifecycleNode::get_parameter_or(
  const std::string & name,
  ParameterT & value,
  const ParameterT & alternative_value) const
{
  bool got_parameter = get_parameter(name, value);
  if (!got_parameter) {
    value = alternative_value;
  }
  return got_parameter;
}

}  // namespace rclcpp_lifecycle
#endif  // RCLCPP_LIFECYCLE__LIFECYCLE_NODE_IMPL_HPP_
