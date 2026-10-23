#!/bin/sh

# SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# Install dependencies for ACT C++ packages
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    libarchive-dev \
    libgpiod-dev \
    libsqlitecpp-dev
