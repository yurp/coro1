// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Iurii Pelykh

#pragma once

#define CO1_ENABLE_TRACE

#ifdef CO1_ENABLE_TRACE
#include <iostream>
#define TRACE(msg) (std::cerr << msg << std::endl)
#else
#define TRACE(msg) ((void)0)
#endif
