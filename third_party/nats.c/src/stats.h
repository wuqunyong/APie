// Copyright 2015-2018 The NATS Authors
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#ifndef STATS_H_
#define STATS_H_

#include <stdint.h>

#include "status.h"

struct __natsStatistics
{
    uint64_t    inMsgs;
    uint64_t    outMsgs;
    uint64_t    inBytes;
    uint64_t    outBytes;
    uint64_t    reconnects;

};

#endif /* STATS_H_ */
