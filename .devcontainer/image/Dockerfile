# SPDX-FileCopyrightText: 2026 Théo Magne <theo.magne@allcircuits.com>
#
# SPDX-License-Identifier: LicenseRef-ALLCircuits-ACT-1.1

# =============================================================================
# Toolchain image: the dev container and the CI jobs
# =============================================================================
# The CI builds this image and publishes it under the git tree hash of this directory (see
# .github/workflows/ci.yml); the dev container and the CI jobs then run the same image, so a check
# that passes in one passes in the other. A layer that changes is rebuilt and republished together
# with every layer below it, so the layers go from the most stable to the most volatile:
#
#   1. Debian base toolchain and utilities    never changes
#   2. LLVM 21                                bumped with the clang-format / clang-tidy version
#   3. Package dependencies                   the most volatile: every new system dependency
#   4. User and Python tools                  small, versions bumped now and then
#
# A library a package links against goes in layer 3, nowhere else, even if apt would take it
# earlier: an early layer rebuilds everything below it.
# =============================================================================

FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# =============================================================================
# Debian base toolchain and utilities
# =============================================================================
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    gdb \
    git \
    ca-certificates \
    curl \
    sudo \
    locales \
    && rm -rf /var/lib/apt/lists/*

RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen \
    && locale-gen

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US:en \
    LC_ALL=en_US.UTF-8

# =============================================================================
# LLVM 21: clang-format, clang-tidy, clangd
# =============================================================================
# The version .clang-tidy and .clang-format are written for; the CI lints with it. The unversioned
# names point at it so a command typed by hand runs the same binary as the CI.
RUN echo "deb [trusted=yes] http://apt.llvm.org/trixie/ llvm-toolchain-trixie-21 main" > /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y --no-install-recommends \
    clang-21 \
    clangd-21 \
    clang-tidy-21 \
    clang-format-21 \
    && ln -sf clang-21 /usr/bin/clang \
    && ln -sf clang-format-21 /usr/bin/clang-format \
    && ln -sf clang-tidy-21 /usr/bin/clang-tidy \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Package dependencies
# =============================================================================
# What the packages link against. The only layer a new dependency should touch.
RUN apt-get update && apt-get install -y --no-install-recommends \
    # act_linux_io
    libgpiod-dev \
    # act_db_sqlite
    libsqlitecpp-dev \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# User and Python tools
# =============================================================================
ARG USERNAME=dev

RUN useradd -m -s /bin/bash $USERNAME \
    && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

# cpp-linter drives clang-format and clang-tidy and posts the findings on a pull request: the CI
# runs it, and the same binary gives developers the linter the CI uses. Installed with pipx in the
# user's home; its shims are on PATH for every user, which is what the CI's root needs.
RUN apt-get update && apt-get install -y --no-install-recommends \
    pipx \
    && rm -rf /var/lib/apt/lists/*

USER $USERNAME
RUN pipx install cpp-linter==1.14.0
ENV PATH="/home/${USERNAME}/.local/bin:${PATH}"
USER root

# The CI runs this image as root (GitHub runs a container job as the image's default user without
# mapping uids, and the runner's work directory is writable by root only), and root then drives a
# repository owned by the runner's uid, which git refuses as "dubious ownership" unless the
# directory is trusted. This image is a toolchain, the only repository it sees is this one.
RUN git config --system --add safe.directory '*'

SHELL ["/bin/bash", "-c"]

RUN echo "=== Development Environment ===" \
    && echo "GCC: $(gcc --version | head -1)" \
    && echo "CMake: $(cmake --version | head -1)" \
    && echo "Ninja: $(ninja --version)" \
    && echo "clang-tidy: $(clang-tidy --version | grep -o 'LLVM version [0-9.]*')" \
    && echo "================================"

# Default user: root, for the CI's container jobs (see the git trust comment above). The dev
# container switches to ${USERNAME} through remoteUser in devcontainer.json.
WORKDIR /home/${USERNAME}
CMD ["/bin/bash"]
