// Copyright 2013 Google Inc. All Rights Reserved.
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

#ifndef SRC_RESOURCE_HANDLER_H_
#define SRC_RESOURCE_HANDLER_H_

#include <time.h>
#include <string>
using ::std::string;
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "include/lmctfy.h"
#include "include/lmctfy.pb.h"
#include "util/task/statusor.h"
#include "util/task/status.h"

namespace containers {
namespace lmctfy {

class ResourceHandler;

// TODO(vmarmol): Change to enum class when that is supported.
// Resource types supported by the lmctfy implementation.
enum ResourceType {
  RESOURCE_CPU,
  RESOURCE_MEMORY,
  RESOURCE_DISKIO,
  RESOURCE_NETWORK,
  RESOURCE_MONITORING,
  RESOURCE_GLOBAL
};

// Factory for ResourceHandlers. For each ContainerApi instance there should only
// ever be one ResourceHandlerFactory per resource. Each container will get its
// own ResourceHandler for each resource. Thus the ResourceHandlerFactories
// implement any resource-specific global logic and the creation and
// initialization of the resource.
class ResourceHandlerFactory {
 public:
  virtual ~ResourceHandlerFactory() {}

  // Creates a Resource Handler for an existing container.
  //
  // Arguments:
  //   container_name: Absolute name of the container.
  // Return:
  //   StatusOr<ResourceHandler *>: Status of the operation. Iff OK, returns an
  //       instance of a handler for this factory's resource. Pointer is owned
  //       by the caller.
  virtual ::util::StatusOr<ResourceHandler *> Get(
      const string &container_name) = 0;

  // Creates a new Resource Handler with the specified spec. Only uses parts of
  // the spec that match the resource implemented.
  //
  // Arguments:
  //   container_name: Absolute name of the container.
  //   spec: Specification for the new ResourceHandler.
  // Return:
  //   StatusOr<ResourceHandler *>: Status of the operation. Iff OK, returns an
  //       instance of a handler for this factory's resource. Pointer is owned
  //       by the caller.
  virtual ::util::StatusOr<ResourceHandler *> Create(
      const string &container_name,
      const ContainerSpec &spec) = 0;

  // Initialize this resource handler on this machine. This setup is idempotent
  // and only needs to be done once at machine bootup.
  virtual ::util::Status InitMachine(const InitSpec &spec) = 0;

  // Returns the type or resource implemented by this ResourceHandlerFactory.
  ResourceType type() const { return type_; }

 protected:
  explicit ResourceHandlerFactory(ResourceType type) : type_(type) {}

 private:
  ResourceType type_;

  DISALLOW_COPY_AND_ASSIGN(ResourceHandlerFactory);
};

// A ResourceHandler is the resource-specific logic that exists with each
// container. Resources are things like CPU, memory, and network. Each resource
// implements its own Resource Handler and each Container that uses a resource
// will receive its own copy of the Resource Handler.
class ResourceHandler {
 public:
  virtual ~ResourceHandler() {}

  // Applies the specified updates to this resource.
  //
  // Arguments:
  //   spec: The specification of the changes to make to the container.
  //   type: How to apply the update. If UPDATE_DIFF it only makes the changes
  //       specified in the spec. If UPDATE_REPLACE it makes the necessary
  //       changes for the resource to mirror the spec.
  // Return:
  //   Status: The status of the operation.
  virtual ::util::Status Update(const ContainerSpec &spec,
                                Container::UpdatePolicy policy) = 0;

  // Populates this resource's portion of the ContainerStats.
  virtual ::util::Status Stats(
      Container::StatsType type,
      ContainerStats *output) const = 0;

  // Populates this resource's portion of the ContainerSpec.
  // As with Stats, this can be an expensive call which does a lot of kernel
  // communication to build the spec, and may not just return a quick copy of
  // whatever population has come through the ResourceHandler itself.
  virtual ::util::Status Spec(ContainerSpec *spec) const = 0;

  // Configure a newly created container.
  virtual ::util::Status Create(const ContainerSpec &spec) = 0;

  // Destroys the resource. On success, deletes itself.
  virtual ::util::Status Destroy() = 0;

  // Enters the specified TIDs into this Resource Handler.
  virtual ::util::Status Enter(const ::std::vector<pid_t> &tids) = 0;

  // Registers a notification for the specified event.
  //
  // Arguments:
  //   spec: The event specification, can only contain one event.
  //   callback: Used to deliver the notification with the status argument
  //       indicating if there were any errors or simply the delivery of a
  //       notification (i.e.: Status of OK). Takes ownership of the pointer.
  // Return:
  //   StatusOr: Status of the operation. Iff OK, an ID for the notification is
  //       provided. If no event that can be handled is found in the spec, a
  //       status of NOT_FOUND is returned.
  virtual ::util::StatusOr<Container::NotificationId> RegisterNotification(
      const EventSpec &spec, Callback1< ::util::Status> *callback) = 0;

  // Returns the absolute name of the container this Resource Handler pertains
  // to.
  const string &container_name() const { return container_name_; }

  // Returns what type of resource is managed by this Resource Handler.
  ResourceType type() const { return type_; }

 protected:
  ResourceHandler(const string &container_name, ResourceType type)
      : container_name_(container_name), type_(type) {}

  string container_name_;
  ResourceType type_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourceHandler);
};

}  // namespace lmctfy
}  // namespace containers

#endif  // SRC_RESOURCE_HANDLER_H_
