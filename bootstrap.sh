#!/bin/sh
sudo apt install -y g++ pkg-config curl zip unzip tar git wget ninja-build cmake gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

if [ ! -d "$HOME/vcpkg" ]; then
    git clone https://github.com/microsoft/vcpkg.git "$HOME/vcpkg"
fi

"$HOME/vcpkg/bootstrap-vcpkg.sh" -disableMetrics

if ! grep -q "VCPKG_ROOT" "$HOME/.bashrc"; then
    echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> "$HOME/.bashrc"
    echo 'export PATH="$VCPKG_ROOT:$PATH"' >> "$HOME/.bashrc"
fi

mkdir -p "$HOME/vcpkg/triplets/community"
cp -v cmake/triplets/* "$HOME/vcpkg/triplets/community/"